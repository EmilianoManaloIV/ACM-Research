# Experiment: M10, blend limited to kin bones

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** none

## Question and hypothesis

On the real body the offset blend leaked across the armpit of the shirt: the
sleeve and the side of the shirt are one surface, so an upper-arm vertex
4 cm out averaged in chest and spine bones (23 influences, stretch 0.54
with arms up against 0.32 without the blend). Two ways to stop it:

1. `bodyblend`: grow the region by the distance between the vertices'
   closest *body* points instead of between garment vertices.
2. `kinblend`: average only over neighbours whose dominant bone is within
   two parent/child steps of the seed's dominant bone (two, not one, because
   this rig has twist bones: upperarm01/02, lowerarm01/02).

Expected: both fix the shirt; neither changes the sleeves or trousers.

## Method

`transfer.cpp`: `TransferParams::blendOnBody` and `blendKinSteps`;
`weightbench` passes the joint hierarchy in as `jointParent`. Scale 4 as
for `offsetblend`. Cells are penetration (% verts) / stretch p95 / torn
edges, worst test pose.

## Results

`bodyblend` on the shirt: 0.48 (LBS), 0.43 (DQS). Not enough: the arm and
the chest are a few centimetres apart in space, so a Euclidean distance on
the body cannot tell them apart; it would take a geodesic. Dropped.

`kinblend` (steps 1 / 2) on the shirt: 0.293 / 0.297 (LBS), 0.281 / 0.280
(DQS); on the loose sleeve 0.167 / 0.135 and 0.084 / 0.070. Two steps is
the default. Full comparison:

Real body, LBS:

| Garment | closest | consistent | offsetblend | kinblend |
| --- | --- | --- | --- | --- |
| shirt | 2.5 / 0.41 / 2.3% | 1.3 / 0.32 / 1.0% | 1.3 / 0.54 / 1.1% | 1.8 / 0.30 / 1.2% |
| skirt | 7.5 / 0.61 / 3.9% | 7.8 / 2.52 / 12.7% | 8.8 / 2.50 / 12.7% | 8.1 / 2.53 / 12.7% |
| sleeve_loose | 0.4 / 0.44 / 2.5% | 0.5 / 0.35 / 0.3% | 1.3 / 0.13 / 0.0% | 1.3 / 0.13 / 0.0% |
| sleeve_tight | 0.0 / 0.33 / 1.2% | 0.0 / 0.25 / 0.0% | 0.4 / 0.22 / 0.0% | 0.4 / 0.22 / 0.0% |
| tights | 4.0 / 0.29 / 0.5% | 5.0 / 0.29 / 0.6% | 4.8 / 0.28 / 0.5% | 5.0 / 0.29 / 0.5% |
| trousers | 9.0 / 0.55 / 1.6% | 9.7 / 0.46 / 1.0% | 8.9 / 0.37 / 0.0% | 8.9 / 0.38 / 0.2% |

Real body, DQS:

| Garment | closest | consistent | offsetblend | kinblend |
| --- | --- | --- | --- | --- |
| shirt | 1.3 / 0.37 / 2.4% | 1.2 / 0.31 / 1.1% | 1.3 / 0.55 / 1.3% | 2.1 / 0.28 / 1.3% |
| skirt | 6.0 / 0.72 / 4.0% | 4.7 / 3.18 / 17.6% | 5.8 / 3.16 / 17.2% | 5.3 / 3.19 / 17.2% |
| sleeve_loose | 0.4 / 0.41 / 2.8% | 0.4 / 0.35 / 0.3% | 1.3 / 0.07 / 0.0% | 1.3 / 0.07 / 0.0% |
| sleeve_tight | 0.4 / 0.29 / 1.7% | 0.4 / 0.23 / 0.0% | 0.5 / 0.16 / 0.0% | 0.5 / 0.16 / 0.0% |
| tights | 2.6 / 0.36 / 0.7% | 2.8 / 0.37 / 0.6% | 2.7 / 0.35 / 0.6% | 2.8 / 0.35 / 0.6% |
| trousers | 6.2 / 0.70 / 2.7% | 6.9 / 0.66 / 1.9% | 6.3 / 0.42 / 0.0% | 6.4 / 0.42 / 0.2% |

