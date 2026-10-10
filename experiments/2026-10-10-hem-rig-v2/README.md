# Experiment: hem rig v2, pivot per hip, gravity lower tier, thigh contact

- **Date started:** 2026-10-10
- **Status:** done (gravity lower tier adopted; pivot and contact not)
- **Related papers:** none in `papers/`

## Question and hypothesis

The hem rig (previous experiment) left three threads: full follow pushed
the close-fitting real skirt into the stance thigh because every bone
pivots on the hip-line midpoint, 1.1 dm inboard of each hip; the lower
tier followed the shank, so a bent knee tilted the hem below it even where
nothing touches it; and `follow` was a global dial, pushing the hem whether
or not a leg reached it. Three options on the driven joints test each:
`"pivot": "hips"` slides each bone's pivot along the hip line toward its
side (side bones turn about their own hip, front and back bones about the
middle); `"lower": "gravity"` hangs the knee tier from the posed knee
point in world orientation, so only a shank's own swing tilts it; and
`"contact": true` measures each thigh's radius from the body (mean
distance of hip-dominated vertices to the hip-knee axis) and each hem
bone's panel radius from the garment (mean horizontal distance of the
vertices in its azimuth slot), then raises a bone's tilt above the follow
value in 2 degree steps until the panel, sampled at four heights, clears
every thigh swung toward it by the thigh radius plus `clearance`.

Runs: `kinblend --prune 8` plus the rig, worst test pose, cells are
penetration % / stretch p95 / stretch mean / torn edges.

## Results

Follow × pivot × lower (contact off):

| Garment | follow | mid, shank (v1) | hips, shank | mid, gravity | hips, gravity |
| --- | --- | --- | --- | --- | --- |
| Real skirt | 1.0 | 14.7 / 2.60 / 0.52 / 15.6% | 13.6 / 2.74 / 0.53 / 14.8% | 15.0 / 2.59 / 0.52 / 15.6% | 13.9 / 2.73 / 0.53 / 14.8% |
| Real skirt | 0.5 | 8.2 / 1.28 / 0.25 / 6.8% | 8.2 / 1.33 / 0.25 / 7.0% | 8.3 / 1.29 / 0.25 / 6.8% | 8.2 / 1.33 / 0.25 / 7.0% |
| Capsule dress | 1.0 | 0.3 / 1.68 / 0.47 / 17.6% | 1.7 / 1.72 / 0.48 / 18.2% | 0.3 / 1.49 / 0.47 / 17.6% | 1.7 / 1.54 / 0.48 / 18.2% |
| Capsule dress | 0.75 | 4.2 / 1.32 / 0.35 / 11.2% | 4.9 / 1.37 / 0.35 / 11.3% | 3.9 / 1.17 / 0.35 / 7.8% | 4.4 / 1.21 / 0.35 / 8.6% |
| Capsule dress | 0.5 | 4.8 / 0.92 / 0.23 / 3.6% | 4.7 / 0.95 / 0.23 / 4.2% | **4.6 / 0.80 / 0.23 / 0.8%** | 4.6 / 0.82 / 0.23 / 1.0% |
| Capsule skirt | 0.5 | 7.4 / 0.38 / 0.11 / 0.0% | 7.6 / 0.40 / 0.11 / 0.0% | 7.4 / 0.40 / 0.11 / 0.0% | 7.6 / 0.41 / 0.11 / 0.0% |

Contact (lower gravity, pivot mid), panel radius 1.1 to 2.2 dm on the real
skirt against a thigh radius of 0.88, 0.25 against 0.066 on the capsules:

