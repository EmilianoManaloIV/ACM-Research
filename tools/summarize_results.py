#!/usr/bin/env python3
"""Turns a weightbench results.csv into a Markdown table, one row per garment
and method, with the worst pose for each metric.

    python3 tools/summarize_results.py data/results/results.csv
"""

import csv
import sys
from collections import defaultdict


def main(path):
    pose_rows = defaultdict(list)
    weight_rows = {}
    failed = set()
    with open(path) as f:
        for row in csv.DictReader(f):
            key = (row["garment"], row["method"])
            if row["pose"] == "weights":
                weight_rows[key] = row
            elif row["pose"] == "failed":
                failed.add(key)
            else:
                pose_rows[key].append(row)

    print("| Garment | Method | Matched | Worst penetration (% verts) | Max depth (% body) | "
          "Stretch p95 (worst pose) | Torn edges (worst pose) | Offset drift (mean) | Valid | Influences | Time (s) |")
    print("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
    keys = sorted(set(pose_rows) | failed, key=lambda k: (k[0], k[1]))
    for garment, method in keys:
        if (garment, method) in failed:
            print(f"| {garment} | {method} | | failed: singular inpainting system | | | | | | | |")
            continue
        rows = pose_rows[(garment, method)]
        w = weight_rows[(garment, method)]
        pen = max(float(r["penetration_fraction"]) for r in rows) * 100
        depth = max(float(r["penetration_max_depth"]) for r in rows) * 100
        stretch = max(float(r["stretch_p95"]) for r in rows)
        torn = max(float(r.get("torn_fraction") or 0) for r in rows) * 100
        drift = sum(float(r["offset_drift"]) for r in rows) / len(rows) * 100
        # Valid = every weight in [0, 1] and every row summing to 1.
        valid = (float(w["min_weight"]) >= -1e-6 and float(w["max_weight"]) <= 1 + 1e-6
                 and float(w["row_sum_error"]) <= 1e-6)
        print(f"| {garment} | {method} | {float(w['matched_fraction']) * 100:.0f}% | {pen:.1f} | {depth:.2f} | "
              f"{stretch:.2f} | {torn:.1f}% | {drift:.2f}% | {'yes' if valid else 'no'} | {w['max_influences']} | "
              f"{float(w['seconds']):.3f} |")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "data/results/results.csv")
