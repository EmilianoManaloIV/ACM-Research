# Experiment: skinning model, linear blend against dual quaternion

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** none in `papers/` yet (Kavan et al. 2008, "Geometric skinning with approximate dual quaternion blending", is the method)

## Question and hypothesis

M7 concluded that the loose sleeve's remaining elbow stretch is linear blend
skinning collapsing the 50/50 vertices toward the joint, not a weights
problem. If that is right, scoring the same weights under dual quaternion
skinning (which rotates each vertex about the joint instead of averaging
positions) should remove the compression and drop the sleeve's stretch,
without changing the single-influence garments. The benchmark needs a
skinning mode either way, since the engine that consumes these weights
decides which model applies.

## Method

`rig.cpp` gains `dualQuaternionSkin` (Kavan et al.: blend unit dual
quaternions, sign-corrected against the heaviest joint, normalise, apply)
and `skin(..., SkinningMode)`. `weightbench --skinning lbs|dqs` poses body
and garment with the chosen model; `tools/run_benchmark.sh out methods dqs`
passes it through. `tests/skinning_test.cpp` (CTest) checks a 50/50 vertex
on a 90 degree bend: LBS lands at half radius, DQS at full radius rotated
45 degrees, and single-influence vertices agree.

Full benchmark on the same data and test poses, once per model, weights
unchanged (the transfer does not know the skinning model).

## Results

Worst test pose per garment, LBS → DQS:

| Garment | Method | Penetration LBS → DQS (% verts) | Max depth LBS → DQS (% body) | Stretch p95 LBS → DQS | Drift LBS → DQS |
| --- | --- | --- | --- | --- | --- |
| cuff | anchored | 26.2 → 21.5 | 3.24 → 1.56 | 0.52 → 0.44 | 1.74% → 1.79% |
| cuff | closest | 26.2 → 21.5 | 3.24 → 1.56 | 0.52 → 0.44 | 1.74% → 1.79% |
| cuff | consistent | 26.2 → 21.5 | 3.32 → 1.64 | 0.58 → 0.41 | 1.73% → 1.79% |
| dress | anchored | 9.2 → 5.7 | 9.72 → 7.34 | 0.58 → 0.97 | 4.14% → 4.87% |
| dress | closest | 5.3 → 3.4 | 7.34 → 7.34 | 1.28 → 1.46 | 1.16% → 1.15% |
| dress | consistent | 7.7 → 3.8 | 7.34 → 7.34 | 1.22 → 1.82 | 2.70% → 2.96% |
| skirt | anchored | 16.4 → 6.4 | 6.40 → 4.06 | 0.52 → 0.57 | 2.41% → 2.17% |
| skirt | closest | 8.0 → 6.4 | 4.06 → 4.06 | 1.54 → 1.75 | 1.29% → 1.22% |
| skirt | consistent | 7.8 → 6.4 | 4.06 → 4.06 | 0.59 → 0.94 | 1.91% → 1.66% |
| sleeve_loose | anchored | 1.6 → 1.6 | 3.84 → 3.84 | 0.00 → 0.00 | 5.37% → 5.37% |
| sleeve_loose | closest | 2.9 → 5.1 | 4.28 → 3.56 | 0.58 → 0.83 | 1.44% → 1.66% |
| sleeve_loose | consistent | 2.9 → 4.1 | 3.01 → 1.74 | 0.54 → 0.80 | 1.54% → 1.83% |
| sleeve_tight | anchored | 9.2 → 9.2 | 3.96 → 3.94 | 0.60 → 0.53 | 1.48% → 1.46% |
| sleeve_tight | closest | 9.2 → 9.2 | 3.96 → 3.94 | 0.60 → 0.53 | 1.47% → 1.46% |
| sleeve_tight | consistent | 9.2 → 9.2 | 3.96 → 3.94 | 0.60 → 0.53 | 1.48% → 1.46% |
| trousers | anchored | 12.6 → 12.2 | 13.72 → 15.21 | 0.47 → 0.35 | 0.64% → 0.63% |
| trousers | closest | 12.9 → 12.6 | 10.99 → 12.17 | 0.54 → 0.55 | 0.59% → 0.60% |
| trousers | consistent | 12.5 → 12.2 | 13.72 → 15.21 | 0.47 → 0.35 | 0.63% → 0.63% |


- **The hypothesis was wrong for the loose sleeve.** Stretch rose from 0.54
  to 0.80 (elbow 135: max 0.86 to 1.16) and penetration from 2.9% to 4.1%.
  The copied blend spans 0.23 to 0.51 along a 0.56 arm, and under DQS each
  ring rotates about the elbow by its own share of the angle: rings 2 cm
  apart at 12 cm radius turn 10 to 15 degrees relative to each other, which
  is 2 to 3 cm of extension on the outside of the bend. LBS hid that by
  collapsing. A wide blend is right for LBS and wrong for DQS; the weights
  have to be made for the model that will skin them.
- **Everything with a narrow blend improved.** Tight sleeve stretch 0.60 to
  0.53, trousers 0.47 to 0.35, cuff 0.58 to 0.41 with penetration 26% to
  22%. The skirt and dress gained on penetration (7.8% to 6.4%, 7.7% to 3.8%)
  but lost on stretch (0.59 to 0.94, 1.22 to 1.82), the same wide-blend
  effect in the panel between the legs.
- **The anchored method's skirt result changed most**: penetration 16.4% to
  6.4% at unchanged stretch, now the best skirt result under either model.
  The ranking of methods differs per model, so the log's keep-or-drop calls
  so far are LBS calls.
- **Data flaw found while checking this.** The LBS and DQS body meshes are
  identical on every bend pose (max difference 0.0; 0.009 on the twist). The
  synthetic bodies are hard-skinned at the joints: `trimesh` capsules have
  only two rings along the cylinder, so `chain_weights`' 3 cm ramp hits no
  vertices at the elbow (the 60 fractional vertices are all at x 0.50 to
  0.52, on the wrist cap). Every garment blend so far came from the garment
  sampling closest points on the two capsule caps around the joint, plus
  smoothing. The numbers across experiments are still comparable with each
  other, but the body is not a model of a skinned body, and the penetration
  metric at bent joints is measuring two intersecting caps.

## Conclusions and next steps

- **Kept**: the benchmark scores under both models from now on
  (`run_benchmark.sh` takes `lbs` or `dqs`), and an experiment's table says
  which. The default stays `lbs` so earlier tables remain comparable.
- The weight-side target changes: under DQS the transfer should *narrow*
  the blend on loose garments, not widen it, and the best profile per model
  can be measured rather than guessed. A method that takes the skinning
  model as input is a real candidate (M8).
- First, fix the body: give the synthetic capsules rings along their length
  and a real blend at each joint (`chain_weights`' ramp is also one-sided,
  running 0.5 to 1 over the last `falloff` of a bone instead of 0 to 1
  centred on the joint). Re-run the whole log under both models on the fixed
  body before M8, since several earlier conclusions were drawn on a
  hard-skinned body.
