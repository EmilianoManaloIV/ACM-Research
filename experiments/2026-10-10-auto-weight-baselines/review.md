# Review: auto-weight baselines on humans, quadrupeds and creatures

Filled by a separate review session that did not do the work. See "Independent review" in `CLAUDE.md`.

- **Reviewer session:** https://claude.ai/code/session_01YacQDnDThLdmtqLvf9u9Vd
- **Date:** 2026-10-10
- **Material reviewed:** PR #5, branch `claude/research-architecture-21xe3f` at `710f28a`: `protocol.md`, `README.md`, `characters.csv`, the four results tables, and the harness diff against `main`.
- **Commits re-run:** `bb8162a` (run 2, the confirmatory run) and `53c9637` (both exploratory welded runs), each in a fresh `git worktree` with a clean tree.
- **Commands re-run:** exactly the three commands in the README's reproducibility record, plus `tools/fetch_articulation.py select` and `convert` (with and without `--weld`) and `tools/make_makehuman.py`.

## Reproduction

Reproduced: **yes, exactly.**

Environment: Ubuntu 24.04.5, x86_64, 4 CPUs, no GPU, g++ 13.3.0, CMake 3.28.3, Blender 4.0.2 (Ubuntu package `4.0.2+dfsg-1ubuntu8`), Python 3.13.16 with the pinned packages in `tools/requirements.txt`. This matches the recorded environment. Data: Articulation-XL2.0 `meta_Articulation_XL_2.0.csv` and `articulation_xlv2_test.npz` (1,154,853,607 bytes) at revision `11f777d8`; Objaverse-XL licence parquets from the repository's `main` branch (the record does not pin their revision).

| Check | Recorded | Re-run | Match |
|-------|----------|--------|-------|
| Character selection (`select`) | `characters.csv`, 145 models pass the filters, 5/4/5 per group | 145 pass, 5/4/5, file identical (`diff` empty) | yes |
| MakeHuman `body.gltf` SHA-256 | `4afe7b1d…3f83183` | `4afe7b1dcdcfb4fc1cd6db501476491190cc46422e15b6eb23d9dcd3a7f83183` | yes |
| Weld counts (`convert --weld`) | 6% to 83% of vertices merged; 6,781 → 4,035 on 0929e5e0; 3,138 → 525 on c84ca8a7; merged weights agree to 1e-15 | same counts on all 14; largest L1 between merged vertices' weights 2.2e-16 | yes |
| Run 2 `results.csv` (958 data rows) | committed table | all 958 rows and every column identical except `seconds` (differs in the 55 rows that carry a run time) | yes |
| Exploratory welded (983 data rows) | `results_explore_welded.csv` | identical except `seconds` | yes |
| Exploratory welded + BBW 100 (1,058 data rows) | `results_explore_welded_bbw100.csv` | identical except `seconds` | yes |
| Run 1 against run 2 (committed tables) | fixes (a) and (b) change only how BBW output is read | every row present in both tables is identical except the three BBW `weights` rows on the quadrupeds, which fix (b) touches; run 2 only adds rows that run 1 lost to the TetGen crash | yes |

Each method is deterministic in this environment: the re-runs reproduce every metric to the printed precision, Blender's bone heat included. A single run per character is therefore not a source of noise here, though it says nothing about sensitivity to the mesh or skeleton.

Run time for run 2 here: 5 min 38 s.

## Tests

`ctest --test-dir build` at `bb8162a`: `skinning_test` passed, `autoweight_test` passed ("all checks passed"), 2 of 2.

What `autoweight_test` covers: bone segment construction on a two-joint chain, the inverse-distance floor on two points, `validWeights` on a valid and a zero-row matrix, `compareWeights` L1, precision and recall, and the self-intersection metric on crossing, separated and edge-sharing triangles.

What no test covers:
- `boundedBiharmonicWeights`: TetGen, the position matching of fix (b), the boundary conditions, the handle-to-joint column mapping.
- `normaliseRows` with its -1e-9 clip.
- The self-intersection metric on faces that touch without sharing a vertex index. This is the case that decides the metric's behaviour on every dataset character (see below), and the inclusive bounds in `segmentHitsTriangle` count it as an intersection.
- The Blender script, the pose generator, the fetcher (`leg_pairs`, `weld`, the licence filter) and `run_auto_weight.sh` (the crash path and table assembly). These were exercised only by the runs themselves.

