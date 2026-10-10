# Experiment: M7 revisited, offset-scaled blend on body v2 under both skinning models

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** none

## Question and hypothesis

The offset-scaled blend was dropped on body v1 because the loose sleeve's
blend was already wide (an artefact of hard-skinned caps) and widening it
further increased LBS collapse. On body v2 the sleeve copies the body's
narrow ±3 cm ramp, so the original premise holds: a garment 7 to 12 cm off
the body should blend over a wider region than the skin. Expected: lower
stretch on the loose garments under both models, at some cost in penetration
under LBS (collapse) and in offset drift (a wider blend lags the limb).

## Method

`weightbench --method offsetblend --offset-blend S --skinning M` for S in
2 to 6 and M in lbs, dqs, on every garment, test poses. Baseline is
`consistent` (the same method with the blend off). Cells are
penetration (% verts) / stretch p95 / mean offset drift, worst test pose.

## Results

LBS:

| Garment | consistent | S = 2 | S = 3 | S = 4 | S = 5 | S = 6 |
| --- | --- | --- | --- | --- | --- | --- |
| cuff | 46.1 / 0.73 / 2.7% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% |
| dress | 6.5 / 1.36 / 2.6% | 6.6 / 1.26 / 2.9% | 5.6 / 0.95 / 3.5% | **6.4 / 0.64 / 4.3%** | 4.9 / 0.53 / 4.5% | 5.1 / 0.49 / 4.7% |
| skirt | 5.1 / 0.50 / 2.0% | 5.7 / 0.50 / 2.1% | 6.1 / 0.50 / 2.2% | **4.5 / 0.51 / 2.2%** | 8.2 / 0.54 / 2.8% | 8.0 / 0.52 / 3.0% |
| sleeve_loose | 6.6 / 0.63 / 0.5% | 7.4 / 0.53 / 0.6% | 7.0 / 0.56 / 0.8% | **5.3 / 0.62 / 1.0%** | 7.0 / 0.82 / 1.3% | 8.4 / 0.96 / 1.6% |
| sleeve_tight | 10.7 / 0.38 / 0.1% | 10.7 / 0.38 / 0.1% | 10.7 / 0.36 / 0.1% | **10.7 / 0.37 / 0.1%** | 10.7 / 0.39 / 0.1% | 10.7 / 0.42 / 0.1% |
| trousers | 9.5 / 0.52 / 0.1% | 9.5 / 0.48 / 0.1% | 9.2 / 0.47 / 0.1% | **9.2 / 0.46 / 0.1%** | 9.2 / 0.39 / 0.2% | 9.6 / 0.38 / 0.2% |

DQS:

| Garment | consistent | S = 2 | S = 3 | S = 4 | S = 5 | S = 6 |
| --- | --- | --- | --- | --- | --- | --- |
| cuff | 46.1 / 0.61 / 2.0% | 34.8 / 0.43 / 2.0% | 34.8 / 0.21 / 2.0% | **34.8 / 0.19 / 2.0%** | 34.8 / 0.21 / 2.0% | 34.8 / 0.08 / 2.0% |
| dress | 4.3 / 1.94 / 2.8% | 4.4 / 1.82 / 3.0% | 3.8 / 1.62 / 4.0% | **3.8 / 1.23 / 5.0%** | 4.0 / 0.88 / 5.4% | 4.6 / 0.61 / 5.6% |
| skirt | 4.7 / 0.71 / 1.8% | 4.1 / 0.67 / 1.8% | 4.3 / 0.61 / 2.0% | **4.3 / 0.69 / 2.1%** | 8.2 / 0.57 / 2.7% | 6.1 / 0.42 / 2.9% |
| sleeve_loose | 6.2 / 0.93 / 0.4% | 5.7 / 0.86 / 0.5% | 2.5 / 0.78 / 0.7% | **2.7 / 0.75 / 1.0%** | 6.8 / 0.90 / 1.6% | 6.8 / 1.12 / 2.1% |
| sleeve_tight | 10.4 / 0.54 / 0.0% | 10.4 / 0.54 / 0.0% | 10.0 / 0.50 / 0.1% | **10.0 / 0.52 / 0.1%** | 10.0 / 0.58 / 0.1% | 9.6 / 0.56 / 0.1% |
| trousers | 9.6 / 0.58 / 0.1% | 9.6 / 0.56 / 0.1% | 9.2 / 0.56 / 0.1% | **9.6 / 0.56 / 0.1%** | 9.6 / 0.48 / 0.1% | 9.6 / 0.46 / 0.1% |

- **Scale 4 improves or ties both primary metrics on every garment under
  DQS**, and under LBS on everything but the skirt's stretch (0.50 to 0.51).
  The loose sleeve under DQS goes from 6.2% / 0.93 to 2.7% / 0.75; the cuff
  from 0.61 to 0.19 stretch with a third less penetration (the button now
  blends into the forearm instead of swinging rigidly); the dress's stretch
  halves under LBS (1.36 to 0.64) and drops by a third under DQS.
- **Scale 5 and 6 overshoot.** The skirt's penetration jumps to 8% under
  both models and the loose sleeve's to 7 to 8%: the blend reaches past the
  knee and elbow and the garment lags the limb. The dress keeps improving
  on stretch, which is the long-panel problem again: it wants a blend the
  short garments cannot afford.
- **Drift is the cost**: a wider blend means the garment follows the joint
  rather than the surface under it, and mean offset drift rises from 2.6% to
  4.3% on the dress (LBS) and 2.8% to 5.0% (DQS). That is the metric that
  would show up as a garment floating off the body in a static pose; it is
  secondary in the plan but worth watching when real garments arrive.
- **The v1 conclusion was an artefact.** M7 failed on body v1 because the
  sleeve's blend was already as wide as the capsule caps made it; on a body
  with a real ramp the premise is true and the method works under both
  models.

## Conclusions and next steps

- **Kept, and now the default method**: `offsetblend` with scale 4
  (`TransferParams::offsetBlend`), on top of `consistent`. It is the first
  change since the baselines that improves both primary metrics on every
  garment under one model, and every garment but one under the other.
- The scale should depend on the garment, not be fixed: the dress wants 6,
  the skirt and sleeve want 4, and the tight garments do not care. The
  natural signal is the distance to the body already used as the radius,
  possibly with a cap at the joint spacing so the blend never crosses the
  next joint. That is M8.
- The per-garment optimum differs between LBS and DQS only in degree, so a
  skinning-model input to the transfer is less urgent than it looked after
  the skinning-model experiment; the benchmark reports both and that is
  enough for now.
- Real body and garments remain the next data step.
