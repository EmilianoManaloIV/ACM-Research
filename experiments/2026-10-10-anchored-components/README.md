# Experiment: M5, anchored components

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** [Abdrashitov et al. 2023](../../papers/2023-abdrashitov-robust-skin-weights-transfer.md)

## Question and hypothesis

Epic's inpainting fails outright on a garment with a detached piece that
matches nothing (the cuff's button): that component's rows of the system are
singular. If every connected component is guaranteed at least one matched
vertex, does the method produce a usable result on such garments, and does
anything else change? Expected: the failure goes away, other garments are
untouched, and the detached piece follows the nearest bone.

## Method

`transfer.cpp` method `anchored`, on top of M2 (`bounded`): after matching,
connected components (`igl::connected_components` on the vertex adjacency)
with no matched vertex get their vertices nearest the body matched with the
thresholds waived, taking every vertex within `anchorSlack` (1.5) times the
component's nearest distance. Then the bounded solve as in M2.

Run: `tools/run_benchmark.sh data/results all`, same data and poses as before.

## Results

Only the cuff changes; every other garment is identical to M2.

| Garment | Method | Matched | Worst penetration (% verts) | Max depth (% body) | Stretch p95 (worst pose) | Offset drift (mean) | Valid | Influences | Time (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| cuff | anchored | 100% | 26.2 | 3.24 | 0.52 | 1.74% | yes | 2 | 0.001 |
| cuff | bounded | 74% | 26.2 | 5.71 | 1.00 | 1.31% | no | 2 | 0.003 |
| cuff | closest | 100% | 26.2 | 3.24 | 0.52 | 1.74% | yes | 2 | 0.002 |
| cuff | inpaint | | failed: singular inpainting system | | | | | | |

- The button (34 vertices, all within 1.5 times its nearest distance) is
  matched whole, so it gets the elbow's weights, the same as closest point.
  The method now returns valid weights where M1 failed and M2 returned zeros.
- Those weights are not good: the button sits 0.13 above the elbow and
  swings into the arm when the elbow bends (26% of the cuff's vertices
  inside, almost all of them the button). Anchoring makes the problem
  solvable; it does not make the answer right.

## Conclusions and next steps

- Kept: it is a strict improvement on garments with detached pieces and a
  no-op elsewhere, and real garments have buttons, belts and patches.
- A detached piece needs weights that keep its offset from the body, not the
  nearest surface point's weights. That is the same need as the loose sleeve
  (follow the limb underneath, not the one bone that happened to match), so
  M3 (adaptive, region-relative matching) is the next experiment, and the
  button is one of its test cases.
