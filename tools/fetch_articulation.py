"""Characters for RQ-002 (experiment 2026-10-10-auto-weight-baselines) from
Articulation-XL2.0, filtered and converted for bodybench.

  python3 tools/fetch_articulation.py select DATA LIST.csv
  python3 tools/fetch_articulation.py convert DATA LIST.csv OUT [--weld]
  python3 tools/fetch_articulation.py select-topology DATA OLD.csv LIST.csv

DATA holds the downloads (see data/README.md):
  articulation-xl2/meta.csv                    the dataset's metadata
  articulation-xl2/articulation_xlv2_test.npz  the test split
  objaverse-xl/sketchfab/sketchfab.parquet     per-object licences
  objaverse-xl/github/github.parquet

select keeps models whose own licence is CC-BY or CC0, with 1,000 to 100,000
vertices and 10 to 150 joints, and writes the first 5 of each body plan in
ascending uuid order:

  humanoid   category label exactly "character"
  quadruped  label exactly "animal", and exactly two pairs of leg chains
  creature   label containing "animal" or "mythical creature" but not
             "anthropomorphic", and not already a quadruped

A leg chain is an unbranched run of at least 3 joints hanging from a
branching joint (or the root). Two chains are a pair when they hang from the
same joint, have the same number of joints, and their lengths differ by at
most 10%. Category labels never say "quadruped", so this rule stands in for
it (approved by the director, logged as a protocol deviation).

convert writes OUT/<uuid>/body.glb (mesh, skeleton, and the four largest
weights per vertex, which is all glTF holds) and OUT/<uuid>/artist.csv (every
artist weight, rows normalised, as the dataset README advises), for
bodybench --artist-weights.

--weld merges vertices at exactly the same position (the dataset splits
meshes at UV seams, and some are unindexed triangle soups) and drops faces
that collapse. Merged vertices get the mean of their artist weights; the
largest disagreement is printed. This is a mesh clean-up, so under the
protocol it must be applied to every method and logged as a deviation.

select-topology (experiment 2026-10-10-auto-weight-baselines-v2) groups by
skeleton instead of label, and sizes the welded mesh. It writes two sets:

  same   the characters in OLD.csv, except c84ca8a7 (dropped by the
         director), regrouped
  fresh  the first 5 of each group in ascending uuid order among the other
         models of the test split whose label names a living thing
         (character, animal, mythical creature, anthropomorphic, humanoid)

Both keep only CC-BY or CC0 models with 10 to 150 joints and 1,000 to 100,000
vertices after welding. The groups:

  limb pair  two children of the same joint whose subtrees have the same
             number of joints and whose reach (largest distance from that
             joint to a joint in the subtree) differs by at most 10%
  limb skin  the vertices whose largest artist weight belongs to a joint of
             the limb's subtree
  leg pair   seen along one of the six axis directions, a limb pair whose
             two lowest joints both lie in the lowest 15% of the mesh's
             extent, at least 5% of the bounding-box diagonal apart, and each
             at least 10% of the diagonal below the joint the pair hangs from
  quadruped  2 leg pairs
  biped      1 leg pair and at least 2 limb pairs; or no leg pair by joints
             but 1 by skin (the lowest vertices of each limb's skin standing
             in for its lowest joints), for rigs that stop at the ankle
  other      the rest

The dataset ships as a pickled NumPy array, so loading it runs pickle. Only
load files downloaded from the dataset's own Hugging Face repository.
"""

import csv
import os
import sys

import numpy as np

LICENCES = {"Creative Commons - Attribution", "Creative Commons Zero v1.0 Universal"}
PER_GROUP = 5


def load_split(data):
    path = os.path.join(data, "articulation-xl2", "articulation_xlv2_test.npz")
    return {str(d["uuid"]): d for d in np.load(path, allow_pickle=True)["arr_0"]}


def tree(d):
    n = len(d["joint_names"])
    children = {i: [] for i in range(n)}
    parent = {int(d["root_index"]): -1}
    for a, b in d["bones"]:
        children[int(a)].append(int(b))
        parent[int(b)] = int(a)
    if len(parent) != n:
        raise ValueError(f"{d['uuid']}: bones do not form one tree")
    return parent, children