## Protocol compliance

1. **BBW is not run at the library's default.** The protocol says each method "runs once per character at its published defaults". The harness caps libigl's active-set solver at 8 iterations, the value in libigl's tutorial 403. The library's own default is 100 (`igl::active_set_params::max_iter(100)` in `active_set.h`; `BBWData` keeps it). At 8, every handle on all three quadrupeds that reached the solve stopped without converging (20 of 20, 31 of 31 and 28 of 28 "max iter without convergence" lines in `bbw.log`), leaving negative weights down to -0.0126 on most rows. These three are 3 of BBW's 14 failures. The README reports the cap but neither the protocol nor its Deviations table records it as a departure from defaults.
2. **Run 2 is a second run.** Under the stopping rule this needs the director's approval, which the Deviations table says is pending. The comparison above confirms run 1 and run 2 differ only where the two harness fixes say they should.
3. **Five deviations are adopted "pending his confirmation":** the group wording, test split only, 4 quadrupeds, the run 1 set-aside, and (implicitly) the welded runs. They need explicit decisions before Gate E can pass.
4. **No tuning on test data in the confirmatory run.** No method parameter was tuned. Two design choices were made after looking at scored data and should be named as such: the limbs-only random poses (chosen after artist weights tore on MakeHuman, which is also a scored character), and the welding and BBW iteration choices in the exploratory runs. The first affects every method alike and is a benchmark-difficulty choice, not method tuning. The exploratory runs are correctly labelled as exploratory.
5. **Run time (a protocol robustness metric) is not measured for bone heat.** For `csv:` methods the `seconds` column times reading the CSV (0.00 to 0.12 s), not Blender's solve. The Blender run time is not recorded anywhere.
6. **H2 cannot be tested,** as the README says: BBW is never valid, and the rule needs all three methods ranked in a body plan.

## Alternative explanations and confounds

