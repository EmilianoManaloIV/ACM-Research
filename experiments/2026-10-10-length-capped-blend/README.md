# Experiment: M9, length-capped blend region

- **Date started:** 2026-10-10
- **Status:** done (not adopted)
- **Related papers:** none

## Question and hypothesis

The joint-capped blend (M8) found the loose sleeve's overshoot at large
scales to be a length problem: the region reaches too far along the arm
without ever touching a third bone. If the blend radius is capped at a
fraction of the length of the vertex's dominant bone, large scales should
be safe on the sleeve and skirt while the dress keeps whatever blend its
bones allow. Expected: the sleeve's scale-4 numbers or better at scales 6
and 8, and the dress no worse than at scale 4.

## Method

`rig.cpp` gains `boneLengths` (distance from each joint to its first child
at rest); `weightbench` fills `TransferParams::boneLength` from the rig.
Method `lengthcap`: `offsetblend` with the radius at most
`lengthCap` times the dominant bone's length. Sweep: scale S in 4, 6, 8
and cap in 0.3, 0.5, 0.75 of the bone length, every garment, both models.
Cells are penetration (% verts) / stretch p95 / mean drift, worst test pose.
The uncapped scale-4 default is the baseline.

## Results

LBS:

| Garment | offsetblend 4 | S4 cap0.3 | S4 cap0.5 | S4 cap0.75 | S6 cap0.3 | S6 cap0.5 | S6 cap0.75 | S8 cap0.3 | S8 cap0.5 | S8 cap0.75 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| cuff | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% | 46.1 / 0.62 / 2.8% |
| dress | 6.4 / 0.64 / 4.3% | 6.5 / 1.36 / 2.6% | 6.6 / 1.36 / 2.7% | 7.2 / 1.36 / 2.8% | 6.5 / 1.36 / 2.6% | 6.6 / 1.36 / 2.7% | 6.9 / 1.36 / 2.8% | 6.5 / 1.36 / 2.6% | 6.6 / 1.36 / 2.7% | 6.9 / 1.36 / 2.8% |
| skirt | 4.5 / 0.51 / 2.2% | 5.3 / 0.51 / 2.1% | 5.7 / 0.50 / 2.1% | 5.7 / 0.50 / 2.1% | 5.3 / 0.51 / 2.1% | 5.7 / 0.50 / 2.1% | 5.7 / 0.50 / 2.1% | 5.3 / 0.51 / 2.1% | 5.7 / 0.50 / 2.1% | 5.7 / 0.50 / 2.1% |
| sleeve_loose | 5.3 / 0.62 / 1.0% | 7.2 / 0.53 / 0.5% | 6.8 / 0.48 / 0.7% | 6.4 / 0.50 / 0.8% | 7.2 / 0.53 / 0.5% | 6.8 / 0.48 / 0.7% | 6.1 / 0.53 / 0.9% | 7.2 / 0.53 / 0.5% | 6.8 / 0.48 / 0.7% | 6.1 / 0.50 / 0.9% |
| sleeve_tight | 10.7 / 0.37 / 0.1% | 10.7 / 0.37 / 0.1% | 10.7 / 0.37 / 0.1% | 10.7 / 0.37 / 0.1% | 10.7 / 0.40 / 0.1% | 10.7 / 0.42 / 0.1% | 10.7 / 0.42 / 0.1% | 10.7 / 0.39 / 0.1% | 10.7 / 0.41 / 0.2% | 10.7 / 0.41 / 0.2% |
| trousers | 9.2 / 0.46 / 0.1% | 9.1 / 0.52 / 0.1% | 9.1 / 0.51 / 0.1% | 9.1 / 0.49 / 0.1% | 9.5 / 0.47 / 0.1% | 9.5 / 0.46 / 0.1% | 9.5 / 0.42 / 0.1% | 9.5 / 0.49 / 0.1% | 9.5 / 0.45 / 0.1% | 9.5 / 0.42 / 0.2% |

DQS:

