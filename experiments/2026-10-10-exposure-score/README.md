# Experiment: a leg-exposure score, and what it says about the hem rig

- **Date started:** 2026-10-10
- **Status:** done (score adopted; it reverses the previous experiment's reading of `follow`)
- **Related papers:** none in `papers/`

## Question and hypothesis

The hem rig v2 experiment ended with a rigid hem (`follow` 0) scoring best
on penetration and stretch on every skirt, which cannot be right: a thigh
swung out through a stiff panel shows as cloth clipping, not as garment
vertices inside the body. The penetration score counts garment vertices
inside the body, so the vertices a thigh crosses on its way out are the
only ones it sees. A second score is needed that counts the body instead:
of the body vertices a garment covers in rest, how many are on the outer
side of the cloth after posing.

## Method

`metrics.h` gains `coverage()` and `exposureFraction()`. Coverage is
decided in rest with the pseudonormal signed distance from each body
vertex to the garment (the sign of an open surface is the side of the
closest point's angle-weighted normal; the winding number used for
penetration is undefined for an open skirt). Body vertices whose closest
garment face touches the garment boundary are skipped, because the side
of a point below a hem or past a cuff is not defined, and the garment's
winding is detected from which side the nearby body lies on. Per pose the
same test runs on the posed meshes, and the score is the share of covered
vertices now on the outer side, again skipping boundary-closest ones (a
thigh that leaves below the hem is not counted as exposed). `weightbench`
prints it, writes it as the last CSV column (`exposure_fraction`), and
`summarize_results.py` shows it as "Exposed body".

Then the `follow` sweep of the previous experiment, with the gravity lower
tier, scored again: cells are penetration % / stretch p95 / stretch mean /
torn edges / exposed body, worst test pose, `kinblend --prune 8`
throughout.

## Results

| Garment | closest | kinblend | hem f0 | hem f0.25 | hem f0.5 | hem f0.75 | hem f1.0 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Real skirt | 7.5 / 0.61 / 0.78 / 3.9% / 23.6% | 8.1 / 2.56 / 0.45 / 12.7% / **11.0%** | 6.9 / 0.03 / 0.00 / 0% / 69.6% | 7.6 / 0.57 / 0.12 / 2.4% / 65.3% | 8.3 / 1.29 / 0.25 / 6.8% / 63.3% | 10.0 / 2.00 / 0.39 / 9.9% / 65.3% | 15.0 / 2.59 / 0.52 / 15.6% / 58.3% |
| Capsule dress | 2.1 / 1.21 / 0.63 / 5.1% / 65.5% | 7.4 / 1.11 / 0.30 / 6.2% / 35.5% | 3.9 / 0.00 / 0.00 / 0% / 75.8% | 4.6 / 0.39 / 0.11 / 0% / 74.7% | 4.6 / 0.80 / 0.23 / 0.8% / 61.8% | 3.9 / 1.17 / 0.35 / 7.8% / 20.5% | **0.3 / 1.49 / 0.47 / 17.6% / 1.2%** |
| Capsule skirt | 4.7 / 1.23 / 0.40 / 5.9% / 24.3% | 4.5 / 0.51 / 0.21 / 0.5% / 14.0% | 7.0 / 0.00 / 0.00 / 0% / 29.0% | 6.2 / 0.20 / 0.05 / 0% / 26.2% | 7.4 / 0.40 / 0.11 / 0% / 17.8% | 5.3 / 0.58 / 0.16 / 0.1% / 7.6% | **0.0 / 0.75 / 0.22 / 1.2% / 0.0%** |

Fitted garments are unaffected: the tight sleeve, trousers and tights
expose 0.0% under `kinblend`.

- **The rigid hem was the metric's blind spot**, as suspected: at `follow`
  0 seven in ten covered thigh vertices are outside the cloth on every
  garment, with penetration and stretch near zero. Exposure is now part of
  every table and the keep-or-drop call for anything on the legs.
- **On the capsules exposure falls monotonically with `follow`**, and full
  follow is the answer: the dress goes from 35.5% exposed under `kinblend`
  to 1.2%, the knee skirt from 14.0% to 0.0%, both with zero penetration.
  The price is stretch (p95 1.49 and 0.75), which is cloth sliding over a
  thigh, and that is what a hem does.
- **On the real skirt the rig is a regression on this score**: 58 to 70%
  exposed at every `follow`, against 11% for plain `kinblend`. Per pose,
  full follow exposes 14.5% in a 20 degree stride and 58% sitting, where
  `kinblend` exposes 0% and 11%. The rig tilts each bone only toward its
  own azimuth, by the cosine of the angle to the leg's swing, so in a
  stride the side panels do not move at all (cosine 0) while the outer
  thigh under them swings forward; on the capsules the legs are thin and
  far from the side panels, on the real body the thigh fills the skirt and
  the side panel has to go with it. The thigh-contact test of v2 did not
  help (62% at follow 0.5) because it tilts along the same azimuth.
- `closest` is worst on exposure (24% on both skirts, 66% on the dress):
  its seam tears are exactly the legs coming out.
- The v2 claim that the sitting penetration was "most likely the waist
  band" is withdrawn: exposure shows it is the thighs.

## Conclusions and next steps

- **Adopted:** the exposure score, as the leg metric alongside penetration;
  `follow` 1.0 for the capsule garments (the generator's `hem.json`), so
  the synthetic benchmark keeps the first penetration-free, exposure-free
  skirt and dress.
- **Not adopted** for the real skirt: the hem rig as driven. The previous
  two experiments' keep calls on the real skirt were made on penetration
  and stretch alone and do not survive this score; `kinblend` without the
  rig is the real-skirt baseline again (11% exposed, 8.1% penetration).
- **Next:** drive each hem bone in the direction of the leg on its side of
  the hip line rather than toward its own azimuth, so a side panel follows
  the thigh under it; the azimuth rule was written for the thin capsule
  legs and the real body shows its limit.
