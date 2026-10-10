# Experiment: M3, region-relative matching

- **Date started:** 2026-10-10
- **Status:** done (not adopted)
- **Related papers:** [Abdrashitov et al. 2023](../../papers/2023-abdrashitov-robust-skin-weights-transfer.md)

## Question and hypothesis

Epic's single global distance threshold (5% of the garment's bounding box)
leaves loose regions unmatched, and inpainting then locks them to whichever
bone did match: the loose sleeve ends up rigid on the shoulder with 5.4%
offset drift. If a vertex also counts as matched when it is no further from
the body than 1.5 times the nearest distance in its own neighbourhood, loose
regions that float at a steady offset should match the limb under them.
Expected: drift on the loose sleeve drops to closest-point levels; tight
garments unchanged; the skirt is the risk, since between the legs the nearest
body point is on the wrong leg for half the vertices.

## Method

`transfer.cpp` method `relative`: M5 (anchored, bounded) with
`TransferParams::relativeMatch` on. For each vertex the smallest closest-point
distance within a mesh neighbourhood of radius 5% of the bounding-box
diagonal is found by a flood fill; the vertex matches if its own distance is
within 1.5 times that, or within the global threshold as before. The normal
test is unchanged.

Run: `tools/run_benchmark.sh data/results all`, same data and poses as before.

## Results

| Garment | Method | Matched | Worst penetration (% verts) | Max depth (% body) | Stretch p95 (worst pose) | Offset drift (mean) | Valid | Influences | Time (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| cuff | anchored | 100% | 26.2 | 3.24 | 0.52 | 1.74% | yes | 2 | 0.001 |
| cuff | relative | 77% | 26.2 | 3.32 | 0.58 | 1.73% | yes | 2 | 0.002 |
| dress | anchored | 7% | 9.2 | 9.72 | 0.58 | 4.14% | yes | 2 | 0.109 |
| dress | closest | 100% | 5.3 | 7.34 | 1.28 | 1.16% | yes | 2 | 0.004 |
| dress | relative | 79% | 5.7 | 7.34 | 2.93 | 1.64% | yes | 6 | 0.039 |
| skirt | anchored | 2% | 16.4 | 6.40 | 0.52 | 2.41% | yes | 2 | 0.083 |
| skirt | closest | 100% | 8.0 | 4.06 | 1.54 | 1.29% | yes | 2 | 0.003 |
| skirt | relative | 69% | 8.6 | 4.06 | 1.58 | 1.45% | yes | 5 | 0.030 |
| sleeve_loose | anchored | 12% | 1.6 | 3.84 | 0.00 | 5.37% | yes | 1 | 0.026 |
| sleeve_loose | closest | 100% | 2.9 | 4.28 | 0.58 | 1.44% | yes | 2 | 0.002 |
| sleeve_loose | relative | 100% | 2.9 | 4.28 | 0.58 | 1.44% | yes | 2 | 0.003 |
| sleeve_tight | relative | 94% | 9.2 | 3.96 | 0.60 | 1.48% | yes | 2 | 0.005 |
| trousers | anchored | 75% | 12.6 | 13.72 | 0.47 | 0.64% | yes | 3 | 0.042 |
| trousers | relative | 79% | 12.6 | 13.72 | 0.49 | 0.61% | yes | 3 | 0.042 |

- **Loose sleeve:** as hoped. Every vertex matches, drift falls from 5.4% to
  1.4%, and the sleeve bends with the arm. The result is identical to
  closest point, including its 0.58 stretch at the elbow.
- **Skirt and dress:** as feared, and worse. 69% and 79% of vertices match,
  split between the two legs, and the inpainted strip between the two
  groups is torn both ways: stretch p95 of 1.6 on the skirt and 2.9 on the
  dress, against 0.5 and 0.6 for the anchored method. Six bones per vertex
  on the dress is the solver bridging between opposite legs.
- **Cuff:** the button now matches on its own (77%) rather than through the
  anchor; same result.

## Conclusions and next steps

- **Not adopted.** It wins the loose sleeve but breaks the dress by more than
  the 5% rule allows on a primary metric. The code stays behind
  `TransferParams::relativeMatch` for the next experiment.
- The relative threshold is right about *whether* to match; what it exposes
  is that closest point is wrong about *what* to match between the legs.
  M4 (reject matches whose bones disagree with their neighbours, then
  inpaint them) is the missing half, and the two should be tried together.
- Separate observation: the loose sleeve's 0.58 stretch at the elbow is
  closest point copying the body's narrow blend (3 cm) onto a sleeve that
  sits 7 cm out. The blend on the garment should widen with its offset from
  the body. Worth its own experiment later (an "offset-scaled blend").