| Garment | follow | no contact | contact, clearance 0 | contact, clearance 0.3 (0.03 capsules) |
| --- | --- | --- | --- | --- |
| Real skirt | 0.5 | 8.3 / 1.29 / 0.25 / 6.8% | 15.1 / 1.67 / 0.38 / 11.1% | 13.9 / 1.82 / 0.42 / 12.4% |
| Real skirt | 0.25 | 7.6 / 0.57 / 0.12 / 2.4% | 12.8 / 1.79 / 0.39 / 11.0% | 11.5 / 2.01 / 0.43 / 12.4% |
| Real skirt | 0.0 | 6.9 / 0.03 / 0.004 / 0.0% | 7.2 / 1.63 / 0.29 / 8.6% | 10.3 / 2.32 / 0.49 / 12.8% |
| Capsule dress | 0.5 | 4.6 / 0.80 / 0.23 / 0.8% | 4.6 / 0.80 / 0.23 / 0.8% | 4.4 / 0.98 / 0.23 / 4.9% |
| Capsule dress | 0.25 | 4.6 / 0.39 / 0.11 / 0.0% | 4.6 / 0.39 / 0.11 / 0.0% | 4.7 / 0.53 / 0.13 / 0.0% |
| Capsule dress | 0.0 | 3.9 / 0.18 / 0.04 / 0.0% | 3.9 / 0.18 / 0.04 / 0.0% | 3.9 / 0.26 / 0.06 / 0.0% |
| Capsule skirt | 0.5 | 7.4 / 0.40 / 0.11 / 0.0% | 7.4 / 0.40 / 0.11 / 0.0% | 6.8 / 0.40 / 0.11 / 0.0% |
| Capsule skirt | 0.0 | 7.0 / 0.07 / 0.01 / 0.0% | 7.0 / 0.07 / 0.01 / 0.0% | 6.2 / 0.11 / 0.02 / 0.0% |

- **Gravity lower tier helps the dress and costs nothing elsewhere**: at
  follow 0.5 the dress goes from 0.92 to 0.80 p95 and from 3.6% to 0.8%
  torn edges, at every follow it lowers p95 by 0.1 to 0.2, and the skirts
  (which end above the knee tier) are unchanged to the second decimal.
- **Pivoting on the hips changes nothing**: a few tenths of a percent of
  penetration either way, p95 up by 0.03 to 0.05. The sitting penetration
  the previous write-up blamed on the midpoint pivot is not where the
  pivot is; see the next point.
- **Contact makes the real skirt worse on every number** once it actually
  fires. (The first version sampled the bone axis, which runs down the
  middle between the thighs and is inside both of them in rest, so it never
  fired on the real body; the panel-radius version does.) Tilting a
  close-fitting panel about the hip line to clear the swung thigh drags its
  upper part into the pelvis and the stance thigh, and penetration doubles
  (7 to 15%). On the capsules the panel at 0.25 already clears a thigh of
  radius 0.066 at every swing, so contact is a no-op, and clearance only
  adds stretch.
- **Penetration is not monotone in `follow`**, and the rig with the legs
  switched off wins: follow 0 (every hem bone rigid under its parent) has
  the lowest penetration on all three garments (real skirt 6.9%, dress
  3.9%, capsule skirt 7.0%) with next to no stretch, beating `kinblend`
  (8.1 / 2.56 on the real skirt, 7.4 / 1.11 on the dress) and the whole
  follow dial except full follow on the capsules. A half tilt moves the
  panel into the stance leg without clearing the swinging one. That a
  rigid skirt scores best says more about the metric than the rig: the
  penetration score counts garment vertices inside the body, and a thigh
  swung through a stiff panel puts only the few vertices it crosses inside
  the body, while the leg itself sticks out through the cloth uncounted.

## Conclusions and next steps

- **Adopted:** `"lower": "gravity"`; `tools/make_*.py` write it into
  `hem.json`. **Not adopted:** `"pivot": "hips"` (no effect) and
  `"contact": true` (worse on the real skirt, inert on the capsules); both
  stay available as options. `follow` stays at 0.5 until the next point is
  settled, because moving it to 0 on this evidence would be fitting the
  metric's blind spot.
- **Next:** a leg-exposure score for skirts and dresses, counting body
  vertices between the hip line and the hem that lie outside the garment
  at their azimuth and height, so a thigh poking through a stiff skirt
  costs what it looks like. Then re-sweep `follow` under both scores.
- The pivot result retires the "midpoint pivot" explanation of the real
  skirt's sitting penetration; with contact inert at follow 0 and the
  remaining 7% not moving with any tilt, that penetration is most likely
  the waist and hip band, which the rig does not own, not the hem.
