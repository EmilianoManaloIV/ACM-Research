# Protocol: automatic body-weighting baselines across body plans

Written before any run, for Gate C. Frozen once the director approves it; later changes go under Deviations.

- **Question:** [`research/questions/RQ-002-automatic-body-weights.md`](../../research/questions/RQ-002-automatic-body-weights.md)
- **Evidence:** [`research/evidence/RQ-002.md`](../../research/evidence/RQ-002.md)
- **Kind:** confirmatory for H1 and H2 below; no method of ours is tested yet
- **Gate C approved:** 2026-10-10, Emiliano

## Why this experiment first

No independent comparison of the established automatic methods on non-human body plans was found at Gate B, and the learned methods report agreement with artist weights rather than deformation quality. This experiment measures the methods that can run here today, so every later method has a fixed reference. Learned models are the second experiment (see "What comes next").

## Hypotheses

- **H1 (robustness):** bounded biharmonic weights fail or degrade more than bone heat on multi-shell, non-manifold creature meshes, because BBW needs a clean tetrahedral mesh (claim from SkinCells and the cut-cell preprint, not yet checked by us).
- **H2 (metric disagreement):** ranking the methods by agreement with artist weights gives a different order than ranking them by deformation quality, for at least one body plan.
- **Null:** both methods rank the same on every metric and body plan, and neither fails more often.

## Methods

Each runs once per character at its published defaults. Nothing is tuned.

| ID | Method | Implementation | Notes |
|----|--------|----------------|-------|
| B0 | Inverse distance to bones (floor) | Ours, in `src/acm/skin/` | Shows how much the real methods add over the simplest rule |
| B1 | Bone heat (Baran and Popović 2007) | Blender 4.0.2 (Ubuntu package), "With Automatic Weights", run headless from a script | The default most artists get |
| B2 | Bounded biharmonic weights (Jacobson et al. 2011) | libigl `bbw`, on a TetGen tetrahedralisation of the mesh | libigl's TetGen module is AGPL; fine for research use, noted for later |
| Ref | Artist weights | Shipped with the character | Reference only, not ground truth (Gate A decision) |

Geodesic voxel binding is left out: there is no open implementation, and Maya cannot run here.

## Data and splits

- **Human:** the MakeHuman base mesh (CC0, pinned by SHA-256) with its artist weights. A second human comes from the dataset below.
- **Quadrupeds and creatures:** Articulation-XL2.0, kept to objects whose own licence is CC0 or CC-BY, with artist weights, 1,000 to 100,000 vertices and 10 to 150 joints. From those, the first 5 humanoids, 5 quadrupeds and 5 other creatures by the dataset's category label, in ascending uuid order. The list is committed before any run.
- **Fallback if Hugging Face stays blocked:** procedural characters from a new generator in `tools/` (a quadruped with a tail, a winged biped, a six-legged creature), built on the capsule generator. These have no artist weights, so H2 would be tested on MakeHuman alone. Using the fallback is a deviation that needs approval.
- **Poses:** per character, 20 test poses with each joint rotated uniformly within ±45° of rest around random axes, from seeds 0 to 19, plus 5 named poses (bend each limb chain fully). There is no training set because nothing is tuned.

## Metrics

Per pose, as approved at Gate A:

- **Primary:** stretch p95 (per edge, |posed length / rest length − 1|) and the self-intersection fraction (share of faces intersecting a non-adjacent face, minus the share already intersecting at rest).
- **Secondary:** mean stretch, torn fraction (edges stretched over 100%), worst pose of each.
- **Robustness:** whether the method produced valid weights at all (finite, non-negative, rows summing to 1 within 1e-6), and its run time.
- **Reference, reported separately:** against artist weights, the L1 weight difference per vertex, precision and recall of influences above 1e-4, and mean vertex distance from the artist-weighted pose as a share of the bounding-box diagonal.

Each metric is reported per pose, then as the mean and the worst pose per character, then as the median and range across the characters in each body plan. With 5 characters per plan these are descriptions, not significance tests.

## Decision rules

- **H1 holds** if BBW's failure count across the 15 dataset characters exceeds bone heat's, or BBW's median stretch p95 is worse by more than 5% on the creature group but not on the humans.
- **H2 holds** if, in any body plan, the order of B0, B1 and B2 by mean L1 against artist weights differs from their order by mean stretch p95.
- A method that fails on a character is recorded as a failure for that character, and is not repaired by mesh clean-up unless the same clean-up is applied to every method beforehand and logged as a deviation.

## Stopping rule

One run per method per character. No re-runs with changed settings. If more than a third of the characters cannot be loaded at all, stop and return to the director.

## Code needed before the runs (Gate D)

- Load a skeleton without weights and compute weights for the body itself (a body-weighting mode in `weightbench`).
- The B0 floor method and the self-intersection metric, each with a unit test.
- A headless Blender script for B1, and libigl BBW for B2.
- A seeded pose generator and a dataset fetcher with the licence and size filters.
- Every run wrapped in `tools/record_run.sh`; the results table committed as `results.csv`.

## What comes next

The second experiment adds the released learned models under the same protocol: UniRig (MIT), SkinTokens (MIT), MagicArticulate (Apache-2.0), Make-It-Animatable (MIT, humanoid-focused) and RigNet (GPLv3). The code licences allow research use. Their trained weights are hosted on Hugging Face, and their licences have not been checked. Inference needs a CUDA GPU with at least 8 GB of memory (UniRig's README), and this cloud environment has none.

## Known risks and threats to validity

- Articulation-XL2.0's artist weights vary in quality; a bad reference inflates the L1 of good methods. Reported separately for that reason.
- Random poses can include anatomically impossible ones that penalise every method alike. The named poses guard against judging only on those.
- Blender 4.0.2's bone heat may differ from the paper's method in detail; results are about the tool artists use, not the paper.
- With 5 characters per body plan, one odd character can move a median.

## Deviations

| Date | Change | Reason | Approved by |
|------|--------|--------|-------------|
| 2026-10-10 | Body weighting is a separate program, `bodybench`, not a mode of `weightbench`. | It scores a body against itself, with different inputs and columns from the garment benchmark; sharing one program would make both harder to read. | pending (Gate D) |
| 2026-10-10 | Weights from every method are row-normalised before the validity check: negatives above -1e-9 set to zero, then each row divided by its sum. A row that sums to zero stays zero and counts as a failure (`unweighted`). | Blender and libigl both normalise at deform time, so un-normalised output is not what an artist would see. Unweighted vertices are the failure that matters and are still caught. | pending (Gate D) |
| 2026-10-10 | Random poses leave root joints at rest. | Turning the root moves the whole body rigidly and changes no metric. | pending (Gate D) |
| 2026-10-10 | "Bend each limb chain fully" made concrete: the 5 longest unbranched chains (a chain runs from a child of a branching joint or root down to the next branching joint or leaf), every joint turned 45 degrees about an axis perpendicular to its bone. | "Fully" had no number. 45 degrees per joint matches the random-pose limit; on the MakeHuman body this picks both legs to the foot, both arms to the wrist, and the lower spine. | pending (Gate D) |
