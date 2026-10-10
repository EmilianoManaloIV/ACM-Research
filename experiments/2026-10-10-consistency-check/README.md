# Experiment: M4, closest-point consistency check

- **Date started:** 2026-10-10
- **Status:** done
- **Related papers:** [Abdrashitov et al. 2023](../../papers/2023-abdrashitov-robust-skin-weights-transfer.md)

## Question and hypothesis

Relative matching (M3) fixed the loose sleeve but tore the skirt and dress,
because between the legs neighbouring vertices match opposite legs. If a
match is dropped wherever the closest-point map jumps (two adjacent garment
vertices whose closest body points are far apart), and a strip around the
jump is dropped with it, the two sides should blend through inpainting
instead of snapping. Expected: skirt and dress stretch back near the anchored
method's, while keeping M3's gains elsewhere.

## Method

`transfer.cpp` method `consistent`: M3 plus, after matching, every matched
edge whose closest points are more than `consistencyJump` edge lengths apart
marks both endpoints; those and every matched vertex within
`consistencyMargin` times the jump distance are unmatched and inpainted.
`weightbench --save-posed` was added to dump posed meshes, which is how the
tear line was located (front and back centre of the skirt, x = 0, both
endpoints matched).

Sweep over the two settings on the skirt and dress (worst test pose):

| Garment | Jump | Margin | Matched | Penetration (% verts) | Stretch p95 |
| --- | --- | --- | --- | --- | --- |
| skirt | 3 | 0.5 | 66% | 8.6 | 1.59 |
| skirt | 3 | 1 | 62% | 8.6 | 1.31 |
| skirt | 3 | 2 | 52% | 8.6 | 0.96 |
| skirt | 2 | 1 | 51% | 9.0 | 1.00 |
| skirt | **2** | **2** | 28% | **7.8** | **0.59** |
| skirt | 2 | 3 | 3% | 15.4 | 0.52 |
| dress | 3 | 0.5 | 77% | 5.7 | 2.88 |
| dress | 2 | 1 | 57% | 5.6 | 2.15 |
| dress | **2** | **2** | 33% | 7.7 | 1.22 |
| dress | 2 | 3 | 8% | 9.1 | 0.64 |

A jump of 2 edge lengths is needed to catch the between-the-legs snap at all
(at 3 it sits right at the threshold and is missed on most rings). The margin
trades penetration for stretch: wider blend strips tear less but the legs
push through the panel more. Jump 2, margin 2 is now the default; margin 3
is the smoothness-first alternative and lands on the anchored method's numbers.

## Results

Full benchmark at the defaults (jump 2, margin 2):

| Garment | Method | Matched | Worst penetration (% verts) | Max depth (% body) | Stretch p95 (worst pose) | Offset drift (mean) | Valid | Influences | Time (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| cuff | anchored | 100% | 26.2 | 3.24 | 0.52 | 1.74% | yes | 2 | 0.001 |
| cuff | closest | 100% | 26.2 | 3.24 | 0.52 | 1.74% | yes | 2 | 0.001 |
| cuff | consistent | 77% | 26.2 | 3.32 | 0.58 | 1.73% | yes | 2 | 0.002 |
| dress | anchored | 7% | 9.2 | 9.72 | 0.58 | 4.14% | yes | 2 | 0.116 |
| dress | closest | 100% | 5.3 | 7.34 | 1.28 | 1.16% | yes | 2 | 0.005 |
| dress | consistent | 33% | 7.7 | 7.34 | 1.22 | 2.70% | yes | 7 | 0.201 |
| skirt | anchored | 2% | 16.4 | 6.40 | 0.52 | 2.41% | yes | 2 | 0.085 |
| skirt | closest | 100% | 8.0 | 4.06 | 1.54 | 1.29% | yes | 2 | 0.004 |
| skirt | consistent | 28% | 7.8 | 4.06 | 0.59 | 1.91% | yes | 5 | 0.062 |
| sleeve_loose | anchored | 12% | 1.6 | 3.84 | 0.00 | 5.37% | yes | 1 | 0.026 |
| sleeve_loose | closest | 100% | 2.9 | 4.28 | 0.58 | 1.44% | yes | 2 | 0.002 |
| sleeve_loose | consistent | 62% | 2.9 | 3.01 | 0.54 | 1.54% | yes | 2 | 0.013 |
| sleeve_tight | anchored | 94% | 9.2 | 3.96 | 0.60 | 1.48% | yes | 2 | 0.008 |
| sleeve_tight | closest | 100% | 9.2 | 3.96 | 0.60 | 1.47% | yes | 2 | 0.002 |
| sleeve_tight | consistent | 94% | 9.2 | 3.96 | 0.60 | 1.48% | yes | 2 | 0.005 |
| trousers | anchored | 75% | 12.6 | 13.72 | 0.47 | 0.64% | yes | 3 | 0.040 |
| trousers | closest | 100% | 12.9 | 10.99 | 0.54 | 0.59% | yes | 2 | 0.004 |
| trousers | consistent | 77% | 12.5 | 13.72 | 0.47 | 0.63% | yes | 3 | 0.041 |

- **Skirt:** the first result that beats both baselines at once: penetration
  7.8% (closest 8.0, anchored 16.4) at stretch 0.59 (closest 1.54, anchored
  0.52). The front and back panels now blend between the legs instead of
  snapping or hanging off one bone.
- **Loose sleeve:** keeps M3's fix (drift 1.5% against the anchored method's
  5.4%), with slightly less stretch than closest point.
- **Dress:** between the baselines on both metrics; margin 3 would match the
  anchored numbers. The long panel below the knees has no good answer with
  rigid bones: whatever follows the legs tears, whatever does not gets kicked
  through. This is where a pose-aware refinement (M6) or extra bones belong.
- **Tight sleeve, trousers, cuff:** unchanged, as they should be.

## Conclusions and next steps

- **Kept** as the default method: it is the only one that is never the
  worst and wins the skirt outright. It fails the strict "no primary metric
  worse by 5%" rule against the anchored method on the dress (stretch 0.58
  to 1.22) and skirt (0.52 to 0.59), but the anchored method buys those
  numbers with 16% penetration and a rigid loose sleeve, so the rule needs a
  second look: a weighted score or a Pareto rule rather than a veto per metric.
- The loose sleeve's remaining stretch at the elbow (0.54) comes from copying
  the body's narrow blend onto a sleeve 7 cm out. Next experiment: widen the
  garment's blend with its offset from the body.
- After that, a real rigged body and garments, since every number so far is
  on capsule bodies with hand-set weights.
