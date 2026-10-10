#!/usr/bin/env python3
"""Generates the synthetic weight-transfer benchmark into data/synthetic/.

Two capsule "bodies" with hand-set skin weights, a few garments around each,
and train/test pose sets. Everything is procedural, so the benchmark is free
of licence questions and reproduces exactly from this file.

    python3 tools/make_synthetic.py [data/synthetic]

Cases written (body glTF, garment OBJs, poses JSON):

  arm/    three-joint arm (shoulder, elbow, wrist)
          sleeve_tight.obj   tube hugging the arm
          sleeve_loose.obj   wide tube, far from the arm at the elbow
          cuff.obj           separate ring around the wrist, plus a detached button
  legs/   pelvis with two legs (hip, knee, ankle each)
          trousers.obj       two tubes joined at a waistband
          skirt.obj          cone from the waist to above the knees (crosses between the legs)
          dress.obj          long cone to the ankles

Needs: pip install numpy pygltflib
"""

import json
import os
import struct
import sys

import numpy as np
import pygltflib

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                                                        "data", "synthetic")


# --- geometry helpers --------------------------------------------------------

def capsule(p0, p1, radius, sections=24, ring_spacing=None):
    """Closed capsule from p0 to p1, as rings of `sections` vertices spaced
    about `ring_spacing` (default half the radius) along the whole length,
    caps included, so skin weights can blend along it. (The first version
    used trimesh capsules, whose cylinder has only its two end rings; the
    joints of those bodies were hard-skinned by accident.)"""
    p0, p1 = np.asarray(p0, float), np.asarray(p1, float)
    height = np.linalg.norm(p1 - p0)
    spacing = ring_spacing or radius / 2
    # Profile (distance along axis, radius): bottom cap, straight part, top cap.
    cap_steps = max(2, int(np.ceil(np.pi / 2 * radius / spacing)))
    profile = []
    for a in np.linspace(-np.pi / 2, 0, cap_steps, endpoint=False)[1:]:
        profile.append((radius * np.sin(a), radius * np.cos(a)))
    for t in np.linspace(0, height, max(2, int(np.ceil(height / spacing)) + 1)):
        profile.append((t, radius))
    for a in np.linspace(0, np.pi / 2, cap_steps, endpoint=False)[1:]:
        profile.append((height + radius * np.sin(a), radius * np.cos(a)))
    d = (p1 - p0) / height
    up = np.array([0.0, 1.0, 0.0]) if abs(d[1]) < 0.9 else np.array([1.0, 0.0, 0.0])
    u = np.cross(d, up)
    u /= np.linalg.norm(u)
    w = np.cross(d, u)
    V = [p0 - radius * d]  # bottom pole
    for t, r in profile:
        for k in range(sections):
            a = 2 * np.pi * k / sections
            V.append(p0 + t * d + r * (np.cos(a) * u + np.sin(a) * w))
    top = len(V)
    V.append(p1 + radius * d)  # top pole
    F = []
    n = len(profile)
    for k in range(sections):
        F.append([0, 1 + (k + 1) % sections, 1 + k])
    for i in range(n - 1):
        for k in range(sections):
            a, b = 1 + i * sections + k, 1 + i * sections + (k + 1) % sections
            c, e = a + sections, b + sections
            F.append([a, b, c])
            F.append([b, e, c])
    for k in range(sections):
        a = 1 + (n - 1) * sections
        F.append([top, a + k, a + (k + 1) % sections])
    return np.array(V), np.array(F)


def tube(path, radii, sections=32, rings_per_segment=8, closed_top=False, closed_bottom=False):
    """Open tube of circular rings around a polyline, radius per path point."""
    path = np.asarray(path, float)
    radii = np.asarray(radii, float)
    # Resample the path and radii.
    pts, rads = [], []
    for i in range(len(path) - 1):
        for t in np.linspace(0, 1, rings_per_segment, endpoint=(i == len(path) - 2)):
            pts.append(path[i] * (1 - t) + path[i + 1] * t)
            rads.append(radii[i] * (1 - t) + radii[i + 1] * t)
    pts, rads = np.array(pts), np.array(rads)
    V, F = [], []
    # A fixed frame keeps the tube from twisting: use the path's overall direction.
    d = pts[-1] - pts[0]
    d /= np.linalg.norm(d)
    up = np.array([0.0, 1.0, 0.0]) if abs(d[1]) < 0.9 else np.array([1.0, 0.0, 0.0])
    u = np.cross(d, up)
    u /= np.linalg.norm(u)
    w = np.cross(d, u)
    for p, r in zip(pts, rads):
        for k in range(sections):
            a = 2 * np.pi * k / sections
            V.append(p + r * (np.cos(a) * u + np.sin(a) * w))
    n = len(pts)
    for i in range(n - 1):
        for k in range(sections):
            a, b = i * sections + k, i * sections + (k + 1) % sections
            c, e = a + sections, b + sections
            F.append([a, b, c])  # outward-facing, matching the capsule bodies
            F.append([b, e, c])
    V = np.array(V)
    F = np.array(F)
    if closed_top or closed_bottom:
        for ring, close in ((0, closed_top), (n - 1, closed_bottom)):
            if close:
                centre = len(V)
                V = np.vstack([V, pts[ring]])
                for k in range(sections):
                    a, b = ring * sections + k, ring * sections + (k + 1) % sections
                    F = np.vstack([F, [centre, a, b] if ring == 0 else [centre, b, a]])
    return V, F


