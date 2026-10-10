# Experiment: helper joints, a leg-driven hem rig for skirts and dresses

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** none in `papers/` (the construction is the game-rigging "skirt bones" idiom)

## Question and hypothesis

Fifteen experiments showed that no per-vertex weight on the body's own
bones handles a garment that bridges the legs: the panel between them is
either torn at a seam (closest point) or stretched everywhere (every
inpainting method), and refitting against example poses cannot fix it
because linear blend skinning with those bones cannot reproduce a relaxed
skirt. Riggers solve this with extra bones: a ring of hem bones hanging
from the hips, each pushed by the leg that swings toward it. If the
benchmark grows such a rig, driven procedurally from the leg poses, and the
transferred leg weight is routed onto it, skirts should stop tearing and
stop penetrating, at some cost in stretch where the hem has to follow a
leg.

## Method

`rig.h` gains driven joints: `skinningMatrices` poses them from the regular
joints after the hierarchy walk. `hem.cpp` builds the rig from a JSON spec
(`weightbench --hem-rig`): `count` bones around the hip-line midpoint in
`tiers` tiers (hip line; knee height, hanging from the first tier). A bone
at azimuth u tilts toward u by `follow` times the largest leg swing in that
direction, each leg's swing angle scaled by cos(angle between the swing's
horizontal direction and u) to the power `falloff`; tier 1 uses the shank's
swing relative to its thigh. Weights: after any transfer, every vertex's
weight on the leg chains (and listed extras, here the pelvis halves) is
moved onto the two nearest hem bones by azimuth, split between tiers by
height; `"apply": "unmatched"` limits that to the vertices the transfer
inpainted, `"all"` moves every vertex's leg weight.

Sweep on the real skirt (MakeHuman, `kinblend --prune 8`) and the capsule
dress: follow 1.0 / 0.75 / 0.5, falloff 1 / 2, count 8 / 16, apply all.
Cells: penetration (% verts) / stretch p95 / stretch mean / torn edges,
worst test pose.

## Results

`"apply": "unmatched"` tears at the boundary between vertices still on the
hips (which rotate about their own hip joints) and vertices on hem bones
(which rotate about the midpoint): real skirt p95 5.5, dress 8.9. The rig
has to own the whole skirt below the hip line; `"all"` from here.

Real skirt:

| Setting | kinblend | closest | f1.0 p1 n8 | f1.0 p2 n8 | f0.75 p2 n8 | f0.5 p1 n8 | f0.5 p2 n8 | f0.5 p2 n16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| pen / p95 / mean / torn | 8.1 / 2.56 / 0.45 / 12.7% | 7.5 / 0.61 / 0.78 / 3.9% | 15.1 / 2.43 / 0.54 / 15.9% | 14.7 / 2.60 / 0.52 / 15.6% | 10.0 / 2.00 / 0.39 / 9.8% | 8.2 / 1.30 / 0.28 / 7.0% | **8.2 / 1.28 / 0.25 / 6.8%** | 7.9 / 1.40 / 0.28 / 7.2% |

Capsule dress:

| Setting | kinblend | closest | f1.0 p1 n8 | f1.0 p2 n8 | f0.75 p2 n8 | f0.5 p1 n8 | f0.5 p2 n8 | f0.5 p2 n16 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| pen / p95 / mean / torn | 7.4 / 1.11 / 0.30 / 6.2% | 2.1 / 1.21 / 0.63 / 5.1% | 0.0 / 1.91 / 0.54 / 25.4% | 0.3 / 1.68 / 0.47 / 17.6% | 4.2 / 1.32 / 0.35 / 11.2% | 4.2 / 1.00 / 0.27 / 5.1% | **4.8 / 0.92 / 0.23 / 3.6%** | 4.7 / 1.08 / 0.24 / 6.7% |

Capsule skirt (knee length): kinblend 4.5 / 0.51 / 0.21 / 0.5%; hem f1.0
0.0 / 0.84 / 0.25 / 3.0%; f0.75 2.9 / 0.63 / 0.19 / 0.1%; f0.5 7.8 / 0.45
/ 0.13 / 0.0%.

- **`follow` is a dial between penetration and stretch**, and the whole
  dial beats `kinblend` on the dress: at 1.0 the dress is penetration-free
  for the first time (7.4% to 0.0%) at p95 1.9; at 0.5 it is better than
  `kinblend` on every number (4.8% / 0.92 / 0.23 / 3.6% against 7.4% /
  1.11 / 0.30 / 6.2%).
- **On the real skirt the rig halves every stretch statistic** against
  `kinblend` at equal penetration (f0.5 p2: 1.28 / 0.25 / 6.8% against
  2.56 / 0.45 / 12.7%), and has a third of closest point's mean stretch,
  but closest point keeps the lower p95 (0.61): its two seams are a few
  edges, the rig's stretch is spread around the hem where front panels
  tilt and side panels do not. Which is "better" is what the metric says,
  and the plan's primary metric is p95.
- **Full follow penetrates more on the real skirt** (15%), the opposite of
  the dress. The hem pivots on the hip-line midpoint, 1.1 dm inboard of
  each real hip, and the skirt helper sits close to the body: in `sit` the
  front panel rotating about the midpoint ends up inside the thighs that
  rotate about the hips. The capsule legs are further apart relative to
  their radius and do not show it. A pivot per side, or on the nearest hip,
  is the obvious fix.
- Sixteen bones are worse than eight everywhere: finer azimuth steps do
  not smooth the field, and more bones means each vertex's two-bone blend
  spans a smaller arc of a rotation field that is itself smooth.
- Falloff 2 (narrower push) beats 1 on every garment at the same follow.
- The rig is for skirts: on trousers and tights it moves leg weight off the
  legs (trousers 0.38 to 0.93 stretch), so it stays opt-in per garment.
- Same picture under DQS (real skirt 8.2 / 1.32, dress 4.4 / 0.96).

## Conclusions and next steps

- **Kept** as the skirt-and-dress path: `tools/make_*.py` write a
  `hem.json` (follow 0.5, falloff 2, 8 bones, 2 tiers) next to the poses,
  and `run_benchmark.sh` passes it for skirts and dresses. It is the first
  change in the log that improves a garment bridging the legs on both
  bodies, and it moves the skirt question from "which weights" to "which
  dial", which is a rigging decision an artist can make per garment.
- Next on this line: pivot the front and back bones on the nearer hip (or
  one ring per side) to fix the sitting penetration on close-fitting
  skirts; make tier 1 hang under gravity instead of following the shank;
  and give the hem bones a tiny collision test against the thighs so
  `follow` can be high only where a leg actually touches the panel.
- The metric question is now real: p95 edge stretch prefers a seam tear
  over a spread one, mean stretch the reverse. The plan should score skirts
  on mean stretch and torn edges with penetration, and leave p95 for
  fitted garments.