1. **The BBW failures have four different causes, only some of which are H1's mechanism.** From run 2 on the 14 dataset characters: TetGen rejects the surface (5: 170d71d4, 1aafc1ea, 1d9648e7, 237ce5d2, 32623191) or crashes (5: 0929e5e0, 214fa1cb, 01a7a2b6, 2957e37e, 81007af3); the solve stops at the 8-iteration cap with negative weights (3 quadrupeds); a bone has no sample inside the mesh (1: 1d0a1663, plus MakeHuman). The 10 TetGen failures match H1's stated mechanism (BBW needs a clean tetrahedral mesh). The 3 cap failures are a configuration choice (item 1 above). The bones-outside failure is a property of the skeleton.
2. **Bones outside the skin are not the main cause.** With a generalised winding number test (libigl `fast_winding_number`, 10 samples per bone, |w| > 0.5 counts as inside), bones lying fully outside the surface occur on 4 of the 14 dataset characters: 0929e5e0 (4 fingertip leaves), 1d0a1663 (1 toe leaf), 237ce5d2 (2 toe leaves), 2957e37e (6), and on MakeHuman (18: tongue, eyelid and lip bones). On three of these TetGen fails first, so the bone check is never reached. Every character has at least one bone partly outside, mostly the harness's half-length extensions of leaf joints. Winding numbers on meshes split at seams are approximate, so treat these counts as indicative.
3. **Bone heat failures are not a unit-scale artefact.** The dataset meshes are normalised to a bounding box diagonal of about 1.0 to 1.4, against about 19.8 for MakeHuman, and Blender's heat weighting is known to be scale sensitive. Re-running the unchanged Blender script on 0929e5e0 at 1×, 10× and 100×, and on 1d9648e7, 2957e37e and 32623191 at 100×, left every vertex unweighted in every case. Scale is ruled out on these four. The README's finding that welding changes outcomes points to mesh topology instead.
4. **The seam split favours bone heat's stretch and hides cracks; this is now measured, not only inferred.** Artist and floor weights depend only on position (artist weights of coincident vertices agree to 2e-16), so coincident vertices move together. Bone heat solves on the split mesh, so it gives coincident vertices different weights. Over groups of coincident vertices on the six characters where bone heat is valid and the mesh is split, the median L1 between their bone heat weights is 0.26 to 0.77, and 59% to 75% of groups differ by more than 0.1 (01a7a2b6 0.41, 1d0a1663 0.42, 81007af3 0.26, c84ca8a7 0.77, d3ea521c 0.26, fb713146 0.43). Under any pose, those vertices separate: the mesh cracks along seams. No metric in the protocol measures that. This supports README finding 3. It also means bone heat's run 2 stretch figures on split meshes (quadruped median p95 0.081) are not comparable with the other methods' figures.
5. **The self-intersection metric is unusable as specified on this data.** At rest, 14.5% to 100% of faces count as self-intersecting (100% on c84ca8a7, d3ea521c and 32623191; MakeHuman 0%), because faces that touch across a seam share no vertex index and touching counts as intersecting. On the three characters at 100%, "posed minus rest" can never be positive, so the metric is blind there (artist and floor score exactly 0.000 in every pose). For bone heat, cracks separate touching faces, which reads as a large negative score (mean -0.08 to -0.21 on the five split characters where it is valid). For artist and floor the posed-minus-rest values stay between -0.006 and +0.036, so they are not obviously corrupted, but the saturation still caps them. This primary metric cannot rank methods on these meshes without welding (applied to every method) or a test that ignores contact.
6. **The 1e-9 clip decides the validity of converged BBW.** With 100 iterations on welded meshes, the most negative BBW weights are -8.0e-9 (c84ca8a7, 8 rows below -1e-9), -1.02e-6 (d3ea521c, 60 rows below -1e-9, 1 row below -1e-6) and -5.1e-9 (fb713146, 12 rows). These are numerical residue, not a method failure. Under a -1e-6 clip, c84ca8a7 and fb713146 would be valid and d3ea521c would still fail by one row. The protocol's text ("non-negative ... rows summing to 1 within 1e-6") can be read either way; the approved deviation fixed -1e-9. At 8 iterations the negatives are real (down to -0.0126 on 3,237 of 3,968 rows on d3ea521c), so the clip does not change run 2.
7. **H1's count is robust to every variant tested.** BBW fails more often than bone heat in run 2 (14 vs 9 of 14), on welded meshes (welded run: bone heat 8; BBW still fails the 10 TetGen cases, 1d0a1663 and the 3 quadrupeds), and if the 3 cap failures were counted as valid (11 vs 9). What the count does not show is that BBW is the less robust method in general: bone heat also fails on most characters, 8 of its 9 failures leave every vertex unweighted, and both methods' failures follow mesh defects that welding only partly fixes.
8. **The groups do not stand for body plans.** The "creature" group holds two Mixamo-style humanoid bipeds (01a7a2b6 with Hips/Spine/LeftHand bones, and 1d0a1663 with `mixamorig_` bones, labelled "mythical creature" and "character, mythical creature"). One of the four quadrupeds, c84ca8a7, has bones named `legR`, `legL`, `handR`, `handL` and is a soup of 525 unique positions. In effect the comparison has 7 to 8 humanoids, 3 to 4 quadrupeds and 3 other creatures. With n = 1 for bone heat in two groups and no valid BBW anywhere, no group-level deformation claim is supported.
9. **The size filter used pre-weld vertex counts.** Two characters (32623191 with 688 and c84ca8a7 with 525 unique positions) are below the protocol's 1,000-vertex floor as real meshes.

## Harness issues found

None of these changes a number in the committed tables.

1. `selfIntersectionFraction` counts touching faces as intersecting (inclusive `u`, `v`, `t` bounds in `segmentHitsTriangle`). This is the mechanism behind confound 5. The function's comment says only coplanar overlap is excluded.
2. `--bbw-iterations` defaults to 8, not libigl's 100 (protocol item 1).
3. Bone heat run time is not measured (protocol item 5).
4. `run_auto_weight.sh` builds the combined table's header from the first character folder in glob order. If that character's `results.csv` exists but is empty (bodybench crashed before writing), the table has no header. This happened in run 1: `results_run1.csv` has no header row. Run 2 is not affected because the BBW crash now has its own process.
5. TetGen writes `tetgen-tmpfile_skipped.face` and `.node` into the working directory, which is the repository root. After the first welded run these untracked files make `record_run.sh` stamp the tree as dirty. That is what happened in the re-run here, and it likely explains the "uncommitted changes" on the recorded welded + BBW 100 run, alongside the README's explanation. They could also be committed by mistake. They are not ignored by `.gitignore`.
6. Not a bug: TetGen's "point is coincident ... Ignored!" warnings come from bone samples at shared joints. `boundary_conditions` looks up tet vertices by position, so the merged points still get their constraints, as in libigl's tutorial.

