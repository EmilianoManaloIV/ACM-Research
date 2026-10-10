# Experiment: synthetic body v2, every method re-run under both skinning models

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** [Abdrashitov et al. 2023](../../papers/2023-abdrashitov-robust-skin-weights-transfer.md)

## Question and hypothesis

The skinning-model experiment found the synthetic bodies hard-skinned at
every joint: `trimesh` capsules carry no rings along their length, so the
3 to 4 cm weight ramp in `chain_weights` touched no vertices, and the ramp
itself ran 0.5 to 1 over one side of the joint rather than 0 to 1 across it.
Every table in the log so far was measured against bodies whose joints do
not blend. Which of the earlier conclusions survive on a body that does?

## Method

`tools/make_synthetic.py`: `capsule` now builds its own rings (spacing half
the radius, caps included; no `trimesh` dependency), and `chain_weights`
ramps 0 to 1 over `2 * falloff` centred on each joint that has a bone on
both sides. The arm body goes from 870 to 1230 vertices, the elbow ring
is 0.5/0.5 with 0.96/0.04 and 0.04/0.96 one ring either side, and the
posed body now differs between LBS and DQS on bend poses (max 3.1 cm at
elbow 135, against 0 before). The button on the cuff is also denser (74
vertices, 44% of the cuff, against 34 before), which matters for its
penetration figure.

Data regenerated, then `tools/run_benchmark.sh data/results all lbs` and
`tools/run_benchmark.sh data/results_dqs all dqs`. Garments and poses are
unchanged.

## Results

Worst test pose per garment, LBS → DQS, body v2. Earlier experiments'
tables are body v1 and are not directly comparable.

| Garment | Method | Matched | Penetration LBS → DQS (% verts) | Max depth LBS → DQS (% body) | Stretch p95 LBS → DQS | Drift LBS → DQS | Influences |
| --- | --- | --- | --- | --- | --- | --- | --- |
| cuff | anchored | 100% | 46.1 → 32.0 | 3.29 → 5.46 | 1.47 → 2.29 | 2.66% → 1.88% | 2 |
| cuff | closest | 100% | 46.1 → 32.0 | 3.29 → 5.46 | 1.47 → 2.29 | 2.66% → 1.88% | 2 |
| cuff | consistent | 57% | 46.1 → 46.1 | 3.19 → 2.95 | 0.73 → 0.61 | 2.71% → 1.96% | 2 |
| cuff | inpaint | 54% | 46.1 → 46.1 | 6.60 → 5.69 | 1.00 → 0.58 | 2.05% → 4.71% | 2 |
| dress | anchored | 8% | 6.9 → 5.5 | 8.72 → 7.36 | 0.61 → 0.87 | 4.17% → 5.06% | 3 |
| dress | closest | 100% | 2.1 → 2.1 | 4.67 → 4.67 | 1.21 → 1.21 | 0.73% → 0.63% | 2 |
| dress | consistent | 27% | 6.5 → 4.3 | 7.07 → 8.70 | 1.36 → 1.94 | 2.62% → 2.75% | 7 |
| dress | inpaint | 8% | 7.6 → 3.4 | 8.03 → 9.26 | 0.65 → 1.04 | 4.45% → 4.91% | 3 |
| skirt | anchored | 5% | 9.8 → 7.8 | 5.27 → 6.66 | 0.31 → 0.25 | 2.94% → 2.93% | 3 |
| skirt | closest | 100% | 4.7 → 4.7 | 5.01 → 5.02 | 1.23 → 1.33 | 0.79% → 0.67% | 2 |
| skirt | consistent | 25% | 5.1 → 4.7 | 5.01 → 7.32 | 0.50 → 0.71 | 2.03% → 1.76% | 5 |
| skirt | inpaint | 5% | 9.8 → 7.8 | 5.27 → 6.64 | 0.31 → 0.25 | 2.93% → 2.93% | 3 |
| sleeve_loose | anchored | 12% | 2.7 → 2.7 | 5.65 → 5.65 | 0.00 → 0.00 | 3.62% → 3.32% | 1 |
| sleeve_loose | closest | 100% | 6.6 → 6.2 | 6.33 → 6.36 | 0.63 → 0.93 | 0.47% → 0.36% | 2 |
| sleeve_loose | consistent | 100% | 6.6 → 6.2 | 6.33 → 6.36 | 0.63 → 0.93 | 0.47% → 0.36% | 2 |
| sleeve_loose | inpaint | 12% | 2.7 → 2.7 | 5.65 → 5.65 | 0.00 → 0.00 | 3.62% → 3.32% | 1 |
| sleeve_tight | anchored | 100% | 10.7 → 10.4 | 3.53 → 3.42 | 0.38 → 0.54 | 0.05% → 0.04% | 2 |
| sleeve_tight | closest | 100% | 10.7 → 10.4 | 3.53 → 3.42 | 0.38 → 0.54 | 0.05% → 0.04% | 2 |
| sleeve_tight | consistent | 100% | 10.7 → 10.4 | 3.53 → 3.42 | 0.38 → 0.54 | 0.05% → 0.04% | 2 |
| sleeve_tight | inpaint | 100% | 10.7 → 10.4 | 3.53 → 3.42 | 0.38 → 0.54 | 0.05% → 0.04% | 2 |
| trousers | anchored | 82% | 9.5 → 9.6 | 2.39 → 2.39 | 0.48 → 0.56 | 0.14% → 0.08% | 3 |
| trousers | closest | 100% | 9.6 → 9.6 | 3.57 → 3.57 | 0.58 → 0.74 | 0.08% → 0.05% | 2 |
| trousers | consistent | 86% | 9.5 → 9.6 | 2.39 → 2.39 | 0.52 → 0.58 | 0.11% → 0.08% | 2 |
| trousers | inpaint | 82% | 9.5 → 9.6 | 2.39 → 2.39 | 0.48 → 0.56 | 0.14% → 0.08% | 3 |

