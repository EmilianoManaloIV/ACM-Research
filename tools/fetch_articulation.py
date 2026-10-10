"""Characters for RQ-002 (experiment 2026-10-10-auto-weight-baselines) from
Articulation-XL2.0, filtered and converted for bodybench.

  python3 tools/fetch_articulation.py select DATA LIST.csv
  python3 tools/fetch_articulation.py convert DATA LIST.csv OUT

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


def convert(data, listing, out):
    split = load_split(data)
    with open(listing) as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        d = split[r["uuid"]]
        folder = os.path.join(out, r["uuid"])
        os.makedirs(folder, exist_ok=True)
        W = artist_weights(d)
        write_gltf(d, W, os.path.join(folder, "body.glb"))
        np.savetxt(os.path.join(folder, "artist.csv"), W, delimiter=",", fmt="%.9g")
        print(f"{r['group']:9s} {r['uuid']}  {len(d['vertices'])} vertices, {W.shape[1]} joints, "
              f"up to {int((W > 0).sum(1).max())} influences per vertex")


def main(argv):
    if len(argv) == 3 and argv[0] == "select":
        select(argv[1], argv[2])
    elif len(argv) == 4 and argv[0] == "convert":
        convert(argv[1], argv[2], argv[3])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
