# Experiment: auto-weight baselines on humans, quadrupeds and creatures

- **Date started:** 2026-10-10
- **Status:** run; waiting on the director for Gate D sign-off, the logged deviations, and Gate E
- **Question:** [`research/questions/RQ-002-automatic-body-weights.md`](../../research/questions/RQ-002-automatic-body-weights.md)
- **Protocol:** [`protocol.md`](protocol.md). Run 2 follows it and is meant to be the confirmatory run, but stays provisional until the director confirms Gate D and the overnight deviations. The two welded runs are exploratory.
- **Review:** not reviewed. `review.md` must come from a separate session.
- **Related papers:** [Pinocchio 2007](../../papers/), [BBW 2011](../../papers/)

## Reproducibility record

| Field | Value |
|-------|-------|
| Code version | Run 2: `bb8162a`, clean tree. Exploratory welded: `53c9637`, clean tree. Exploratory welded + BBW 100: `53c9637`; the stamp reports uncommitted changes, which were `tools/summarize_auto_weight.py` (written during the run, not used by it). Run 1 (set aside): `d3ce256`. |
| Command | Run 2: `tools/record_run.sh data/auto-weight-baselines tools/run_auto_weight.sh data/auto-weight-baselines`. Welded: `... tools/run_auto_weight.sh data/auto-weight-welded data/articulation-xl2/characters_welded`. Welded + BBW 100: `... tools/run_auto_weight.sh data/auto-weight-welded-bbw100 data/articulation-xl2/characters_welded --bbw-iterations 100 --score-invalid` |
| Environment | Ubuntu 24.04.5, x86_64, 4 CPUs, no GPU; g++ 13.3.0; CMake 3.28.3; Blender 4.0.2 (Ubuntu package); Python 3.13.16 with libigl 2.6.3, numpy 2.5.3, pandas 3.0.5, pyarrow 26.0.0, pygltflib 1.16.5, scipy 1.18.1 |
| Data | MakeHuman body from `tools/make_makehuman.py`, `body.gltf` SHA-256 `4afe7b1d…3f83183`. Articulation-XL2.0 test split (`articulation_xlv2_test.npz`, 1,154,853,607 bytes) at Hugging Face revision `11f777d8`, with licences from Objaverse-XL; characters in [`characters.csv`](characters.csv), converted by `tools/fetch_articulation.py convert` (with `--weld` for the exploratory runs). See `data/README.md`. |
| Seeds | Pose seeds 0 to 19 (`tools/make_body_poses.py`); everything else deterministic. Each result is a single run. |
| Results table | Run 2: [`results.csv`](results.csv). Exploratory: [`results_explore_welded.csv`](results_explore_welded.csv), [`results_explore_welded_bbw100.csv`](results_explore_welded_bbw100.csv). Set aside: [`results_run1.csv`](results_run1.csv). |
| Large outputs | `data/auto-weight-baselines/`, `data/auto-weight-welded/`, `data/auto-weight-welded-bbw100/`, `data/auto-weight-baselines-run1/`: per character, the bones, poses, every method's weights, and the Blender and bodybench logs. Regenerate with the commands above. |

## Question and hypotheses

From the protocol. **H1 (robustness):** BBW fails or degrades more than bone heat on multi-shell, non-manifold creature meshes. **H2 (metric disagreement):** in some body plan, ranking the methods by agreement with artist weights gives a different order from ranking them by deformation quality.

## Method

There are 15 characters:
- MakeHuman, plus 5 humanoids from the dataset;
- 4 quadrupeds (only 4 passed the four-leg-chain rule);
- 5 creatures.

Each character gets four sets of weights:
- the inverse-distance floor (B0);
- Blender's bone heat (B1), run headless with defaults;
- libigl BBW on a TetGen mesh (B2), with an active-set cap of 8 iterations, as in libigl's tutorial;
- the artist's weights, as a reference.

Valid weights are posed with LBS in 25 poses: 20 random poses that turn the limb-chain joints within ±45°, and 5 that bend one chain each (the fish-like 32623191 has only 4 chains, so it gets 4). The metrics are the protocol's. Deviations are listed at the end of the protocol. Run 1 was set aside because of two harness defects, fixed in `bb8162a`.

## Results (run 2, as the protocol specifies)

**Validity.** These are failures out of 14 dataset characters, with MakeHuman given separately:

| Method | Dataset failures | MakeHuman | How they failed |
|---|---|---|---|
| Floor (B0) | 0 / 14 | valid | |
| Bone heat (B1) | 9 / 14 | valid | All 5 dataset humanoids and 4 of 5 creatures. Blender reports "Bone Heat Weighting: failed to find solution for one or more bones" and leaves most or all vertices unweighted. |
| BBW (B2) | 14 / 14 | failed | TetGen rejected the surface (5) or crashed (5); the solve ran but left negative weights down to -0.013 (3, all quadrupeds); a bone had no samples inside the tetrahedral mesh (1d0a1663, and MakeHuman). |

**Deformation.** Median across characters of each method's mean p95 stretch, counting only valid weights:

| Group | Artist | Floor | Bone heat | BBW |
|---|---|---|---|---|
| Humanoid | 0.107 (n=6) | 0.330 (n=6) | 0.127 (n=1) | none valid |
| Quadruped | 0.214 (n=4) | 0.313 (n=4) | 0.081 (n=4) | none valid |
| Creature | 0.111 (n=5) | 0.352 (n=5) | 0.153 (n=1) | none valid |

Per-character tables come from `python3 tools/summarize_auto_weight.py results.csv`.

**Against the decision rules.**
- **H1:** by the rule as written, H1 holds. BBW fails on 14 of 14 dataset characters and bone heat on 9, so BBW's failure count is higher. The stretch half of the rule cannot be evaluated, because BBW has no valid character.
- **H2:** this run cannot test H2. The rule needs all three methods ranked in a body plan, and BBW is never valid.

Whether these outcomes stand is the director's call at Gate E. The findings below change how much weight they can carry.

## Findings that qualify run 2

1. **The dataset meshes are split at UV seams, and two are unindexed triangle soups.** Welding vertices that share a position removes 6% to 83% of them (for example, 6,781 → 4,035 vertices on 0929e5e0, and 3,138 → 525 on c84ca8a7). The artist weights of merged vertices agree to 1e-15.
2. **On split meshes, self-intersection above rest is unreliable.** Faces on either side of a seam touch, but they share no vertex index. At rest, 14% to 100% of faces count as self-intersecting (MakeHuman: 0%), so any small separation reads as a negative score (bone heat's −0.08 to −0.21). Welding lowers rest self-intersection to between 0 and 0.35. What remains on several characters is real overlap between shells.
3. **On split meshes, bone heat's stretch looks better than it is.** On the 4 quadrupeds, bone heat's median p95 went from 0.081 (split) to 0.213 (welded). The welded figure is level with the artist weights. The likely cause is that each seam island deforms on its own, so edges stretch less while cracks open along seams. No metric measures cracks, so this is an interpretation, not a measurement.
4. **Bone heat failures are mostly about the mesh, not the bones.** Welding turns one humanoid (0929e5e0) from all-unweighted to valid. Partial failures shrink a lot, for example from 2,224 to 24 unweighted vertices on 2957e37e. With welding, bone heat fails on 8 of 14 dataset characters instead of 9.
5. **BBW at 8 iterations has not converged.** With a cap of 100 on the welded meshes:
   - Two quadrupeds converge, and the third leaves 2 bones short.
   - The largest negative weight shrinks from -0.013 to between -5e-9 and -1e-6.
   - That is still past the protocol's strict non-negativity check (only values above -1e-9 are clipped), so the weights stay "invalid".
   - Scored anyway, BBW's mean p95 is close to bone heat's: 0.253 vs 0.264, 0.211 vs 0.185, and 0.217 vs 0.241.
6. **BBW's other failures are what H1 predicts.** TetGen needs a clean, closed, non-self-intersecting surface. Welding doesn't fix that, so TetGen still fails or crashes on the same 11 characters.

## Limitations

- There are only 4 quadrupeds, and one of them (c84ca8a7) has bones named legs and hands, so it may be a biped.
- Two of the "creatures" are Mixamo-style bipeds that the dataset labels "mythical creature".
- Characters come from the test split only.
- The size filter used vertex counts before welding. Two characters fall under 1,000 vertices once welded (525 and 688).
- Each method was run once. There is no review yet.
- Blender 4.0.2's bone heat is a tool, not the 2007 paper's method.

## Conclusions and next steps

These are left to the director (Gate E). Open decisions are listed in the thread's morning summary.