What changed against body v1 (LBS):

- **Skirt**: the `consistent` call stands. Penetration 5.1% at stretch 0.50,
  against closest point 4.7% / 1.23 and anchored 9.8% / 0.31. Both baselines
  improved with the softer pelvis, the balance between them did not move.
- **Dress**: the call flips. Closest point now beats `consistent` on both
  metrics (2.1% / 1.21 against 6.5% / 1.36). `consistent`'s penetration is in
  the stride poses (6.5% at stride 60), where the inpainted strip between the
  legs blends toward whichever leg swings back and the other pushes through.
  Raising the margin to 3 gets stretch to 0.90 at 7.0%; it does not beat
  closest point. For a long dress the right answer is still not a per-vertex
  weight.
- **Loose sleeve**: `consistent` and closest point are now the same result
  (100% matched: the denser body puts every vertex inside the relative
  threshold). The sleeve copies the body's narrow ±3 cm elbow ramp, and the
  6.6% penetration at elbow 135 is 34 vertices on the inside of the bend
  with 0.4/0.6 weights, which is LBS collapse of a *narrow* blend. The v1
  sleeve had a wide blend only because it sampled two hard-skinned caps.
- **Cuff**: 46% penetration is the denser button again, nothing else.
- **Tight sleeve and trousers**: same picture, slightly different numbers.

The offset-scaled blend (M7), re-tested on the new sleeve since its premise
is now true:

| Skinning | offsetBlend | Penetration (% verts) | Stretch p95 |
| --- | --- | --- | --- |
| LBS | off (`consistent`) | 6.6 | 0.63 |
| LBS | 1 | 7.2 | 0.53 |
| LBS | 2 | 7.4 | 0.53 |
| LBS | 3 | 7.0 | 0.56 |
| DQS | off (`consistent`) | 6.2 | 0.93 |
| DQS | 1 | 6.8 | 0.84 |
| DQS | 2 | 5.7 | 0.86 |
| DQS | 3 | **2.5** | **0.79** |

Under LBS it trades a little penetration for stretch, as M7 hoped. Under
DQS at scale 3 it improves both metrics on the loose sleeve, the first
result that does. Since the body's own blend is now narrow, "widen with the
offset" and "DQS punishes wide blends" (previous experiment) are no longer in
conflict: the sleeve's blend was being compared with a v1 blend that had no
counterpart on the body.

## Conclusions and next steps

- **Kept**: body v2 is the benchmark from here; `data/README.md` says so.
  Earlier experiment tables are marked as body v1 in the log.
- The default method stays `consistent`: it still wins the skirt, ties on
  the sleeves and trousers, and is now second on the dress, where it was
  already judged the open problem.
- Next: the offset-scaled blend under DQS across every garment, with the
  skinning model as an input to the transfer (M8). If it holds up on the
  skirt and dress, `offsetBlend` turns on by default for DQS targets.
- Then the real body and garments, which this experiment makes more urgent,
  not less: two body revisions changed two of five conclusions.
