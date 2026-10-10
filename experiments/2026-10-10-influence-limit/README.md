# Experiment: M11, influence limit inside the solve

- **Date started:** 2026-10-10
- **Status:** done (not adopted)
- **Related papers:** none

## Question and hypothesis

On the 163-joint MakeHuman rig the inpainting family leaves 12 to 24
non-zero weights per vertex; engines take 4 to 8. `weightbench --prune N`
keeps the top N after the fact and renormalises. If the bounded solve is
instead run a second time with every weight outside a vertex's top N fixed
at zero, the solver can redistribute within the allowed support and the
limited result should score better than the pruned one.

## Method

`TransferParams::solveInfluenceLimit` (`--solve-limit N`): after the first
bounded solve, each vertex's support is its top N bones; the per-bone
solves run again with the vertices outside that bone's support added to the
known set at 0. Smoothing runs after, as before. Compared with `--prune 4`
on the real body (`kinblend`, LBS, test poses).

## Results

| Garment | kinblend (influences) | --prune 4 | --solve-limit 4 (influences) | --solve-limit 4 --prune 4 | --solve-limit 8 --prune 8 |
| --- | --- | --- | --- | --- | --- |
| shirt | 1.8 / 0.297 (21) | 2.2 / 0.319 | 2.3 / 0.312 (12) | 2.4 / 0.318 | 1.7 / 0.299 |
| sleeve_loose | 1.3 / 0.135 (12) | 1.6 / 0.103 | 1.6 / 0.095 (6) | 1.6 / 0.097 | 1.3 / 0.134 |
| trousers | 8.9 / 0.384 (6) | 8.9 / 0.385 | 8.9 / 0.385 (5) | 8.9 / 0.385 | 8.9 / 0.384 |
| tights | 5.0 / 0.285 (16) | 4.6 / 0.287 | 4.7 / 0.285 (16) | 4.5 / 0.286 | 5.0 / 0.285 |

Penetration (% verts) / stretch p95, worst test pose.

- **Same as pruning, to the second decimal.** The limited solve ends up
  where post-hoc pruning ends up, and it is slower (two solves).
- The limit does not even hold: the post-solve smoothing (10 passes of
  neighbour averaging) spreads support back out, so `--solve-limit 4`
  leaves 12 influences on the shirt and 16 on the tights. A real limit
  would have to constrain the smoothing too, and the pruned result already
  shows what that would be worth: nothing measurable.
- **Pruning is not free on the shirt** (0.297 to 0.319) and is free or
  helpful elsewhere (sleeve 0.135 to 0.103). Eight influences is free
  everywhere. On rigs this dense the influence budget is part of the
  result and tables should say which budget they were scored at.

## Conclusions and next steps

- **Not adopted.** `--prune N` stays the way to meet an influence budget;
  the code stays behind `solveInfluenceLimit`.
- Real-rig tables from here on report at `--prune 8`, which costs nothing
  on any garment and is what most engines allow.
- Next: M6, pose-aware refinement, for the skirt and dress.