def merge(parts):
    V = np.vstack([p[0] for p in parts])
    F, off = [], 0
    for v, f in parts:
        F.append(np.asarray(f) + off)
        off += len(v)
    return V, np.vstack(F)


def write_obj(path, V, F):
    with open(path, "w") as f:
        f.write("# synthetic garment, see tools/make_synthetic.py\n")
        for v in V:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        for t in F:
            f.write(f"f {t[0] + 1} {t[1] + 1} {t[2] + 1}\n")


# --- skeleton and weights ----------------------------------------------------

def chain_weights(V, joints, falloff):
    """Weights for a chain of bones: each vertex blends the two bones whose
    segments it sits between, with a smooth ramp of width `falloff` at joints.

    joints: list of (name, parent_index, world_position). A bone j runs from
    joint j to its first child (leaf joints have no bone)."""
    W = np.zeros((len(V), len(joints)))
    children = {j: [k for k, (_, p, _) in enumerate(joints) if p == j] for j in range(len(joints))}
    bones = [(j, children[j][0]) for j in range(len(joints)) if children[j]]
    for i, v in enumerate(V):
        best = None
        for j, c in bones:
            a, b = joints[j][2], joints[c][2]
            ab = b - a
            t = np.clip(np.dot(v - a, ab) / np.dot(ab, ab), 0.0, 1.0)
            d = np.linalg.norm(v - (a + t * ab))
            if best is None or d < best[0]:
                best = (d, j, c, t, np.linalg.norm(ab))
        _, j, c, t, length = best
        # Blend 0 to 1 over a ramp of 2 * falloff centred on each joint that
        # has a bone on both sides: into the child bone near the far joint,
        # and into the parent bone near the near joint.
        s_end = float(np.clip(0.5 - (1.0 - t) * length / (2 * falloff), 0.0, 0.5)) if children[c] else 0.0
        p = joints[j][1]
        s_start = float(np.clip(0.5 - t * length / (2 * falloff), 0.0, 0.5)) if p >= 0 and children[p] else 0.0
        W[i, j] += 1.0 - s_end - s_start
        W[i, c] += s_end
        if s_start > 0.0:
            W[i, p] += s_start
    return W