def leg_pairs(d):
    parent, children = tree(d)
    J = d["joints"]
    root = int(d["root_index"])
    chains = []
    for j in range(len(J)):
        if j == root or (parent[j] != root and len(children[parent[j]]) < 2):
            continue
        c = [j]
        while len(children[c[-1]]) == 1:
            c.append(children[c[-1]][0])
        if len(c) >= 3:
            pts = [J[parent[j]]] + [J[k] for k in c]
            length = sum(float(np.linalg.norm(pts[i + 1] - pts[i])) for i in range(len(pts) - 1))
            chains.append((parent[j], len(c), length))
    used, pairs = set(), 0
    for i, a in enumerate(chains):
        if i in used:
            continue
        for k in range(i + 1, len(chains)):
            b = chains[k]
            if k not in used and a[0] == b[0] and a[1] == b[1] and abs(a[2] - b[2]) <= 0.1 * max(a[2], b[2]):
                used |= {i, k}
                pairs += 1
                break
    return pairs


def select(data, out):
    import pandas as pd

    meta = pd.read_csv(os.path.join(data, "articulation-xl2", "meta.csv"))
    lic = pd.concat(
        pd.read_parquet(os.path.join(data, "objaverse-xl", s, s + ".parquet"), columns=["fileIdentifier", "license"])
        for s in ("sketchfab", "github")
    ).drop_duplicates("fileIdentifier")
    meta = meta.merge(lic, on="fileIdentifier", how="left")
    split = load_split(data)
    ok = meta[
        meta.uuid.isin(split)
        & meta.license.isin(LICENCES)
        & meta.vertex_count.between(1000, 100000)
        & meta.joint_count.between(10, 150)
    ].sort_values("uuid")
    print(f"{len(split)} models in the test split, {len(ok)} pass the licence and size filters")

    groups = {"humanoid": [], "quadruped": [], "creature": []}
    for _, r in ok.iterrows():
        tags = {t.strip() for t in r.category_label.lower().split(",")}
        if tags == {"character"}:
            group = "humanoid"
        elif tags == {"animal"} and leg_pairs(split[r.uuid]) == 2:
            group = "quadruped"
        elif tags & {"animal", "mythical creature"} and "anthropomorphic" not in tags:
            group = "creature"
        else:
            continue
        if len(groups[group]) < PER_GROUP:
            groups[group].append(r)

    with open(out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["group", "uuid", "category_label", "license", "source", "vertices", "joints", "fileIdentifier"])
        for group, rows in groups.items():
            for r in rows:
                w.writerow([group, r.uuid, r.category_label, r.license, r.source, r.vertex_count, r.joint_count,
                            r.fileIdentifier])
            print(f"{group}: {len(rows)}")


LIVING = {"character", "animal", "mythical creature", "anthropomorphic", "humanoid"}
DROPPED = {"c84ca8a774b142788dfd403ca139e4a0"}  # director, 2026-10-10


def limb_pairs(d):
    _, children = tree(d)
    J = d["joints"]
    pairs = []
    for p in range(len(J)):
        info = []
        for c in children[p]:
            sub = [c]
            k = 0
            while k < len(sub):
                sub += children[sub[k]]
                k += 1
            if len(sub) >= 2:
                info.append((len(sub), max(float(np.linalg.norm(J[s] - J[p])) for s in sub), sub, p))
        used = set()
        for i, a in enumerate(info):
            if i in used:
                continue
            for k in range(i + 1, len(info)):
                b = info[k]
                if k not in used and a[0] == b[0] and abs(a[1] - b[1]) <= 0.1 * max(a[1], b[1]):
                    used |= {i, k}
                    pairs.append((a[2], b[2], p))
                    break
    return pairs


def count_leg_pairs(d, lowest_points):
    """Largest number of leg pairs over the six axis directions. lowest_points
    maps (pair, axis, sign) to the two points that stand for the pair's feet."""
    V, J = d["vertices"], d["joints"]
    diag = float(np.linalg.norm(V.max(0) - V.min(0)))
    pairs = limb_pairs(d)
    best = 0
    for axis in range(3):
        for sign in (1, -1):
            h = sign * V[:, axis]
            lo, extent = h.min(), h.max() - h.min()
            n = 0
            for pair in pairs:
                feet = lowest_points(pair, axis, sign)
                if feet is None:
                    continue
                pa, pb = feet
                top = sign * J[pair[2], axis]
                ha, hb = sign * pa[axis], sign * pb[axis]
                if (ha - lo < 0.15 * extent and hb - lo < 0.15 * extent
                        and np.linalg.norm(pa - pb) > 0.05 * diag
                        and top - ha > 0.1 * diag and top - hb > 0.1 * diag):
                    n += 1
            best = max(best, n)
    return best, len(pairs)


