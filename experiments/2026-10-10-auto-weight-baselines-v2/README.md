# Experiment: auto-weight baselines, amended protocol (v2)

- **Date started:** 2026-10-10
- **Status:** confirmatory run done (2026-10-11). Awaiting independent review and the director's Gate E decision. The harness changes (Gate D for v2) are in PR #5.
- **Question:** [`research/questions/RQ-002-automatic-body-weights.md`](../../research/questions/RQ-002-automatic-body-weights.md)
- **Protocol:** [`protocol.md`](protocol.md), Gate C approved 2026-10-10 (D-006). One deviation: a 600 s limit on BBW, after a discarded first attempt (see the protocol's Deviations).
- **Kind:** confirmatory, one run per method per character, as the stopping rule allows.
- **Review:** not yet done. A separate session will re-run the recorded command.

## Reproducibility record

| Field | Value |
|-------|-------|
| Code version | `95a3c8f`, clean tree (from `data/auto-weight-v2/run_record.txt`) |
| Command | `tools/record_run.sh data/auto-weight-v2 tools/run_auto_weight_v2.sh data/auto-weight-v2` (exit 0) |
| Settings | Welded meshes; `--ignore-contact --negative-tolerance 1e-6` for every method; BBW with `--bbw-iterations 100` under `timeout 600` |
| Environment | Ubuntu 24.04.5, x86_64, 4 CPUs, no GPU; g++ 13.3.0; CMake 3.28.3; Blender 4.0.2 (Ubuntu package); Python 3.13.16 with libigl 2.6.3, numpy 2.5.3, pandas 3.0.5, pyarrow 26.0.0, pygltflib 1.16.5, scipy 1.18.1 |
| Data | Articulation-XL2.0 test split (`articulation_xlv2_test.npz`) at Hugging Face revision `11f777d8`, licences from Objaverse-XL; characters in [`characters.csv`](characters.csv), converted with `tools/fetch_articulation.py convert --weld`. MakeHuman body as in the first experiment (`body.gltf` SHA-256 `4afe7b1d…3f83183`). See `data/README.md`. |
| Seeds | Pose seeds 0 to 19 (`tools/make_body_poses.py`); everything else deterministic. Each result is a single run. |
| Results table | [`results.csv`](results.csv) (copy of `data/auto-weight-v2/results.csv`, with a `set` column) |
| Large outputs | `data/auto-weight-v2/{same,fresh}/<character>/`: bones, poses, every method's weights, Blender, bodybench and BBW logs. The discarded first attempt is in `data/auto-weight-v2-attempt1/`. |

Per-character tables: `python3 tools/summarize_auto_weight.py results.csv`.

## Characters

- **Same set:** 12 dataset characters (biped 8, quadruped 3, other 1) plus MakeHuman (biped).
- **Fresh set:** 12 dataset characters (biped 5, quadruped 3, other 4).
- Groups follow the protocol's skeleton rule. Three fresh "other" characters (a5ed8e22, ba707933, c5ec6938) have biped bone names; the rule's known errors.

## Results

### Validity, by cause

BBW failures, with the protocol's causes:

| Cause | Same (12 dataset) | Fresh (12) |
|---|---|---|
| TetGen rejected the surface (status 1) | 4: 01a7a2b6, 170d71d4, 1aafc1ea, 1d9648e7 | 10: 0bce9943, 0c668ecb, 0e869cba, 0cb6db12, 94484878, c9070452, a5ed8e22, ba707933, c5ec6938, f1529842 |
| TetGen crashed (signal) | 5: 0929e5e0, 214fa1cb, 237ce5d2, 81007af3, 2957e37e | 1: 0e8b6860 |
| TetGen did not finish in 600 s | 0 | 1: 084f8b12 |
| No convergence | 1: d3ea521c | 0 |
| Bone without samples inside | 1: 1d0a1663 (and MakeHuman) | 0 |
| **Total** | **11 of 12** | **12 of 12** |

On d3ea521c the active-set solver stopped at 100 iterations without converging on 2 bones ("active_set: max iter without convergence" in `bbw.log`). One weight is -1.02e-6, just past the -1e-6 tolerance, so the weights are invalid. The results table's note for that row is empty; the cause comes from the log.

Bone heat failures (vertices left unweighted; Blender reports it could not find a solution for one or more bones):

| Set | Failures | All vertices unweighted | Partly unweighted |
|---|---|---|---|
| Same (12 dataset) | 7 | 3: 170d71d4, 1aafc1ea, 214fa1cb | 4: 01a7a2b6 (1,116), 1d9648e7 (1,071), 237ce5d2 (320), 2957e37e (24) |
| Fresh (12) | 8 | 6: 084f8b12, 0bce9943, 94484878, a5ed8e22, c9070452, f1529842 | 2: 0e869cba (152), 0e8b6860 (319) |

Bone heat is valid on 0929e5e0, 1d0a1663, 81007af3, d3ea521c, fb713146, MakeHuman (same) and 0c668ecb, 0cb6db12, ba707933, c5ec6938 (fresh). Its measured solve time was 0.01 to 2.1 s per character. The floor is valid on every character.

### Deformation, valid weights only

Mean p95 stretch over 25 poses:

| Set | Character | Group | Artist | Floor | Bone heat | BBW |
|---|---|---|---|---|---|---|
| same | 0929e5e0 | biped | 0.091 | 0.291 | 0.099 | |
| same | 1d0a1663 | biped | 0.333 | 0.351 | 0.285 | |
| same | MakeHuman | biped | 0.126 | 0.255 | 0.127 | |
| same | 81007af3 | quadruped | 0.123 | 0.325 | 0.117 | |
| same | d3ea521c | quadruped | 0.179 | 0.231 | 0.185 | |
| same | fb713146 | quadruped | 0.246 | 0.290 | 0.241 | 0.217 |
| fresh | 0c668ecb | biped | 0.091 | 0.293 | 0.120 | |
| fresh | 0cb6db12 | quadruped | 0.077 | 1.050 | 0.087 | |
| fresh | ba707933 | other | 0.097 | 0.364 | 0.070 | |
| fresh | c5ec6938 | other | 0.300 | 0.317 | 0.274 | |

Group medians, descriptive only (protocol change 6):

| Set | Group | Artist | Floor | Bone heat | BBW |
|---|---|---|---|---|---|
| same | biped (9) | 0.092 | 0.351 | 0.127 (n=3) | none valid |
| same | quadruped (3) | 0.179 | 0.290 | 0.185 (n=3) | 0.217 (n=1) |
| same | other (1) | 0.113 | 0.671 | none valid | none valid |
| fresh | biped (5) | 0.091 | 0.285 | 0.120 (n=1) | none valid |
| fresh | quadruped (3) | 0.105 | 0.458 | 0.087 (n=1) | none valid |
| fresh | other (4) | 0.108 | 0.341 | 0.172 (n=2) | none valid |

### Against the decision rules

These are the counts the rules call for. Whether H1 or H2 passes is the director's decision at Gate E.

- **H1** (BBW's failure count across dataset characters exceeds bone heat's):
  - Same set: BBW 11, bone heat 7. TetGen causes only: 9.
  - Fresh set: BBW 12, bone heat 8. TetGen causes only: 12 (11 if the timeout is not counted as TetGen).
  - Both sets together: BBW 23, bone heat 15.
  - Bone heat also fails on most characters (15 of 24), and 9 of its 15 failures leave every vertex unweighted.
- **H2** (on a character where floor, bone heat and BBW are all valid, the order by L1 differs from the order by mean p95):
  - Only fb713146 qualifies. By L1 against artist weights: bone heat 0.495 < BBW 0.581 < floor 0.922. By mean p95: BBW 0.217 < bone heat 0.241 < floor 0.290.
  - The orders differ, so the rule's condition is met on 1 character (same set, quadruped). No fresh character qualifies.

## Observations to check

- The artist weights on 237ce5d2 (same) and 0bce9943 (fresh) give a p95 stretch of 0.000 in every pose. The likely reason is that those rigs leave the limb joints rigid or unweighted, so the poses don't deform the skin; this is inferred, not checked.
- The floor on 0cb6db12 has mean p95 1.050 (worst 2.640), far above every other character.
- Bone heat's self-intersection above rest is slightly negative on 0929e5e0 (-0.0069).

## Limitations

- Small n: BBW is valid on 1 character of 25, so H2 rests on one quadruped.
- The grouping rule misplaces three fresh bipeds in "other"; group figures are descriptive only.
- The welded-mesh and BBW-100 settings were chosen after exploratory runs on the same characters. Only the fresh set is untouched by that choice.
- The first attempt (commit `eeffcc3`) stalled when TetGen ran for 2 hours on 084f8b12. It was discarded and the whole run repeated with a 600 s BBW limit. The limit changed one outcome (084f8b12 times out instead of hanging).
- The results table leaves the note empty for d3ea521c's BBW row; the cause is only in its log.
- Each method ran once. The first experiment's review found every method deterministic, but this run has not been re-run yet.
- Blender 4.0.2's bone heat is a tool, not the 2007 paper's method.

## Conclusions and next steps

Left to the director at Gate E, after the independent review.
