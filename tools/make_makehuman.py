#!/usr/bin/env python3
"""Builds a real-body benchmark case from the MakeHuman base mesh.

Downloads (CC0, see data/README.md) the MakeHuman base mesh, default
skeleton and skin weights from the makehuman repository on GitHub, and writes:

    data/makehuman/body.gltf   the "body" group of base.obj, rigged with the
                               default skeleton (163 joints) and artist weights
    data/makehuman/skirt.obj   the "helper-skirt" group: a skirt-shaped proxy
    data/makehuman/tights.obj  the "helper-tights" group: a full-body tight suit
    data/makehuman/sleeve_tight.obj, sleeve_loose.obj, trousers.obj, shirt.obj
                               body regions (by dominant bone) pushed out along
                               their normals by 1.5 to 6 cm, with the body's own
                               topology: garments with real shapes and real
                               looseness, with no cloth of their own
    data/makehuman/poses.json  train/test pose sets on the leg, arm and spine bones
    data/makehuman/hem.json    leg-driven hem rig for the skirt (weightbench --hem-rig)

Needs: pip install numpy pygltflib
"""

import json
import os
import sys
import hashlib
import urllib.request

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_synthetic import rot, write_obj, write_rig_gltf  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "data", "makehuman")
RAW = "https://raw.githubusercontent.com/makehumancommunity/makehuman/master/makehuman/data/"
FILES = {
    "base.obj": RAW + "3dobjs/base.obj",
    "default.mhskel": RAW + "rigs/default.mhskel",
    "default_weights.mhw": RAW + "rigs/default_weights.mhw",
}
# The files come from a moving branch, so the download is pinned by content:
# SHA-256 of each file as fetched on 2026-10-10. A mismatch stops the script
# rather than silently changing the benchmark body.
SHA256 = {
    "base.obj": "8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c",
    "default.mhskel": "99f179bce0aa850b45d4191a1d0d234c5851f881c057439470ded3bddf729a24",
    "default_weights.mhw": "0f3641d651ae3d00ad6b4ccee43142edb109d3bd909d27d9e4139ef1beed8625",
}


def fetch(out):
    raw = os.path.join(out, "raw")
    os.makedirs(raw, exist_ok=True)
    for name, url in FILES.items():
        path = os.path.join(raw, name)
        if not os.path.exists(path):
            print("downloading", url)
            urllib.request.urlretrieve(url, path)
        with open(path, "rb") as f:
            digest = hashlib.sha256(f.read()).hexdigest()
        if digest != SHA256[name]:
            sys.exit(f"{name}: SHA-256 {digest} does not match the pinned {SHA256[name]}; "
                     "upstream changed, or the download is corrupt. Delete it to retry, "
                     "or update SHA256 with a decision log entry.")
    return raw


def read_obj_groups(path):
    """All vertices, and the (polygon) faces of each group, 0-based."""
    V, groups, current = [], {}, None
    with open(path) as f:
        for line in f:
            if line.startswith("v "):
                V.append([float(x) for x in line.split()[1:4]])
            elif line.startswith("g "):
                current = line.split()[1]
                groups.setdefault(current, [])
            elif line.startswith("f ") and current is not None:
                groups[current].append([int(x.split("/")[0]) - 1 for x in line.split()[1:]])
    return np.array(V), groups


def extract(V, faces):
    """A group as its own mesh: used vertices reindexed, polygons fanned into triangles."""
    used = sorted({i for face in faces for i in face})
    remap = {old: new for new, old in enumerate(used)}
    F = []
    for face in faces:
        for k in range(1, len(face) - 1):
            F.append([remap[face[0]], remap[face[k]], remap[face[k + 1]]])
    return V[used], np.array(F), used


def vertex_normals(V, F):
    """Area-weighted vertex normals."""
    n = np.cross(V[F[:, 1]] - V[F[:, 0]], V[F[:, 2]] - V[F[:, 0]])
    N = np.zeros_like(V)
    for k in range(3):
        np.add.at(N, F[:, k], n)
    return N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)


