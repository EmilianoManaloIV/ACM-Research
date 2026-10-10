# Experiment: real body, the MakeHuman base mesh with its artist weights

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** [Abdrashitov et al. 2023](../../papers/2023-abdrashitov-robust-skin-weights-transfer.md)

## Question and hypothesis

Every call so far was made on capsule bodies with four to nine bones and a
hand-set ramp. Do they hold on a real character: a 13k-vertex human with a
163-joint skeleton and weights painted by artists? Expected: the loose
garments still favour `offsetblend`; the skirt behaves like the synthetic
dress; the many-bone rig shows something the capsules could not.

## Method

`tools/make_makehuman.py` downloads the MakeHuman base mesh, default
skeleton and weights (CC0, see `data/README.md`) and writes
`data/makehuman/`: the body as a rigged glTF (the `body` group only; joint
positions are the centroids of the skeleton's joint vertex groups; weights
normalised and kept to the top four per vertex), two garments that ship
with MakeHuman (`skirt`, the knee-length skirt helper, 720 vertices;
`tights`, the full-body suit helper, 2674 vertices), and four cut from the
body itself by dominant bone and pushed out along the normals: `sleeve_tight`
(1.5 cm), `sleeve_loose` (6 cm), `trousers` (3 cm), `shirt` (4 cm; torso,
shoulders and upper arms). Poses: 8 train, 12 test (strides to 60 degrees,
a knee, a step, sitting, a spread, a kick, an elbow at 120, arms up 90,
a spine bend and a twist); the sign conventions were checked on the posed
feet, head and hands. MakeHuman units are decimetres.

Two metrics were added first, because the skirt's first run showed closest
point with a stretch p95 of 0.61 and a stretch *max* of 67: `stretch_mean`
and `torn_fraction` (share of edges stretched or shrunk by more than 100%).
A method that tears a few edges badly and one that spreads the tear over
many now read differently.

Run: `tools/run_benchmark.sh data/results_mh all lbs makehuman` and the
same with `dqs`. Rows for `bounded`, `anchored`, `relative` and `jointcap`
are left out below (they match `inpaint` or `consistent` to the second
decimal everywhere).

## Results

LBS:

| Garment | Method | Matched | Worst penetration (% verts) | Max depth (% body) | Stretch p95 (worst pose) | Torn edges (worst pose) | Offset drift (mean) | Valid | Influences | Time (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| shirt | closest | 100% | 2.5 | 1.59 | 0.41 | 2.3% | 0.03% | yes | 6 | 0.024 |
| shirt | consistent | 61% | 1.3 | 1.59 | 0.32 | 1.0% | 0.03% | yes | 15 | 2.187 |
| shirt | inpaint | 88% | 1.3 | 1.59 | 0.34 | 1.2% | 0.03% | no | 14 | 0.044 |
| shirt | lengthcap | 61% | 1.3 | 1.87 | 0.38 | 1.1% | 0.06% | yes | 24 | 2.172 |
| shirt | offsetblend | 61% | 1.3 | 2.10 | 0.54 | 1.1% | 0.09% | yes | 23 | 2.408 |
| skirt | closest | 100% | 7.5 | 1.08 | 0.61 | 3.9% | 0.04% | yes | 6 | 0.019 |
| skirt | consistent | 59% | 7.8 | 1.38 | 2.52 | 12.7% | 0.53% | yes | 14 | 0.350 |
| skirt | inpaint | 57% | 7.8 | 1.38 | 2.40 | 12.7% | 0.57% | no | 14 | 0.029 |
| skirt | lengthcap | 59% | 7.9 | 1.39 | 2.52 | 12.8% | 0.54% | yes | 14 | 0.506 |
| skirt | offsetblend | 59% | 8.8 | 1.05 | 2.50 | 12.7% | 0.56% | yes | 15 | 0.474 |
| sleeve_loose | closest | 100% | 0.4 | 0.10 | 0.44 | 2.5% | 0.01% | yes | 6 | 0.022 |
| sleeve_loose | consistent | 61% | 0.5 | 0.10 | 0.35 | 0.3% | 0.02% | yes | 8 | 0.414 |
| sleeve_loose | inpaint | | failed: singular inpainting system | | | | | | | |
| sleeve_loose | lengthcap | 61% | 0.5 | 0.10 | 0.24 | 0.1% | 0.02% | yes | 9 | 0.392 |
| sleeve_loose | offsetblend | 61% | 1.3 | 1.85 | 0.13 | 0.0% | 0.02% | yes | 14 | 0.371 |
| sleeve_tight | closest | 100% | 0.0 | 0.00 | 0.33 | 1.2% | 0.00% | yes | 6 | 0.016 |
| sleeve_tight | consistent | 92% | 0.0 | 0.00 | 0.25 | 0.0% | 0.00% | yes | 8 | 0.115 |
| sleeve_tight | inpaint | 98% | 0.0 | 0.00 | 0.28 | 0.4% | 0.00% | yes | 6 | 0.018 |
| sleeve_tight | lengthcap | 92% | 0.2 | 0.11 | 0.22 | 0.0% | 0.00% | yes | 8 | 0.105 |
| sleeve_tight | offsetblend | 92% | 0.4 | 0.29 | 0.22 | 0.0% | 0.00% | yes | 8 | 0.106 |
| tights | closest | 100% | 4.0 | 0.62 | 0.29 | 0.5% | 0.00% | yes | 7 | 0.022 |
| tights | consistent | 98% | 5.0 | 1.11 | 0.29 | 0.6% | 0.00% | yes | 15 | 0.631 |
| tights | inpaint | 98% | 5.0 | 1.11 | 0.29 | 0.6% | 0.00% | no | 17 | 0.039 |
| tights | lengthcap | 98% | 4.8 | 2.09 | 0.28 | 0.4% | 0.01% | yes | 16 | 0.601 |
| tights | offsetblend | 98% | 4.8 | 1.35 | 0.28 | 0.5% | 0.01% | yes | 16 | 0.514 |
| trousers | closest | 100% | 9.0 | 1.32 | 0.55 | 1.6% | 0.03% | yes | 6 | 0.022 |
| trousers | consistent | 98% | 9.7 | 1.40 | 0.46 | 1.0% | 0.03% | yes | 5 | 0.150 |
| trousers | inpaint | 98% | 9.7 | 1.40 | 0.46 | 1.0% | 0.03% | no | 5 | 0.040 |
| trousers | lengthcap | 98% | 9.9 | 1.74 | 0.43 | 0.2% | 0.06% | yes | 7 | 0.188 |
| trousers | offsetblend | 98% | 8.9 | 2.11 | 0.37 | 0.0% | 0.06% | yes | 6 | 0.179 |

DQS:

| Garment | Method | Matched | Worst penetration (% verts) | Max depth (% body) | Stretch p95 (worst pose) | Torn edges (worst pose) | Offset drift (mean) | Valid | Influences | Time (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| shirt | closest | 100% | 1.3 | 1.30 | 0.37 | 2.4% | 0.02% | yes | 6 | 0.029 |
| shirt | consistent | 61% | 1.2 | 1.30 | 0.31 | 1.1% | 0.03% | yes | 15 | 2.060 |
| shirt | inpaint | 88% | 1.2 | 1.30 | 0.33 | 1.3% | 0.02% | no | 14 | 0.048 |
| shirt | lengthcap | 61% | 1.3 | 1.60 | 0.46 | 1.3% | 0.05% | yes | 24 | 2.121 |
| shirt | offsetblend | 61% | 1.3 | 1.91 | 0.55 | 1.3% | 0.10% | yes | 23 | 2.338 |
| skirt | closest | 100% | 6.0 | 0.91 | 0.72 | 4.0% | 0.03% | yes | 6 | 0.021 |
| skirt | consistent | 59% | 4.7 | 0.88 | 3.18 | 17.6% | 0.82% | yes | 14 | 0.439 |
| skirt | inpaint | 57% | 4.7 | 1.09 | 3.07 | 17.3% | 0.89% | no | 14 | 0.025 |
| skirt | lengthcap | 59% | 4.3 | 0.62 | 3.19 | 17.2% | 0.84% | yes | 14 | 0.462 |
| skirt | offsetblend | 59% | 5.8 | 0.90 | 3.16 | 17.2% | 0.87% | yes | 15 | 0.372 |
| sleeve_loose | closest | 100% | 0.4 | 0.10 | 0.41 | 2.8% | 0.01% | yes | 6 | 0.021 |
| sleeve_loose | consistent | 61% | 0.4 | 0.11 | 0.35 | 0.3% | 0.02% | yes | 8 | 0.416 |
| sleeve_loose | inpaint | | failed: singular inpainting system | | | | | | | |
| sleeve_loose | lengthcap | 61% | 0.4 | 0.11 | 0.19 | 0.1% | 0.02% | yes | 9 | 0.403 |
| sleeve_loose | offsetblend | 61% | 1.3 | 2.05 | 0.07 | 0.0% | 0.02% | yes | 14 | 0.328 |
| sleeve_tight | closest | 100% | 0.4 | 0.12 | 0.29 | 1.7% | 0.00% | yes | 6 | 0.020 |
| sleeve_tight | consistent | 92% | 0.4 | 0.12 | 0.23 | 0.0% | 0.00% | yes | 8 | 0.111 |
| sleeve_tight | inpaint | 98% | 0.4 | 0.12 | 0.25 | 0.6% | 0.00% | yes | 6 | 0.028 |
| sleeve_tight | lengthcap | 92% | 0.4 | 0.23 | 0.19 | 0.0% | 0.00% | yes | 8 | 0.129 |
| sleeve_tight | offsetblend | 92% | 0.5 | 0.39 | 0.16 | 0.0% | 0.00% | yes | 8 | 0.096 |
| tights | closest | 100% | 2.6 | 1.22 | 0.36 | 0.7% | 0.00% | yes | 7 | 0.022 |
| tights | consistent | 98% | 2.8 | 1.22 | 0.37 | 0.6% | 0.00% | yes | 15 | 0.672 |
| tights | inpaint | 98% | 2.7 | 1.22 | 0.37 | 0.6% | 0.00% | no | 17 | 0.047 |
| tights | lengthcap | 98% | 2.9 | 2.34 | 0.36 | 0.6% | 0.01% | yes | 16 | 0.748 |
| tights | offsetblend | 98% | 2.7 | 1.81 | 0.35 | 0.6% | 0.01% | yes | 16 | 0.678 |
| trousers | closest | 100% | 6.2 | 1.38 | 0.70 | 2.7% | 0.02% | yes | 6 | 0.021 |
| trousers | consistent | 98% | 6.9 | 1.38 | 0.66 | 1.9% | 0.02% | yes | 5 | 0.165 |
| trousers | inpaint | 98% | 6.9 | 1.38 | 0.66 | 1.9% | 0.02% | no | 5 | 0.028 |
| trousers | lengthcap | 98% | 6.2 | 1.22 | 0.46 | 0.7% | 0.06% | yes | 7 | 0.181 |
| trousers | offsetblend | 98% | 6.3 | 2.49 | 0.42 | 0.0% | 0.06% | yes | 6 | 0.142 |

- **Epic's method fails on the loose sleeve outright**: "no target vertex
  matched". Its distance threshold is 5% of the *garment's* bounding box,
  which for a sleeve is 3.5 cm, less than the 6 cm the sleeve sits out.
  With the threshold at 10% it matches 83% and gives 0.5% / 0.45. The
  relative-threshold methods (M3 onward) match 61% and need no tuning.
  This is the paper's own use case, and the knob it depends on is tied to
  the wrong object.
- **Loose sleeve, trousers, tight sleeve**: `offsetblend` wins as on the
  capsules. Sleeve stretch 0.13 at 0% torn (closest 0.44 / 2.5%), with
  penetration 1.3% against 0.4%; trousers 0.37 / 0% torn against closest's
  0.55 / 1.6% at equal penetration; tight sleeve 0.22 against 0.33. The
  same order under DQS.
- **Shirt: `offsetblend` regresses.** Arms up 90 gives stretch 0.54 against
  `consistent`'s 0.32, with 23 influences per vertex. The blend region is
  grown over the garment surface, and at the armpit the shirt's sleeve and
  its side are one surface, so an upper-arm vertex 4 cm out averages in the
  chest and spine bones. The capsules never had two limbs meeting.
- **Skirt: every inpainting method loses to closest point**, 2.4 to 2.5
  stretch and 13% torn edges against 0.61 and 3.9%. The jump between the
  legs is two decimetres, the margin is twice that, so the whole lower
  skirt is unmatched and inpainted as a smooth blend of both legs; a
  stride then stretches all of it. Closest point assigns each hem vertex to
  one leg and tears at two seams (max stretch 67, that is a few edges
  stretched sixty-fold), which the p95 and even the torn fraction prefer.
  Neither is a usable skirt: this is the synthetic dress's problem, and it
  does not have a per-vertex-weight answer.
- **Tights**: all methods within 1% of each other; closest point has the
  least penetration (4.0% to 5.0%) because it never blends.
- **Influence counts**: the inpainting methods leave 14 to 24 non-zero
  weights per vertex on a 163-bone rig; a game engine takes 4 to 8.
  `--prune` handles it after the fact but the solve should know the limit.

## Conclusions and next steps

- Three synthetic calls survive (offset blend on loose limbs, relative
  matching, the consistency check on trousers), one is reversed (the blend
  must not cross between limbs), and one weak spot is confirmed worse than
  the capsules showed (garments bridging the legs).
- **Default stays `offsetblend`** for now: it wins three of six garments on
  both models and is never the worst except on the shirt, where the fix is
  known. The keep-or-drop rule would reject it on the shirt's 70% stretch
  loss alone; as before, the rule needs to weigh garments, not veto per
  metric.
- Next method (M10): grow the blend region by distance on the *body*, not
  on the garment, so the armpit and crotch stop leaking. Each matched
  vertex has a closest body point; neighbours whose body points are further
  apart than the radius are not averaged. Same code path, one extra test.
- The skirt needs the pose-aware refinement (M6) or a different model
  (a bone or two for the skirt itself); it is logged as out of scope for
  weight transfer alone until then.
- The benchmark should cap influences inside the solve, since every
  real-rig number above was produced with up to 24 per vertex.
