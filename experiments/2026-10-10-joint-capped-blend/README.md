# Experiment: M8, joint-capped blend region

- **Date started:** 2026-10-10
- **Status:** done (not adopted)
- **Related papers:** none

## Question and hypothesis

The offset blend's sweep showed the dress wanting scale 6 while the skirt
and loose sleeve overshoot past 4. If the blend region is grown nearest-first
and cut off as soon as it would take in a third dominant bone, a large scale
should give the dress its wide blend while the sleeve and skirt stop at the
joint. Expected: the dress's scale-6 stretch with the scale-4 numbers
everywhere else.

## Method

`transfer.cpp` method `jointcap`: `offsetblend` with
`TransferParams::blendJointCap`; the flood fill becomes a distance-ordered
search that stops when the matched vertices it has taken in span more than
two dominant bones. Scales 4, 6, 8 on every garment under both models.
Cells are penetration (% verts) / stretch p95 / mean drift, worst test pose.

## Results

| Skinning | Garment | offsetblend 4 | jointcap 4 | jointcap 6 | jointcap 8 |
| --- | --- | --- | --- | --- | --- |
| LBS | cuff | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% |
| LBS | dress | 6.4 / 0.64 / 4.3% | 7.4 / 1.32 / 2.9% | 6.4 / 1.32 / 2.9% | 6.2 / 1.32 / 2.9% |
| LBS | skirt | 4.5 / 0.51 / 2.2% | 6.1 / 0.50 / 2.3% | 6.1 / 0.49 / 2.3% | 6.6 / 0.49 / 2.4% |
| LBS | sleeve_loose | 5.3 / 0.62 / 1.0% | 5.3 / 0.62 / 1.0% | 8.4 / 0.96 / 1.6% | 8.4 / 0.81 / 2.0% |
| LBS | sleeve_tight | 10.7 / 0.37 / 0.1% | 10.7 / 0.37 / 0.1% | 10.7 / 0.42 / 0.1% | 10.7 / 0.41 / 0.2% |
| LBS | trousers | 9.2 / 0.46 / 0.1% | 9.2 / 0.46 / 0.1% | 9.6 / 0.38 / 0.2% | 9.6 / 0.39 / 0.2% |
| DQS | cuff | 34.8 / 0.19 / 2.0% | 34.8 / 0.19 / 2.0% | 34.8 / 0.08 / 2.0% | 34.8 / 0.04 / 2.0% |
| DQS | dress | 3.8 / 1.23 / 5.0% | 4.9 / 1.91 / 3.0% | 4.3 / 1.91 / 3.0% | 4.3 / 1.91 / 3.0% |
| DQS | skirt | 4.3 / 0.69 / 2.1% | 4.9 / 0.58 / 2.1% | 4.7 / 0.58 / 2.2% | 5.1 / 0.54 / 2.2% |
| DQS | sleeve_loose | 2.7 / 0.75 / 1.0% | 2.7 / 0.75 / 1.0% | 6.8 / 1.12 / 2.1% | 7.0 / 1.25 / 3.0% |
| DQS | sleeve_tight | 10.0 / 0.52 / 0.1% | 10.0 / 0.52 / 0.1% | 9.6 / 0.56 / 0.1% | 9.2 / 0.60 / 0.2% |
| DQS | trousers | 9.6 / 0.56 / 0.1% | 9.6 / 0.56 / 0.1% | 9.6 / 0.46 / 0.1% | 9.6 / 0.44 / 0.2% |

- Wrong on both counts. The dress's useful blend is exactly the one the cap
  forbids: between the legs it spans pelvis and both hips, three bones, so
  the cap cuts it back to the `consistent` result (stretch 1.32 / 1.91) at
  any scale.
- The sleeve's overshoot at scale 6 is not a third bone either. Shoulder and
  elbow are the only bones it sees, so the cap never fires, and the region
  still reaches too far along the arm (8.4% penetration, as uncapped).
  What overshoots is the region's length relative to the bone, not its bone
  count.
- The cap does help where a garment really does straddle two joints with
  short bones: the cuff and trousers improve on stretch at 6 and 8 (cuff
  0.19 to 0.04 under DQS). Too small a gain for a rule that breaks the
  dress.

## Conclusions and next steps

- **Not adopted.** Code stays behind `blendJointCap` (off by default).
- The right cap is a length, not a bone count: the region should not grow
  past some fraction of the bone it sits on, which needs the skeleton (or
  the body's weight gradient as a proxy for bone length). Worth one more
  try with the skeleton passed in, since `weightbench` already has it.
- The dress's between-the-legs panel wants a blend over three bones on
  purpose, so any cap has to be per-region. Defer until real garments show
  whether long panels like it are common.
