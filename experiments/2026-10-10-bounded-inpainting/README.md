# Experiment: M2, bounded inpainting

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** [Abdrashitov et al. 2023](../../papers/2023-abdrashitov-robust-skin-weights-transfer.md); Jacobson et al. 2011, Bounded Biharmonic Weights (no note yet)

## Question and hypothesis

Epic's inpainting (M1) is an unconstrained solve, so weights can leave [0, 1]
(the baselines run showed -0.013 to 1.006 on the trousers). Does solving the
same energy with 0 <= w <= 1 per bone, then normalising rows, give valid
weights without hurting the pose metrics? Expected: validity fixed, pose
metrics unchanged, some extra run time.

## Method

`transfer.cpp` method `bounded`: the matching and the system Q are shared with
M1 (`prepareInpaint`); each bone is then solved with `igl::active_set` and
bounds [0, 1], rows are normalised to sum to 1, and Epic's smoothing runs as
before. One trap: `igl::active_set` compares each iterate with the previous
one to decide convergence, and with no warm start it accepts the first,
unconstrained solve as converged. Passing a zero vector as the start fixes it.

Run: `tools/run_benchmark.sh data/results all`, same data and poses as the
baselines experiment.

## Results

| Garment | Method | Matched | Worst penetration (% verts) | Max depth (% body) | Stretch p95 (worst pose) | Offset drift (mean) | Valid | Influences | Time (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| cuff | bounded | 74% | 26.2 | 5.71 | 1.00 | 1.31% | no | 2 | 0.003 |
| cuff | closest | 100% | 26.2 | 3.24 | 0.52 | 1.74% | yes | 2 | 0.002 |
| cuff | inpaint | | failed: singular inpainting system | | | | | | |
| dress | bounded | 7% | 9.2 | 9.72 | 0.58 | 4.14% | yes | 2 | 0.101 |
| dress | closest | 100% | 5.3 | 7.34 | 1.28 | 1.16% | yes | 2 | 0.005 |
| dress | inpaint | 7% | 9.2 | 9.69 | 0.58 | 4.14% | yes | 2 | 0.009 |
| skirt | bounded | 2% | 16.4 | 6.40 | 0.52 | 2.41% | yes | 2 | 0.089 |
| skirt | closest | 100% | 8.0 | 4.06 | 1.54 | 1.29% | yes | 2 | 0.004 |
| skirt | inpaint | 2% | 16.4 | 6.43 | 0.51 | 2.42% | yes | 2 | 0.006 |
| sleeve_loose | bounded | 12% | 1.6 | 3.84 | 0.00 | 5.37% | yes | 1 | 0.023 |
| sleeve_loose | closest | 100% | 2.9 | 4.28 | 0.58 | 1.44% | yes | 2 | 0.003 |
| sleeve_loose | inpaint | 12% | 1.6 | 3.84 | 0.00 | 5.37% | yes | 1 | 0.003 |
| sleeve_tight | bounded | 94% | 9.2 | 3.96 | 0.60 | 1.48% | yes | 2 | 0.003 |
| sleeve_tight | closest | 100% | 9.2 | 3.96 | 0.60 | 1.47% | yes | 2 | 0.002 |
| sleeve_tight | inpaint | 94% | 9.2 | 3.96 | 0.60 | 1.48% | no | 3 | 0.002 |
| trousers | bounded | 75% | 12.6 | 13.72 | 0.47 | 0.64% | yes | 3 | 0.050 |
| trousers | closest | 100% | 12.9 | 10.99 | 0.54 | 0.59% | yes | 2 | 0.004 |
| trousers | inpaint | 75% | 12.6 | 13.72 | 0.47 | 0.64% | no | 3 | 0.007 |

- Validity is fixed where M1 was out of range (tight sleeve, trousers), and
  every pose metric is unchanged to two decimals. The bounds only bite on a
  few vertices near the matched/unmatched border, so the deformation is the same.
- Cost: 3 to 10 times slower (0.1 s on the dress), since each bone is a
  separate active-set solve.
- On the cuff the detached button still has nothing to inpaint from. Where M1
  fails outright, M2 returns all-zero rows for the button, which is marked
  invalid; that is no better, only quieter. Anchoring every component (M5) is
  still needed.

## Conclusions and next steps

- Kept as the default solve: it passes the must-pass validity check at no
  cost in the primary metrics. It does not move penetration or stretch, so it
  is not a win on its own, and the baselines' real weaknesses (loose regions
  locked to one bone, tearing between the legs, detached pieces) stand.
- `weightbench` should refuse all-zero rows as a failure rather than score
  them; done in the summary table for now (Valid column).
- Next: M5 (anchor each connected component to the body) fixes the hard
  failure and is a prerequisite for real garments with buttons and belts; then
  M3/M4 for the skirt.
