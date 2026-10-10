"""Summarises a results.csv from tools/run_auto_weight.sh as Markdown tables.

  python3 tools/summarize_auto_weight.py results.csv

Per character and method: whether the weights were valid, then over the
scored poses the mean and worst stretch p95, mean self-intersection above
rest, mean torn fraction, and mean distance from the artist-weighted pose;
L1 against the artist weights. Then failure counts per group, and the
median per group of each method's mean stretch p95.
"""

import collections
import csv
import statistics
import sys

METHODS = ["artist", "floor", "bone_heat", "bbw"]


def num(rows, col):
    return [float(r[col]) for r in rows if r[col] != ""]


def main(path):
    with open(path) as f:
        rows = list(csv.DictReader(f))
    weights = {(r["character"], r["method"]): r for r in rows if r["pose"] == "weights"}
    poses = collections.defaultdict(list)
    for r in rows:
        if r["pose"] != "weights":
            poses[(r["character"], r["method"])].append(r)
    group = {r["character"]: r["group"] for r in rows}
    chars = sorted(group, key=lambda c: (["humanoid", "quadruped", "creature"].index(group[c]), c))

    print("| group | character | method | valid | L1 vs artist | poses | mean p95 | worst p95 | self-int. | torn | dist. |")
    print("|---|---|---|---|---|---|---|---|---|---|---|")
    for c in chars:
        for m in METHODS:
            w = weights.get((c, m))
            if w is None:
                continue
            p = poses[(c, m)]
            note = w["note"].strip('"')
            valid = "yes" if w["valid"] == "1" else "no" + (f" ({note})" if note else
                                                          f" ({w['unweighted']} unweighted)" if w["unweighted"] not in ("", "0") else "")
            cells = [group[c], c[:8], m, valid, w["l1"] and f"{float(w['l1']):.3f}", str(len(p))]
            if p:
                p95 = num(p, "stretch_p95")
                cells += [f"{statistics.mean(p95):.3f}", f"{max(p95):.3f}",
                          f"{statistics.mean(num(p, 'self_intersection')):.4f}",
                          f"{statistics.mean(num(p, 'torn')):.4f}",
                          f"{statistics.mean(num(p, 'artist_pose_distance')):.4f}" if num(p, "artist_pose_distance") else ""]
            else:
                cells += [""] * 5
            print("| " + " | ".join(cells) + " |")

    print()
    print("| group | characters | " + " | ".join(f"{m} failures" for m in METHODS[1:]) + " | "
          + " | ".join(f"{m} median p95" for m in METHODS) + " |")
    print("|---|---|" + "---|" * (2 * len(METHODS) - 1))
    for g in ["humanoid", "quadruped", "creature"]:
        cs = [c for c in chars if group[c] == g]
        fails = [sum(1 for c in cs if weights.get((c, m), {}).get("valid") != "1") for m in METHODS[1:]]
        med = []
        for m in METHODS:
            means = [statistics.mean(num(poses[(c, m)], "stretch_p95")) for c in cs
                     if poses[(c, m)] and weights[(c, m)]["valid"] == "1"]
            med.append(f"{statistics.median(means):.3f} (n={len(means)})" if means else "none valid")
        print(f"| {g} | {len(cs)} | " + " | ".join(map(str, fails)) + " | " + " | ".join(med) + " |")


if __name__ == "__main__":
    main(sys.argv[1])
