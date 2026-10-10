"""Test poses for RQ-002 (experiment 2026-10-10-auto-weight-baselines).

  python3 tools/make_body_poses.py bones.json poses.json

bones.json  written by bodybench --export-bones
poses.json  gets two lists bodybench reads with --pose-set test,named:

test   20 random poses, seeds 0 to 19. Every joint in the limb chains (the
       same 5 chains as the named poses) turns by an angle drawn uniformly
       from -45 to 45 degrees about an axis drawn uniformly from the sphere;
       every other joint stays at rest.
named  one pose per limb chain, for the 5 longest chains: every joint in the
       chain bends 45 degrees about an axis perpendicular to its bone, so the
       chain curls. A chain starts at a child of a branching joint (or of a
       root) and runs down through joints with one child, ending at the
       next joint that branches or at a leaf (a leg ends at the foot, before
       the toes). Chains are ranked by bone length, longest first.

Nothing here reads the mesh or the weights, so every method sees the same
poses. The output depends only on bones.json and the seeds.
"""

import json
import math
import random
import sys

RANDOM_POSES = 20
MAX_DEG = 45.0
NAMED_POSES = 5
BEND_DEG = 45.0


def length(v):
    return math.sqrt(sum(x * x for x in v))


def random_axis(rng):
    while True:
        v = [rng.gauss(0.0, 1.0) for _ in range(3)]
        n = length(v)
        if n > 1e-9:
            return [x / n for x in v]


def bend_axis(local_tail):
    """A unit axis perpendicular to the bone, in the joint's own frame."""
    d = local_tail
    if length(d) < 1e-9:
        return [1.0, 0.0, 0.0]
    # Cross with the frame axis least aligned with the bone.
    k = min(range(3), key=lambda i: abs(d[i]))
    e = [0.0, 0.0, 0.0]
    e[k] = 1.0
    c = [d[1] * e[2] - d[2] * e[1], d[2] * e[0] - d[0] * e[2], d[0] * e[1] - d[1] * e[0]]
    n = length(c)
    return [x / n for x in c]


def chains(bones):
    children = {b["name"]: [] for b in bones}
    for b in bones:
        if b["parent"] is not None:
            children[b["parent"]].append(b["name"])
    bone = {b["name"]: b for b in bones}
    seg = {b["name"]: length([t - h for h, t in zip(b["head"], b["tail"])]) for b in bones}

    starts = [
        b["name"]
        for b in bones
        if b["parent"] is not None and (bone[b["parent"]]["parent"] is None or len(children[b["parent"]]) > 1)
    ]
    found = []
    for s in starts:
        chain = [s]
        while len(children[chain[-1]]) == 1:
            chain.append(children[chain[-1]][0])
        found.append((sum(seg[j] for j in chain), s, chain))
    found.sort(key=lambda x: (-x[0], x[1]))
    return [chain for _, _, chain in found[:NAMED_POSES]]


def main(argv):
    if len(argv) != 2:
        sys.exit(__doc__)
    bones_path, out_path = argv
    with open(bones_path) as f:
        bones = json.load(f)["bones"]

    limbs = chains(bones)
    limb_joints = {j for chain in limbs for j in chain}
    test = []
    for seed in range(RANDOM_POSES):
        rng = random.Random(seed)
        joints = {}
        for b in bones:
            if b["name"] not in limb_joints:
                continue
            axis = random_axis(rng)
            joints[b["name"]] = {"axis": axis, "deg": rng.uniform(-MAX_DEG, MAX_DEG)}
        test.append({"name": f"random{seed:02d}", "seed": seed, "joints": joints})

    named = []
    for chain in limbs:
        local = {b["name"]: b["local_tail"] for b in bones}
        named.append(
            {
                "name": "bend_" + chain[0],
                "joints": {j: {"axis": bend_axis(local[j]), "deg": BEND_DEG} for j in chain},
            }
        )

    with open(out_path, "w") as f:
        json.dump({"test": test, "named": named}, f, indent=1)
    print(f"wrote {out_path}: {len(test)} random poses, {len(named)} named: "
          + ", ".join(f"{p['name']} ({len(p['joints'])} joints)" for p in named))


if __name__ == "__main__":
    main(sys.argv[1:])