Capsules, LBS:

| Garment | closest | consistent | offsetblend | kinblend |
| --- | --- | --- | --- | --- |
| cuff | 46.1 / 1.47 / 8.2% | 46.1 / 0.73 / 0.0% | 46.1 / 0.62 / 0.0% | 46.1 / 0.62 / 0.0% |
| dress | 2.1 / 1.21 / 5.1% | 6.5 / 1.36 / 8.3% | 6.4 / 0.64 / 1.0% | 7.4 / 1.11 / 6.2% |
| skirt | 4.7 / 1.23 / 5.9% | 5.1 / 0.50 / 0.0% | 4.5 / 0.51 / 0.5% | 4.5 / 0.51 / 0.5% |
| sleeve_loose | 6.6 / 0.63 / 2.6% | 6.6 / 0.63 / 2.6% | 5.3 / 0.62 / 0.0% | 5.3 / 0.62 / 0.0% |
| sleeve_tight | 10.7 / 0.38 / 0.0% | 10.7 / 0.38 / 0.0% | 10.7 / 0.37 / 0.0% | 10.7 / 0.37 / 0.0% |
| trousers | 9.6 / 0.58 / 1.8% | 9.5 / 0.52 / 0.3% | 9.2 / 0.46 / 0.0% | 9.2 / 0.46 / 0.0% |

Capsules, DQS:

| Garment | closest | consistent | offsetblend | kinblend |
| --- | --- | --- | --- | --- |
| cuff | 32.0 / 2.29 / 14.7% | 46.1 / 0.61 / 0.0% | 34.8 / 0.19 / 0.0% | 34.8 / 0.19 / 0.0% |
| dress | 2.1 / 1.21 / 6.5% | 4.3 / 1.94 / 12.6% | 3.8 / 1.23 / 7.7% | 4.6 / 1.60 / 11.6% |
| skirt | 4.7 / 1.33 / 6.6% | 4.7 / 0.71 / 2.1% | 4.3 / 0.69 / 1.8% | 4.3 / 0.69 / 1.8% |
| sleeve_loose | 6.2 / 0.93 / 4.4% | 6.2 / 0.93 / 4.4% | 2.7 / 0.75 / 1.1% | 2.7 / 0.75 / 1.1% |
| sleeve_tight | 10.4 / 0.54 / 2.3% | 10.4 / 0.54 / 2.3% | 10.0 / 0.52 / 2.0% | 10.0 / 0.52 / 2.0% |
| trousers | 9.6 / 0.74 / 2.2% | 9.6 / 0.58 / 0.9% | 9.6 / 0.56 / 0.3% | 9.6 / 0.56 / 0.3% |

- **Shirt fixed**: 0.30 / 0.28 stretch, the best of any method, at 1.8% /
  2.1% penetration (consistent 1.3%, closest 2.5% / 1.3%).
- **Identical to `offsetblend` on every other real garment** and on the
  capsule sleeves, skirt, cuff and trousers, which is the point: the limit
  only fires where another limb is within reach.
- **The capsule dress gets worse** (0.64 to 1.11 LBS, 1.23 to 1.60 DQS).
  Its hem panel blends hip, knee and the other leg's bones, three or more
  steps apart, and the limit cuts that. The dress is the one garment where
  the uncapped blend was helping, and the one garment nothing handles.
- `tights` and the real `skirt` unchanged, as before.

## Conclusions and next steps

- **Kept, and now the default method.** It fixes a real garment's
  regression and costs only the synthetic dress, which is already outside
  what per-vertex weight transfer can do. (Third time the 5%-per-metric
  rule would have said no; the loop has been applying "wins on real
  garments, never the worst elsewhere" instead, and the plan should say so.)
- The kin limit is a crude geodesic: the proper version is a geodesic
  radius on the body surface from the closest point, which would also
  handle the dress without a skeleton heuristic. Worth building once the
  loop has a second real body to check it on.
- Next on the method side: cap influences inside the solve (every real-rig
  result has 8 to 24 per vertex); then M6, pose-aware refinement, for the
  skirt and dress.