def write_rig_gltf(path, V, F, W, joints):
    """Writes a skinned glTF (embedded buffer) with one node per joint."""
    V = np.ascontiguousarray(V, dtype=np.float32)
    F = np.ascontiguousarray(F, dtype=np.uint32).ravel()
    n = len(V)
    # Top-4 influences per vertex, as glTF requires.
    order = np.argsort(-W, axis=1)[:, :4]
    J = np.ascontiguousarray(order, dtype=np.uint16)
    Wk = np.take_along_axis(W, order, axis=1)
    Wk = np.ascontiguousarray(Wk / np.maximum(Wk.sum(axis=1, keepdims=True), 1e-12), dtype=np.float32)

    # Inverse bind matrices: joints have no rotation at rest, so just a translation back.
    ibm = []
    for _, _, p in joints:
        m = np.eye(4, dtype=np.float32)
        m[:3, 3] = -p
        ibm.append(m.T.ravel())  # column-major
    ibm = np.ascontiguousarray(np.array(ibm, dtype=np.float32))

    blobs = [V.tobytes(), F.tobytes(), J.tobytes(), Wk.tobytes(), ibm.tobytes()]
    offsets, data = [], b""
    for b in blobs:
        offsets.append(len(data))
        data += b + b"\0" * ((4 - len(b) % 4) % 4)

    g = pygltflib.GLTF2()
    g.buffers = [pygltflib.Buffer(byteLength=len(data))]
    g.bufferViews = [
        pygltflib.BufferView(buffer=0, byteOffset=offsets[0], byteLength=len(blobs[0]), target=34962),
        pygltflib.BufferView(buffer=0, byteOffset=offsets[1], byteLength=len(blobs[1]), target=34963),
        pygltflib.BufferView(buffer=0, byteOffset=offsets[2], byteLength=len(blobs[2]), target=34962),
        pygltflib.BufferView(buffer=0, byteOffset=offsets[3], byteLength=len(blobs[3]), target=34962),
        pygltflib.BufferView(buffer=0, byteOffset=offsets[4], byteLength=len(blobs[4])),
    ]
    g.accessors = [
        pygltflib.Accessor(bufferView=0, componentType=5126, count=n, type="VEC3",
                           min=V.min(axis=0).tolist(), max=V.max(axis=0).tolist()),
        pygltflib.Accessor(bufferView=1, componentType=5125, count=len(F), type="SCALAR"),
        pygltflib.Accessor(bufferView=2, componentType=5123, count=n, type="VEC4"),
        pygltflib.Accessor(bufferView=3, componentType=5126, count=n, type="VEC4"),
        pygltflib.Accessor(bufferView=4, componentType=5126, count=len(joints), type="MAT4"),
    ]
    g.meshes = [pygltflib.Mesh(primitives=[pygltflib.Primitive(
        attributes=pygltflib.Attributes(POSITION=0, JOINTS_0=2, WEIGHTS_0=3), indices=1)])]
    # Nodes: 0 = mesh, then one per joint with local translation relative to its parent.
    nodes = [pygltflib.Node(name="body", mesh=0, skin=0)]
    for k, (name, parent, p) in enumerate(joints):
        local = p - joints[parent][2] if parent >= 0 else p
        nodes.append(pygltflib.Node(name=name, translation=local.tolist()))
    for k, (_, parent, _) in enumerate(joints):
        if parent >= 0:
            nodes[parent + 1].children.append(k + 1)
    g.nodes = nodes
    roots = [k + 1 for k, (_, parent, _) in enumerate(joints) if parent < 0]
    g.skins = [pygltflib.Skin(joints=list(range(1, len(joints) + 1)), inverseBindMatrices=4, skeleton=roots[0])]
    g.scenes = [pygltflib.Scene(nodes=[0] + roots)]
    g.scene = 0
    g.set_binary_blob(data)
    g.convert_buffers(pygltflib.BufferFormat.DATAURI)
    g.save(path)


def rot(axis, deg):
    return {"axis": list(axis), "deg": deg}


# --- cases ---------------------------------------------------------------------

def make_arm(out):
    os.makedirs(out, exist_ok=True)
    shoulder, elbow, wrist, tip = (np.array(p) for p in
                                   ([0, 0, 0], [0.30, 0, 0], [0.56, 0, 0], [0.64, 0, 0]))
    joints = [("shoulder", -1, shoulder), ("elbow", 0, elbow), ("wrist", 1, wrist), ("hand_end", 2, tip)]
    V, F = merge([capsule(shoulder, elbow, 0.05), capsule(elbow, wrist, 0.042), capsule(wrist, tip, 0.035)])
    W = chain_weights(V, joints, falloff=0.03)
    write_rig_gltf(os.path.join(out, "body.gltf"), V, F, W, joints)

    along = [shoulder + [0.02, 0, 0], elbow, wrist - [0.02, 0, 0]]
    write_obj(os.path.join(out, "sleeve_tight.obj"), *tube(along, [0.062, 0.058, 0.052]))
    write_obj(os.path.join(out, "sleeve_loose.obj"), *tube(along, [0.07, 0.12, 0.09]))
    cuff = tube([wrist - [0.03, 0, 0], wrist + [0.01, 0, 0]], [0.05, 0.05], rings_per_segment=3)
    button_v, button_f = capsule(elbow + [0, 0.13, 0], elbow + [0.012, 0.13, 0], 0.012, sections=8)
    write_obj(os.path.join(out, "cuff.obj"), *merge([cuff, (button_v, button_f)]))

    poses = {
        "train": [
            {"name": "elbow45", "joints": {"elbow": rot([0, 0, 1], 45)}},
            {"name": "elbow90", "joints": {"elbow": rot([0, 0, 1], 90)}},
            {"name": "elbow120", "joints": {"elbow": rot([0, 0, 1], 120)}},
            {"name": "twist60", "joints": {"wrist": rot([1, 0, 0], 60)}},
            {"name": "raise60", "joints": {"shoulder": rot([0, 0, 1], 60)}},
        ],
        "test": [
            {"name": "elbow30", "joints": {"elbow": rot([0, 0, 1], 30)}},
            {"name": "elbow75", "joints": {"elbow": rot([0, 0, 1], 75)}},
            {"name": "elbow105", "joints": {"elbow": rot([0, 0, 1], 105)}},
            {"name": "elbow135", "joints": {"elbow": rot([0, 0, 1], 135)}},
            {"name": "elbow90_twist90", "joints": {"elbow": rot([0, 0, 1], 90), "wrist": rot([1, 0, 0], 90)}},
            {"name": "raise90_elbow60", "joints": {"shoulder": rot([0, 0, 1], 90), "elbow": rot([0, 0, 1], 60)}},
            {"name": "swing45", "joints": {"shoulder": rot([0, 1, 0], 45)}},
        ],
    }
    with open(os.path.join(out, "poses.json"), "w") as f:
        json.dump(poses, f, indent=1)