| Garment | offsetblend 4 | S4 cap0.3 | S4 cap0.5 | S4 cap0.75 | S6 cap0.3 | S6 cap0.5 | S6 cap0.75 | S8 cap0.3 | S8 cap0.5 | S8 cap0.75 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| cuff | 34.8 / 0.19 / 2.0% | 34.8 / 0.23 / 2.0% | 34.8 / 0.19 / 2.0% | 34.8 / 0.19 / 2.0% | 34.8 / 0.23 / 2.0% | 34.8 / 0.13 / 2.0% | 34.8 / 0.08 / 2.0% | 34.8 / 0.22 / 2.0% | 34.8 / 0.13 / 2.0% | 34.8 / 0.04 / 2.0% |
| dress | 3.8 / 1.23 / 5.0% | 4.3 / 1.93 / 2.8% | 4.4 / 1.94 / 2.8% | 4.7 / 1.94 / 2.9% | 4.3 / 1.93 / 2.8% | 4.4 / 1.94 / 2.8% | 4.6 / 1.94 / 2.9% | 4.3 / 1.93 / 2.8% | 4.4 / 1.94 / 2.8% | 4.6 / 1.94 / 2.9% |
| skirt | 4.3 / 0.69 / 2.1% | 4.5 / 0.68 / 1.8% | 4.1 / 0.65 / 1.8% | 4.3 / 0.64 / 1.9% | 4.5 / 0.68 / 1.8% | 4.1 / 0.65 / 1.8% | 4.3 / 0.64 / 1.9% | 4.5 / 0.68 / 1.8% | 4.1 / 0.65 / 1.8% | 4.3 / 0.64 / 1.9% |
| sleeve_loose | 2.7 / 0.75 / 1.0% | 7.2 / 0.84 / 0.4% | 5.3 / 0.82 / 0.6% | 1.2 / 0.76 / 0.8% | 7.2 / 0.84 / 0.4% | 5.3 / 0.82 / 0.6% | 1.0 / 0.72 / 0.9% | 7.2 / 0.84 / 0.4% | 5.3 / 0.82 / 0.6% | 1.0 / 0.72 / 1.0% |
| sleeve_tight | 10.0 / 0.52 / 0.1% | 10.0 / 0.52 / 0.1% | 10.0 / 0.52 / 0.1% | 10.0 / 0.52 / 0.1% | 9.6 / 0.56 / 0.1% | 9.6 / 0.56 / 0.1% | 9.6 / 0.56 / 0.1% | 9.6 / 0.61 / 0.1% | 9.2 / 0.60 / 0.2% | 9.2 / 0.60 / 0.2% |
| trousers | 9.6 / 0.56 / 0.1% | 9.6 / 0.58 / 0.1% | 9.6 / 0.57 / 0.1% | 9.6 / 0.57 / 0.1% | 9.6 / 0.55 / 0.1% | 9.6 / 0.52 / 0.1% | 9.6 / 0.48 / 0.1% | 9.6 / 0.57 / 0.1% | 9.6 / 0.47 / 0.1% | 9.6 / 0.46 / 0.1% |

- **The dress breaks again, for a reason M8 did not show.** Its panel
  between the legs is dominated by the pelvis, whose "bone" in this skeleton
  is the 9 cm hop to the first hip. Any cap keyed on that length limits the
  dress's blend to a few centimetres, and its stretch returns to the
  uncapped `consistent` value (1.36 / 1.94) at every setting. A root or hub
  joint's distance to one child is not a useful length.
- **The loose sleeve does what was hoped, under DQS.** Scale 6 or 8 with
  cap 0.75 gives 1.0% penetration at stretch 0.72, against 2.7% / 0.75 at
  the uncapped scale 4; the overshoot is gone. Under LBS it is a trade
  (stretch 0.62 to 0.48, penetration 5.3% to 6.8%).
- Skirt, trousers and the cuff move within noise; the cuff keeps improving
  on stretch with scale, as under M8.

## Conclusions and next steps

- **Not adopted.** The sleeve gain does not pay for the dress loss, and the
  cap's failure mode (short hub bones) will be common on real skeletons
  (pelvis, spine, clavicle). Code stays behind `lengthCap`, and
  `boneLengths` stays, since a per-bone signal is still the right idea.
- Two caps have now failed on the same garment. The dress's between-the-legs
  panel is a region that *should* blend over several bones, which is what
  M6 (pose-aware refinement) was for: decide the blend from how the garment
  behaves in the training poses, not from the rest-pose geometry. That is
  the next method-side experiment, after the data step.
- Data step next: a real rigged body and real garments. Every synthetic
  conclusion has so far survived one body revision at most.
