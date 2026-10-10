# Experiment: M7, offset-scaled blend

- **Date started:** 2026-10-10
- **Status:** done (not adopted)
- **Related papers:** none yet

## Question and hypothesis

The loose sleeve's remaining stretch (p95 0.54 at a 135 degree elbow) was
assumed to come from copying the body's narrow blend (3 cm) onto a sleeve
that sits 7 cm out. If each matched vertex averages the copied weights over
a radius proportional to its distance from the body, the blend across the
elbow should widen with the offset and the stretch should fall.

## Method

`transfer.cpp` method `offsetblend`: `consistent` plus
`TransferParams::offsetBlend` (default 2): after matching, every matched
vertex takes the mean of the matched closest-point weights within
`offsetBlend` times its own body distance, found by a flood fill. Swept
the scale from 1 to 4 on the loose sleeve; then `--save-posed` and a script
to split the elbow stretch into extensions and compressions.

## Results

Loose sleeve, worst test pose (elbow 135):

| Method | Penetration (% verts) | Stretch p95 | Compressions p95 | Extensions p95 |
| --- | --- | --- | --- | --- |
| consistent | 1.4 (2.9 worst pose) | 0.54 | 0.49 | 0.37 |
| offsetblend 1 | 5.1 | 0.59 | | |
| offsetblend 2 | 5.7 (1.2 at elbow 135) | 0.60 | 0.60 | 0.28 |
| offsetblend 3 | 4.3 | 0.65 | | |
| offsetblend 4 | 3.5 | 0.59 | | |

Ring-averaged weights along the sleeve (shoulder bone, elbow bone):

| x along arm | consistent | offsetblend 2 |
| --- | --- | --- |
| 0.16 | 1.00 / 0.00 | 0.95 / 0.05 |
| 0.23 | 0.77 / 0.23 | 0.76 / 0.24 |
| 0.30 (elbow) | 0.50 / 0.50 | 0.65 / 0.35 |
| 0.37 | 0.27 / 0.73 | 0.52 / 0.48 |
| 0.44 | 0.10 / 0.90 | 0.42 / 0.58 |
| 0.51 | 0.02 / 0.98 | 0.37 / 0.63 |

- The premise was wrong. The `consistent` blend is already wide (the
  transition spans 0.23 to 0.51, longer than the forearm's own blend),
  because a far-off sleeve samples a spread of closest points around the
  elbow.
- Widening it further made things worse. The elbow stretch is dominated by
  compression on the inside of the bend (p95 0.49), which is linear blend
  skinning collapsing the 50/50 vertices toward the joint, and a wider blend
  puts more vertices in that zone (compression p95 up to 0.60). Extensions
  did fall (0.37 to 0.28), which is the part the hypothesis got right.
- The averaging also leaks: at the wrist end the forearm never reaches full
  weight (0.63), so the cuff lags the hand and penetration rises in the
  milder poses.

## Conclusions and next steps

- **Not adopted.** Code stays behind `offsetBlend` (off by default).
- The loose sleeve's elbow is a skinning-model limit, not a weights limit:
  no weight profile stops linear blend skinning from collapsing a bent tube.
  Dual quaternion skinning would, so the benchmark should grow a skinning
  mode and score each method under both. That is a one-day change and would
  change the ranking on every bending joint.
- The remaining weight-side target is the dress below the knees, which needs
  the pose-aware refinement (M6) or added bones.