## Claims checked

| Claim (README) | Verdict |
|---|---|
| BBW fails on 14 of 14 dataset characters, MakeHuman too | Supported (reproduced). Causes: TetGen 10, iteration cap 3, bone outside 1. |
| Bone heat fails on 9 of 14 (all 5 humanoids, 4 of 5 creatures) | Supported. 8 of the 9 leave every vertex unweighted; 01a7a2b6 leaves 1,328 of 7,568. Valid on all 4 quadrupeds, 1d0a1663 and MakeHuman. |
| Medians of mean p95 per group (table) | Supported: the summariser on the re-run table gives identical figures. |
| H1 holds by the rule as written | Supported mechanically, and robust to the variants above. The stretch half of the rule cannot be evaluated. See confounds 1 and 7 for what it does and does not show. |
| H2 cannot be tested | Supported. |
| Finding 1, seams and soups | Supported (weld counts reproduced). |
| Finding 2, self-intersection unreliable on split meshes | Supported, and stronger than stated: the metric is blind on 3 characters (rest 100%) and biased for bone heat (confound 5). |
| Finding 3, bone heat's split-mesh stretch looks better than it is | Supported. The quadruped median goes from 0.081 to 0.213 when welded (reproduced). The README calls the crack explanation an interpretation; the weight disagreement across seams (confound 4) now measures its cause, though not crack width. |
| Finding 4, welding turns 0929e5e0 valid; 2957e37e from 2,224 to 24 unweighted; 8 of 14 failures welded | Supported (reproduced). Note that 2,224 is all of 2957e37e's vertices, so that is a total failure becoming a partial one, not a partial one shrinking. |
| Finding 5, BBW at 100 iterations is close to bone heat | Reproduced: 0.253 vs 0.264 (c84ca8a7), 0.211 vs 0.185 (d3ea521c), 0.217 vs 0.241 (fb713146). Exploratory only: 3 characters, welded meshes, weights invalid under the protocol, scored with `--score-invalid`. "Two converge, the third leaves 2 bones short" matches `bbw.log` (d3ea521c, 2 handles at max iterations). This does not support "BBW matches bone heat" in general. |
| Finding 6, TetGen still fails on the same characters after welding | Supported (reproduced). |
| Limitations list | Accurate. Add: bone heat run time not measured, BBW cap departs from library default, self-intersection blind on 3 characters. |

## Recommendation

**Revise.** The research director makes the decision at Gate E.

The work is reproducible to the last digit, and the record is candid about its own weaknesses. Most of its findings hold up, and some are stronger than stated. It should not pass Gate E as it stands, for these reasons:

1. **H1's confirmation rests partly on a non-default setting.** The protocol requires published defaults. The 8-iteration BBW cap is not libigl's default and accounts for 3 of BBW's 14 failures. The H1 count still holds without them (11 vs 9), but the deviation has to be logged and decided, and the record should report the failure causes as above rather than as one count.
2. **One primary metric cannot be read on this dataset.** Self-intersection above rest is saturated on 3 characters and biased for bone heat on all split meshes. Bone heat's stretch on split meshes hides seam cracks. As run, run 2 supports no deformation-quality comparison involving bone heat.
3. **H2, half the experiment's question, is untestable** because no BBW result is valid.
4. **Pending approvals:** the second run, test split only, 4 quadrupeds, and the group wording.
5. **The groups do not represent body plans** (confound 8), so no per-group claim should be drawn.

Suggested revisions, each the director's decision:

- (a) Accept run 2 as the confirmatory record for H1's failure count only, with the failure causes broken down, and log the BBW cap as a deviation.
- (b) Decide whether a re-run is warranted under an amended protocol: welding applied to every method beforehand, as the protocol allows when logged; BBW at the library default of 100 iterations; an explicit negative-weight tolerance; and a self-intersection test that ignores contact or runs on welded meshes. This would be a new confirmatory run, and it is the only route to testing H2.
- (c) Record bone heat's run time from Blender, and add tests for `boundedBiharmonicWeights` and the touching-face case.
- (d) Re-examine group membership (the two Mixamo bipeds, c84ca8a7) before any per-group claim, or report results per character only.