def body_plan(d):
    J = d["joints"]

    def by_joint(pair, axis, sign):
        a, b, _ = pair
        return (J[min(a, key=lambda s: sign * J[s, axis])], J[min(b, key=lambda s: sign * J[s, axis])])

    legs, n_pairs = count_leg_pairs(d, by_joint)
    if legs == 2:
        return "quadruped", 4, n_pairs
    if legs == 1 and n_pairs >= 2:
        return "biped", 2, n_pairs
    if legs == 0:
        # Some rigs stop at the ankle or knee, so no joint reaches the ground.
        # Their skin does: a limb's skin is the vertices whose largest artist
        # weight belongs to the limb. Used only to find bipeds, since hanging
        # arms also reach low and would make false quadrupeds.
        V = d["vertices"]
        owner = np.argmax(artist_weights(d), axis=1)

        def by_skin(pair, axis, sign):
            a, b, _ = pair
            va, vb = V[np.isin(owner, a)], V[np.isin(owner, b)]
            if not len(va) or not len(vb):
                return None
            return va[np.argmin(sign * va[:, axis])], vb[np.argmin(sign * vb[:, axis])]

        skin_legs, _ = count_leg_pairs(d, by_skin)
        if skin_legs == 1 and n_pairs >= 2:
            return "biped", 2, n_pairs
    return "other", 2 * legs, n_pairs


def select_topology(data, old, out):
    import pandas as pd

    meta = pd.read_csv(os.path.join(data, "articulation-xl2", "meta.csv"))
    lic = pd.concat(
        pd.read_parquet(os.path.join(data, "objaverse-xl", s, s + ".parquet"), columns=["fileIdentifier", "license"])
        for s in ("sketchfab", "github")
    ).drop_duplicates("fileIdentifier")
    meta = meta.merge(lic, on="fileIdentifier", how="left").set_index("uuid")
    split = load_split(data)
    with open(old) as f:
        old_ids = [r["uuid"] for r in csv.DictReader(f)]

    def describe(uuid):
        d = split[uuid]
        welded, _, _ = weld(d, artist_weights(d))
        plan, legs, pairs = body_plan(d)
        return plan, legs, pairs, len(welded["vertices"])

    rows = []
    for uuid in old_ids:
        if uuid in DROPPED:
            continue
        plan, legs, pairs, nv = describe(uuid)
        if 1000 <= nv <= 100000:
            rows.append(("same", plan, uuid, legs, pairs, nv))
        else:
            print(f"same: {uuid} left out, {nv} vertices after welding")
    candidates = meta[
        meta.index.isin(split)
        & ~meta.index.isin(old_ids)
        & ~meta.index.isin(DROPPED)
        & meta.license.isin(LICENCES)
        & meta.joint_count.between(10, 150)
    ].sort_index()
    taken = {"biped": 0, "quadruped": 0, "other": 0}
    for uuid, r in candidates.iterrows():
        tags = {t.strip() for t in str(r.category_label).lower().split(",")}
        if not tags & LIVING:
            continue
        plan, legs, pairs, nv = describe(uuid)
        if not 1000 <= nv <= 100000 or taken[plan] >= PER_GROUP:
            continue
        taken[plan] += 1
        rows.append(("fresh", plan, uuid, legs, pairs, nv))
    with open(out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["set", "group", "uuid", "category_label", "license", "legs", "limb_pairs", "welded_vertices",
                    "joints", "fileIdentifier"])
        for st, plan, uuid, legs, pairs, nv in rows:
            r = meta.loc[uuid]
            w.writerow([st, plan, uuid, r.category_label, r.license, legs, pairs, nv, r.joint_count, r.fileIdentifier])
    for st in ("same", "fresh"):
        print(st, {g: sum(1 for x in rows if x[0] == st and x[1] == g) for g in taken})


def artist_weights(d):
    n_v, n_j = (int(x) for x in d["skinning_weights_shape"])
    W = np.zeros((n_v, n_j))
    np.add.at(W, (d["skinning_weights_row"].astype(int), d["skinning_weights_col"].astype(int)),
              d["skinning_weights_value"].astype(float))
    s = W.sum(1, keepdims=True)
    np.divide(W, s, out=W, where=s > 0)
    return W


