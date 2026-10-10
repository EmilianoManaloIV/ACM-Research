# Experiment: weight transfer baselines (M0, M1)

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** [Abdrashitov et al. 2023](../../papers/2023-abdrashitov-robust-skin-weights-transfer.md)

## Question and hypothesis

Set up the benchmark for automatic body-to-garment skin weight transfer and
establish the two baselines every later method must beat: plain closest point
(M0, what most tools do) and Epic's weight inpainting (M1). Expected: M1 is
smoother and tears less than M0, but struggles on loose garments and fails on
garments with detached pieces.

## Method

- **Code:** `src/acm/skin/` (rig loading, linear blend skinning, transfer
  methods, metrics) and `apps/weightbench` (runs a method on a garment and
  scores it over a pose set). M1 is a line-for-line port of Epic's released
  Python; `tools/verify_inpaint.py` checks it on their sphere-to-plane example
  and the two agree to 1.2e-13.
- **Data:** `tools/make_synthetic.py` writes two capsule bodies with hand-set
  weights and six garments into `data/synthetic/`: an arm (tight sleeve, loose
  sleeve, a wrist cuff with a detached button) and a pair of legs (trousers
  with a waistband, a skirt, a long dress). Each body has a `train` pose set
  (unused here) and a held-out `test` set of 7 to 8 poses.
- **Run:** `tools/run_benchmark.sh data/results` then
  `python3 tools/summarize_results.py data/results/results.csv`.
- **Metrics** (per test pose, worst pose reported): share of garment vertices
  inside the body, deepest penetration as a share of the body's bounding-box
  diagonal, 95th percentile of edge stretch, and mean offset drift (how much
  the garment-to-body distance changes from rest). Lower is better everywhere.
- **Settings:** Epic's defaults, 5% distance and 30 degree normal thresholds,
  10 smoothing passes at 0.2. No influence pruning.

## Results

| Garment | Method | Matched | Worst penetration (% verts) | Max depth (% body) | Stretch p95 (worst pose) | Offset drift (mean) | Influences | Time (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
<!-- table from the first run; later runs add a Valid column -->
| cuff | closest | 100% | 26.2 | 3.24 | 0.52 | 1.74% | 2 | 0.001 |
| cuff | inpaint | | failed: singular inpainting system | | | | | |
| dress | closest | 100% | 5.3 | 7.34 | 1.28 | 1.16% | 2 | 0.004 |
| dress | inpaint | 7% | 9.2 | 9.69 | 0.58 | 4.14% | 2 | 0.009 |
| skirt | closest | 100% | 8.0 | 4.06 | 1.54 | 1.29% | 2 | 0.003 |
| skirt | inpaint | 2% | 16.4 | 6.43 | 0.51 | 2.42% | 2 | 0.006 |
| sleeve_loose | closest | 100% | 2.9 | 4.28 | 0.58 | 1.44% | 2 | 0.002 |
| sleeve_loose | inpaint | 12% | 1.6 | 3.84 | 0.00 | 5.37% | 1 | 0.004 |
| sleeve_tight | closest | 100% | 9.2 | 3.96 | 0.60 | 1.47% | 2 | 0.002 |
| sleeve_tight | inpaint | 94% | 9.2 | 3.96 | 0.60 | 1.48% | 3 | 0.002 |
| trousers | closest | 100% | 12.9 | 10.99 | 0.54 | 0.59% | 2 | 0.005 |
| trousers | inpaint | 75% | 12.6 | 13.72 | 0.47 | 0.64% | 3 | 0.006 |

Observations:

- **Tight sleeve:** the two methods agree, as they should; 94% of vertices
  match directly. The 9% penetration is the inner elbow folding into the arm
  at 135 degrees, which no weight assignment on a tube can avoid.
- **Loose sleeve:** only the shoulder end matches, so inpainting fills the whole
  sleeve with the shoulder bone (one influence, zero stretch). The sleeve then
  stays straight while the arm bends: offset drift jumps to 5.4%, the highest
  in the table. Closest point bends the sleeve but tears at the elbow.
- **Skirt and dress:** closest point tears between the legs (stretch p95 of
  1.5 and 1.3, the wrong-leg case). Inpainting matches only 2% and 7% of
  vertices and gives smooth weights, but twice the penetration. Neither is
  acceptable; this is where a better method has the most room.
- **Trousers:** both are similar; the waistband (no matches) is inpainted from
  the legs. Inpainted weights go to -0.013 and 1.006, outside [0, 1].
- **Cuff with a detached button:** Epic's method fails outright, because the
  button's component has no matched vertex and the system is singular.
  Closest point gives the button the elbow's weights and it penetrates badly.

## Conclusions and next steps

- The benchmark and metrics behave as intended: they separate the methods and
  surface the failure modes the plan predicted (loose regions locked to one
  bone, out-of-range weights, singular systems on detached pieces, tearing
  between the legs).
- M1 is the baseline to beat on smoothness; M0 is the baseline to beat on
  drift. A method that wins must keep inpainting's smoothness while letting
  loose regions follow the nearest limb.
- Next: M2 (bounded solve) is a small change; M5 (anchor every component)
  fixes the hard failure; M3/M4 (adaptive matching, consistency check) target
  the skirt. Also add SkinCells as a second baseline once its details are
  available, and bring in a real body and garments.