def main(out):
    raw = fetch(out)
    V, groups = read_obj_groups(os.path.join(raw, "base.obj"))
    skel = json.load(open(os.path.join(raw, "default.mhskel")))
    mhw = json.load(open(os.path.join(raw, "default_weights.mhw")))

    # Joint positions: the centroid of each joint's helper vertices.
    joint_pos = {name: V[idx].mean(axis=0) for name, idx in skel["joints"].items()}
    names = list(skel["bones"].keys())
    # Parents must come before children for write_rig_gltf's node order.
    ordered = []
    def visit(n):
        if n not in ordered:
            p = skel["bones"][n]["parent"]
            if p:
                visit(p)
            ordered.append(n)
    for n in names:
        visit(n)
    index = {n: k for k, n in enumerate(ordered)}
    joints = []
    for n in ordered:
        b = skel["bones"][n]
        joints.append((n, index[b["parent"]] if b["parent"] else -1, joint_pos[b["head"]]))

    body_V, body_F, body_used = extract(V, groups["body"])
    remap = {old: new for new, old in enumerate(body_used)}
    W = np.zeros((len(body_V), len(ordered)))
    for bone, pairs in mhw["weights"].items():
        if bone not in index:
            continue
        for v, w in pairs:
            if v in remap:
                W[remap[v], index[bone]] += w
    row = W.sum(axis=1, keepdims=True)
    unweighted = int((row[:, 0] <= 0).sum())
    if unweighted:
        # Give any unweighted vertex the root bone so every row sums to 1.
        W[row[:, 0] <= 0, index["root"]] = 1.0
        row = W.sum(axis=1, keepdims=True)
    W /= row
    os.makedirs(out, exist_ok=True)
    write_rig_gltf(os.path.join(out, "body.gltf"), body_V, body_F, W, joints)
    print(f"body: {len(body_V)} vertices, {len(body_F)} triangles, {len(ordered)} joints, "
          f"{unweighted} vertices had no weights")

    for group, name in (("helper-skirt", "skirt"), ("helper-tights", "tights")):
        gV, gF, _ = extract(V, groups[group])
        write_obj(os.path.join(out, name + ".obj"), gV, gF)
        lo, hi = gV.min(axis=0), gV.max(axis=0)
        print(f"{name}: {len(gV)} vertices, {len(gF)} triangles, y {lo[1]:.2f} to {hi[1]:.2f}")

    # Garments cut from the body: faces whose vertices are all dominated by
    # one of the region's bones, offset along the vertex normals.
    normals = vertex_normals(body_V, body_F)
    dominant = W.argmax(axis=1)
    bone_index = {n: k for k, n in enumerate(ordered)}

    def region(prefixes):
        bones = {k for n, k in bone_index.items() if any(n.startswith(p) for p in prefixes)}
        inside = np.isin(dominant, list(bones))
        faces = body_F[inside[body_F].all(axis=1)]
        return faces

    arm_l = ["upperarm01.L", "upperarm02.L", "lowerarm01.L", "lowerarm02.L"]
    legs_both = ["upperleg01", "upperleg02", "lowerleg01", "lowerleg02", "pelvis", "root"]
    torso = ["spine01", "spine02", "spine03", "spine04", "spine05", "clavicle", "shoulder01",
             "upperarm01", "upperarm02", "breast", "root", "pelvis"]
    for name, prefixes, offset in (("sleeve_tight", arm_l, 0.15), ("sleeve_loose", arm_l, 0.6),
                                   ("trousers", legs_both, 0.3), ("shirt", torso, 0.4)):
        faces = region(prefixes)
        gV, gF, used = extract(body_V, faces)
        gV = gV + offset * normals[used]
        write_obj(os.path.join(out, name + ".obj"), gV, gF)
        print(f"{name}: {len(gV)} vertices, {len(gF)} triangles, offset {offset * 10:.1f} cm")

    # Poses. MakeHuman is Y up, the character faces +Z, and the rest skeleton
    # has no joint rotations, so axes are world axes. A leg points down, so
    # rotating it about -X swings it forward and +X on a knee folds it back;
    # +Z moves the left leg or arm outward; +X on the spine bends forward.
    def elbow_axis(side):
        # Perpendicular to the upper arm and to the forward direction, signed
        # so that a positive angle brings the forearm forward.
        upper = joint_pos[skel["bones"][f"upperarm01.{side}"]["head"]]
        elbow = joint_pos[skel["bones"][f"lowerarm01.{side}"]["head"]]
        axis = np.cross(elbow - upper, [0, 0, 1])
        return (axis / np.linalg.norm(axis)).tolist()

    def legs(a):
        return {"upperleg01.L": rot([1, 0, 0], -a), "upperleg01.R": rot([1, 0, 0], a)}

    def sitting(hip, knee):
        return {"upperleg01.L": rot([1, 0, 0], -hip), "upperleg01.R": rot([1, 0, 0], -hip),
                "lowerleg01.L": rot([1, 0, 0], knee), "lowerleg01.R": rot([1, 0, 0], knee)}

    poses = {
        "train": [
            {"name": "stride30", "joints": legs(30)},
            {"name": "stride50", "joints": legs(50)},
            {"name": "kneeL60", "joints": {"lowerleg01.L": rot([1, 0, 0], 60)}},
            {"name": "squat", "joints": sitting(70, 90)},
            {"name": "spread30", "joints": {"upperleg01.L": rot([0, 0, 1], 30), "upperleg01.R": rot([0, 0, 1], -30)}},
            {"name": "elbowL90", "joints": {"lowerarm01.L": rot(elbow_axis("L"), 90)}},
            {"name": "armsUp60", "joints": {"upperarm01.L": rot([0, 0, 1], 60), "upperarm01.R": rot([0, 0, 1], -60)}},
            {"name": "bend20", "joints": {"spine02": rot([1, 0, 0], 20), "spine03": rot([1, 0, 0], 15)}},
        ],
        "test": [
            {"name": "stride20", "joints": legs(20)},
            {"name": "stride40", "joints": legs(40)},
            {"name": "stride60", "joints": legs(60)},
            {"name": "kneeR45", "joints": {"lowerleg01.R": rot([1, 0, 0], 45)}},
            {"name": "stepL", "joints": {"upperleg01.L": rot([1, 0, 0], -45), "lowerleg01.L": rot([1, 0, 0], 60)}},
            {"name": "sit", "joints": sitting(90, 90)},
            {"name": "spread45", "joints": {"upperleg01.L": rot([0, 0, 1], 45), "upperleg01.R": rot([0, 0, 1], -45)}},
            {"name": "kick", "joints": {"upperleg01.R": rot([1, 0, 0], -80), "lowerleg01.R": rot([1, 0, 0], 20)}},
            {"name": "elbowR120", "joints": {"lowerarm01.R": rot(elbow_axis("R"), 120)}},
            {"name": "armsUp90", "joints": {"upperarm01.L": rot([0, 0, 1], 90), "upperarm01.R": rot([0, 0, 1], -90)}},
            {"name": "bend35", "joints": {"spine02": rot([1, 0, 0], 35), "spine03": rot([1, 0, 0], 20)}},
            {"name": "twist", "joints": {"spine02": rot([0, 1, 0], 30), "spine03": rot([0, 1, 0], 20)}},
        ],
    }
    with open(os.path.join(out, "poses.json"), "w") as f:
        json.dump(poses, f, indent=1)
    # Leg-driven hem rig for the skirt (weightbench --hem-rig).
    with open(os.path.join(out, "hem.json"), "w") as f:
        json.dump({"apply": "all", "count": 8, "tiers": 2, "parent": "root", "follow": 1.0, "falloff": 2, "lower": "gravity", "drive": "side", "waist": 0.2,
                   "legs": [["upperleg01.L", "lowerleg01.L", "foot.L"], ["upperleg01.R", "lowerleg01.R", "foot.R"]],
                   "move": ["pelvis.L", "pelvis.R"]}, f, indent=1)
    print(f"wrote {out}")


if __name__ == "__main__":
    main(OUT)