def write_gltf(d, W, path):
    import pygltflib as g

    V = d["vertices"].astype(np.float32)
    F = d["faces"].astype(np.uint32)
    J = d["joints"].astype(np.float64)
    parent, children = tree(d)
    names, seen = [], set()
    for k, name in enumerate(str(x) for x in d["joint_names"]):
        names.append(name if name not in seen else f"{name}_{k}")
        seen.add(names[-1])

    top = np.argsort(-W, axis=1)[:, :4]
    jw = np.take_along_axis(W, top, axis=1)
    s = jw.sum(1, keepdims=True)
    np.divide(jw, s, out=jw, where=s > 0)
    joints4 = top.astype(np.uint16)
    weights4 = jw.astype(np.float32)
    ibm = np.zeros((len(J), 4, 4), dtype=np.float32)
    for j in range(len(J)):
        m = np.eye(4)
        m[:3, 3] = -J[j]
        ibm[j] = m.T  # glTF stores matrices column-major

    blobs = [V.tobytes(), F.tobytes(), joints4.tobytes(), weights4.tobytes(), ibm.tobytes()]
    offsets, buf = [], b""
    for b in blobs:
        buf += b"\0" * (-len(buf) % 4)
        offsets.append(len(buf))
        buf += b
    views = [g.BufferView(buffer=0, byteOffset=o, byteLength=len(b)) for o, b in zip(offsets, blobs)]
    acc = [
        g.Accessor(bufferView=0, componentType=g.FLOAT, count=len(V), type=g.VEC3,
                   min=V.min(0).tolist(), max=V.max(0).tolist()),
        g.Accessor(bufferView=1, componentType=g.UNSIGNED_INT, count=F.size, type=g.SCALAR),
        g.Accessor(bufferView=2, componentType=g.UNSIGNED_SHORT, count=len(V), type=g.VEC4),
        g.Accessor(bufferView=3, componentType=g.FLOAT, count=len(V), type=g.VEC4),
        g.Accessor(bufferView=4, componentType=g.FLOAT, count=len(J), type=g.MAT4),
    ]
    nodes = []
    for j in range(len(J)):
        p = parent[j]
        t = J[j] - (J[p] if p >= 0 else 0.0)
        nodes.append(g.Node(name=names[j], translation=[float(x) for x in t], children=children[j] or None))
    mesh_node = len(nodes)
    nodes.append(g.Node(name="body", mesh=0, skin=0))
    root = int(d["root_index"])
    gltf = g.GLTF2(
        scene=0,
        scenes=[g.Scene(nodes=[root, mesh_node])],
        nodes=nodes,
        meshes=[g.Mesh(primitives=[g.Primitive(attributes=g.Attributes(POSITION=0, JOINTS_0=2, WEIGHTS_0=3),
                                               indices=1)])],
        skins=[g.Skin(joints=list(range(len(J))), inverseBindMatrices=4, skeleton=root)],
        accessors=acc,
        bufferViews=views,
        buffers=[g.Buffer(byteLength=len(buf))],
    )
    gltf.set_binary_blob(buf)
    gltf.save_binary(path)


def weld(d, W):
    V = d["vertices"]
    _, first, inverse = np.unique(V, axis=0, return_index=True, return_inverse=True)
    inverse = inverse.reshape(-1)
    order = np.argsort(first)  # keep the original order of first occurrences
    rank = np.empty_like(order)
    rank[order] = np.arange(len(order))
    new = rank[inverse]
    F = new[d["faces"]]
    F = F[(F[:, 0] != F[:, 1]) & (F[:, 1] != F[:, 2]) & (F[:, 0] != F[:, 2])]
    Wn = np.zeros((len(order), W.shape[1]))
    np.add.at(Wn, new, W)
    count = np.bincount(new)
    Wn /= count[:, None]
    spread = float(np.abs(W - Wn[new]).sum(1).max())
    d = dict(d)
    d["vertices"] = V[first[order]]
    d["faces"] = F
    return d, Wn, spread


def convert(data, listing, out, welded=False):
    split = load_split(data)
    with open(listing) as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        d = split[r["uuid"]]
        folder = os.path.join(out, r["uuid"])
        os.makedirs(folder, exist_ok=True)
        W = artist_weights(d)
        if welded:
            before = (len(d["vertices"]), len(d["faces"]))
            d, W, spread = weld(d, W)
            print(f"  welded {before[0]} -> {len(d['vertices'])} vertices, {before[1]} -> {len(d['faces'])} faces, "
                  f"largest L1 between merged vertices' weights {spread:.3g}")
        write_gltf(d, W, os.path.join(folder, "body.glb"))
        np.savetxt(os.path.join(folder, "artist.csv"), W, delimiter=",", fmt="%.9g")
        print(f"{r['group']:9s} {r['uuid']}  {len(d['vertices'])} vertices, {W.shape[1]} joints, "
              f"up to {int((W > 0).sum(1).max())} influences per vertex")


def main(argv):
    if len(argv) == 3 and argv[0] == "select":
        select(argv[1], argv[2])
    elif len(argv) == 4 and argv[0] == "select-topology":
        select_topology(argv[1], argv[2], argv[3])
    elif len(argv) in (4, 5) and argv[0] == "convert" and argv[4:] in ([], ["--weld"]):
        convert(argv[1], argv[2], argv[3], welded=argv[4:] == ["--weld"])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