def make_legs(out):
    os.makedirs(out, exist_ok=True)
    pelvis = np.array([0, 0, 0])
    joints = [("pelvis", -1, pelvis)]
    parts = [capsule([-0.1, 0.02, 0], [0.1, 0.02, 0], 0.09)]
    for side, x in (("L", -0.09), ("R", 0.09)):
        hip, knee, ankle, toe = (np.array(p) for p in ([x, -0.02, 0], [x, -0.47, 0], [x, -0.90, 0], [x, -0.98, 0]))
        base = len(joints)
        joints += [(f"hip_{side}", 0, hip), (f"knee_{side}", base, knee), (f"ankle_{side}", base + 1, ankle),
                   (f"toe_{side}", base + 2, toe)]
        parts += [capsule(hip, knee, 0.075), capsule(knee, ankle, 0.055), capsule(ankle, toe, 0.045)]
    V, F = merge(parts)
    W = chain_weights(V, joints, falloff=0.04)
    write_rig_gltf(os.path.join(out, "body.gltf"), V, F, W, joints)

    waist = np.array([0, 0.06, 0])
    legs = []
    for x in (-0.09, 0.09):
        legs.append(tube([[x, 0.0, 0], [x, -0.47, 0], [x, -0.86, 0]], [0.10, 0.085, 0.07]))
    band = tube([waist + [0, 0.04, 0], waist - [0, 0.06, 0]], [0.2, 0.2], rings_per_segment=3)
    write_obj(os.path.join(out, "trousers.obj"), *merge(legs + [band]))
    write_obj(os.path.join(out, "skirt.obj"), *tube([waist + [0, 0.04, 0], [0, -0.40, 0]], [0.2, 0.30], rings_per_segment=16))
    write_obj(os.path.join(out, "dress.obj"), *tube([waist + [0, 0.04, 0], [0, -0.45, 0], [0, -0.88, 0]], [0.2, 0.26, 0.38],
                                                      rings_per_segment=12))

    def stride(a):
        return {"hip_L": rot([1, 0, 0], a), "hip_R": rot([1, 0, 0], -a)}

    poses = {
        "train": [
            {"name": "stride30", "joints": stride(30)},
            {"name": "stride50", "joints": stride(50)},
            {"name": "kneeL60", "joints": {"knee_L": rot([1, 0, 0], 60)}},
            {"name": "squat", "joints": {"hip_L": rot([1, 0, 0], 70), "hip_R": rot([1, 0, 0], 70),
                                          "knee_L": rot([1, 0, 0], -90), "knee_R": rot([1, 0, 0], -90)}},
            {"name": "spread30", "joints": {"hip_L": rot([0, 0, 1], 30), "hip_R": rot([0, 0, 1], -30)}},
        ],
        "test": [
            {"name": "stride20", "joints": stride(20)},
            {"name": "stride40", "joints": stride(40)},
            {"name": "stride60", "joints": stride(60)},
            {"name": "kneeR45", "joints": {"knee_R": rot([1, 0, 0], 45)}},
            {"name": "stepL", "joints": {"hip_L": rot([1, 0, 0], 45), "knee_L": rot([1, 0, 0], -60)}},
            {"name": "sit", "joints": {"hip_L": rot([1, 0, 0], 90), "hip_R": rot([1, 0, 0], 90),
                                        "knee_L": rot([1, 0, 0], -90), "knee_R": rot([1, 0, 0], -90)}},
            {"name": "spread45", "joints": {"hip_L": rot([0, 0, 1], 45), "hip_R": rot([0, 0, 1], -45)}},
            {"name": "kick", "joints": {"hip_R": rot([1, 0, 0], 80), "knee_R": rot([1, 0, 0], -20)}},
        ],
    }
    with open(os.path.join(out, "poses.json"), "w") as f:
        json.dump(poses, f, indent=1)


if __name__ == "__main__":
    make_arm(os.path.join(OUT, "arm"))
    make_legs(os.path.join(OUT, "legs"))
    print(f"wrote {OUT}")
