# Experiment: hem rig v3, side-driven bones and a waistband rule

- **Date started:** 2026-10-10
- **Status:** done (both kept as options; the real skirt still scores better without the rig on exposure)
- **Related papers:** none in `papers/`

## Question and hypothesis

The exposure score showed the azimuth-driven hem rig leaving 58 to 70% of
the real thighs outside the skirt: a bone tilts only toward its own
azimuth, so in a stride the side panels never move while the thigh under
them swings forward. Two changes test whether the rig can be driven
right on a real body. `"drive": "side"`: each leg pushes the bones on its
side of the hip line (fully on its side, fading to nothing on the far
side, front and back bones belonging to both) in the direction of its own
horizontal swing, only when that swing is not away from the bone, and
pushing legs are averaged, so in a stride the front bone follows the
forward leg, in a sit both legs agree and in a spread the two pushes on
the front bone cancel. `"waist"`: a fifth of the real skirt lies above the
hip line, and the rig was swinging that waistband about the hips with the
hem; the share of a vertex's leg weight that moves onto the rig now ramps
from 0 at the hip line to 1 over `waist` thigh lengths, and the rest goes
to the joint the rig hangs from.

Cells: penetration % / stretch p95 / stretch mean / torn edges / exposed
body, worst test pose, `kinblend --prune 8` under the rig, gravity lower
tier throughout.

## Results

Drive, everything moved onto the rig (`waist` off):

| Garment | kinblend, no rig | azimuth f1.0 | side f0.5 | side f0.75 | side f1.0 |
| --- | --- | --- | --- | --- | --- |
| Real skirt | 8.1 / 2.56 / 0.45 / 12.7% / **11.0%** | 15.0 / 2.59 / 0.52 / 15.6% / 58.3% | 9.2 / 1.53 / 0.27 / 8.2% / 62.3% | 14.4 / 2.28 / 0.40 / 11.8% / 43.0% | 13.6 / 3.01 / 0.53 / 15.1% / 14.4% |
| Capsule dress | 7.4 / 1.11 / 0.30 / 6.2% / 35.5% | **0.3 / 1.49 / 0.47 / 17.6% / 1.2%** | 7.7 / 1.49 / 0.26 / 7.1% / 31.8% | 4.6 / 2.22 / 0.36 / 11.8% / 12.4% | 2.3 / 2.74 / 0.46 / 18.3% / 10.7% |
| Capsule skirt | 4.5 / 0.51 / 0.21 / 0.5% / 14.0% | **0.0 / 0.75 / 0.22 / 1.2% / 0.0%** | 9.0 / 0.74 / 0.12 / 2.4% / 12.3% | 1.6 / 1.03 / 0.19 / 5.2% / 0.0% | 1.6 / 1.20 / 0.26 / 6.4% / 0.0% |

Waistband rule at full follow (`waist` 0.2; "on legs" is a first version
that left the band's share where the transfer put it, "to parent" moves
it to the rig's parent joint):

| Garment | drive | waist off | waist 0.2, on legs | waist 0.2, to parent |
| --- | --- | --- | --- | --- |
| Real skirt | side | 13.6 / 3.01 / 0.53 / 15.1% / 14.4% | 6.4 / 3.01 / 0.52 / 15.1% / 13.3% | **6.7 / 3.01 / 0.51 / 15.1% / 15.6%** |
| Real skirt | azimuth | 15.0 / 2.59 / 0.52 / 15.6% / 58.3% | 11.9 / 2.59 / 0.52 / 15.6% / 57.7% | 12.5 / 2.59 / 0.50 / 15.6% / 57.7% |
| Capsule dress | azimuth | 0.3 / 1.49 / 0.47 / 17.6% / 1.2% | 0.3 / 1.49 / 0.48 / 17.6% / 15.9% | 3.1 / 1.49 / 0.46 / 17.6% / 12.4% |
| Capsule skirt | azimuth | 0.0 / 0.75 / 0.22 / 1.2% / 0.0% | 0.0 / 0.80 / 0.24 / 2.0% / 76.5% | 5.5 / 0.77 / 0.20 / 1.6% / 38.2% |

- **Side drive is what the real body needs**: exposure at full follow
  falls from 58% to 14% (per pose: stride 60 from 41% to 9%, sit from 58%
  to 15%). On the capsules it is worse than the azimuth rule (dress 1.2%
  to 10.7% exposed in the spread, where the front-left bone now takes the
  left leg's full 45 degree push instead of the cosine-weighted 22), so
  the rule stays per body.
- **The waistband was the real skirt's penetration**: moving only the
  weight below the hip line halves it (13.6% to 6.4%, below `kinblend`'s
  8.1%) with no change to the other numbers. On the capsules, whose band
  is 0.12 thigh lengths tall, swinging the band with the hem was harmless
  and any waist rule is harmful: left on the legs the band splits between
  the hips in a spread (skirt 0% to 76% exposed), moved to the static
  pelvis the ramp zone blends a 90 degree rotation with identity in a sit
  and the ring collapses inward past the thighs (38%). `waist` is
  therefore off by default and set to 0.2 only for the MakeHuman rig.
- **The real skirt is still not won by the rig.** Its best rig (side,
  full follow, waist 0.2) against plain `kinblend`: penetration 6.7% vs
  8.1%, exposure 15.6% vs 11.0%, p95 3.01 vs 2.56, torn 15.1% vs 12.7%.
  Better on one number, worse on three; the sit pose is where both lose
  (rig 15.6% exposed, `kinblend` 11%). Four experiments on the rig have
  not produced a real-skirt result that beats a per-vertex transfer on
  the leg score, while on the capsules the rig is the only thing that
  ever reached zero.

## Conclusions and next steps

- **Kept as options:** `"drive": "side"` and `"waist"`; the MakeHuman
  generator writes `follow 1.0, drive side, waist 0.2` (its best rig), the
  synthetic one keeps `follow 1.0, azimuth`. `run_benchmark.sh` now scores
  skirts and dresses twice, plain and with the rig (`+hem` rows), so the
  table always shows both sides of this call.
- **Not adopted** as the real-skirt default: the rig; `kinblend` alone
  remains the real-skirt baseline on exposure.
- **Next:** the two paths should meet. The rig gives the smooth field
  between the legs that a per-vertex transfer cannot, the transfer keeps
  each thigh clad where the rig's eight rigid panels cannot bend around a
  real thigh. Blending the two weight sets per vertex by distance to the
  nearest thigh (rig between the legs, transfer over them) is the obvious
  experiment, and the exposure score will say whether it holds.
