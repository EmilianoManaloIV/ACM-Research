# Experiment: M6, pose-aware refinement

- **Date started:** 2026-10-10
- **Status:** done (kept as an option, not the default)
- **Related papers:** none in `papers/` (the weight fit is the weight step of Le and Deng's SSDR, 2012)

## Question and hypothesis

Every method so far sets weights from the rest pose alone, and the two
garments that bridge the legs (the capsule dress, the MakeHuman skirt)
defeat all of them. If the garment is skinned into each training pose,
relaxed against the posed body (pushed out of it, edges pulled back toward
rest length) and the weights are then refit so that linear blend skinning
reproduces the relaxed positions, the weights should learn what the rest
pose cannot show: where the panel between the legs has to give. Expected:
lower stretch and penetration on the skirt and dress in the held-out
poses, no change on garments that were already fine.

## Method

`refine.cpp`, `weightbench --refine`: start from any method's weights; for
each training pose (8 on the real body, 5 on the capsules) run 20 rounds of
collision projection (a vertex closer to the body than half its rest
clearance is moved out to that distance along the closest-point direction)
and 4 Jacobi edge-length passes at stiffness 0.5; then per vertex solve
min |A w - t|^2 + mu |w - w0|^2 on the simplex over the vertex's 8 largest
current bones (A stacks the pose transforms applied to the rest vertex,
t the relaxed positions), by 300 projected-gradient steps. Scored on the
test poses as usual. Base method `kinblend`, LBS, the real body at
`--prune 8`. Cells are penetration (% verts) / stretch p95 / torn edges /
mean drift.

## Results

Real body:

| Garment | kinblend | kinblend + refine |
| --- | --- | --- |
| shirt | 1.9 / 0.31 / 1.2% / 0.06% | 1.9 / 0.31 / 1.2% / 0.06% |
| skirt | 8.1 / 2.56 / 12.7% / 0.55% | 8.5 / 2.13 / 12.8% / 0.64% |
| sleeve_loose | 1.3 / 0.13 / 0.0% / 0.02% | 1.3 / 0.14 / 0.0% / 0.02% |
| sleeve_tight | 0.4 / 0.22 / 0.0% / 0.00% | 0.4 / 0.22 / 0.0% / 0.00% |
| tights | 5.0 / 0.29 / 0.5% / 0.01% | 5.3 / 0.27 / 0.4% / 0.01% |
| trousers | 8.9 / 0.38 / 0.2% / 0.06% | 8.9 / 0.35 / 0.1% / 0.07% |

Capsules:

| Garment | kinblend | kinblend + refine |
| --- | --- | --- |
| cuff | 46.1 / 0.62 / 0.0% / 2.79% | 46.1 / 0.65 / 0.0% / 2.79% |
| dress | 7.4 / 1.11 / 6.2% / 3.30% | 5.5 / 0.98 / 4.8% / 3.34% |
| skirt | 4.5 / 0.51 / 0.5% / 2.24% | 7.0 / 0.53 / 0.0% / 2.19% |
| sleeve_loose | 5.3 / 0.62 / 0.0% / 0.98% | 5.7 / 0.55 / 0.0% / 1.09% |
| sleeve_tight | 10.7 / 0.37 / 0.0% / 0.08% | 10.4 / 0.35 / 0.0% / 0.12% |
| trousers | 9.2 / 0.46 / 0.0% / 0.14% | 9.4 / 0.38 / 0.0% / 0.16% |

The diagnostic that explains it (real skirt): the relaxation moves the
targets by 0.46% of the body's diagonal on average, and the fit residual
is 0.33%. On every garment the residual is within a few hundredths of the
correction: linear blend skinning with the available bones cannot reach
the relaxed positions, so the fit recovers only a fraction of them. On the
training poses themselves the skirt only goes from 2.00 to 1.58; this is
not overfitting, it is the model. Relaxing harder (100 rounds) moves the
targets twice as far, the residual follows (0.70%), and the test skirt
trades stretch 1.69 for 12.8% penetration. Starting from closest point
instead of `kinblend` is worse (0.61 to 1.76): its tear is concentrated in
a few edges, the relaxation spreads it, and LBS cannot represent the
spread.

- Small, consistent stretch gains on trousers and sleeves (0.38 to 0.35,
  0.62 to 0.55), within what a scale sweep gives.
- The dress improves on both metrics (7.4 / 1.11 to 5.5 / 0.98) and the
  real skirt on stretch (2.56 to 2.13); neither becomes usable.
- The capsule skirt loses penetration (4.5% to 7.0%): the relaxed targets
  pull the hem toward one leg in the training strides, which is the wrong
  side in a test spread.

## Conclusions and next steps

- **Not the default.** Kept behind `--refine`, since it never costs much
  and a garment with an awkward rest pose might want it.
- This closes the weight-only line for skirts and dresses: the rest-pose
  methods fail there, and refitting against example poses shows that no
  per-vertex LBS weight on the body's bones reproduces a relaxed skirt. The
  remaining options change the representation, not the weights: helper
  joints for the hem driven by the legs (a bone between the knees, a bone
  per hem quadrant), or a pose-space corrective (a per-pose displacement
  blended by joint angle). Both are a different research line.
- On the method side the loop's standing result is `kinblend`: relative
  matching, consistency check, offset-scaled blend limited to kin bones,
  bounded solve. It beats or ties closest point and Epic's inpainting on
  every garment that does not bridge the legs, on a real rig and under
  both skinning models.
