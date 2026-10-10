# Protocol: automatic body-weighting baselines, amended after review

Approved at Gate C on 2026-10-10. Frozen; later changes go under Deviations. It amends [the first protocol](../2026-10-10-auto-weight-baselines/protocol.md) along the lines the independent review proposed ([`review.md` on PR #6](https://github.com/EmilianoManaloIV/ACM-Research/pull/6), suggested revision b). Everything not listed under "What changes" stays as in the first protocol, including the approved deviations (limbs-only random poses, chain bends, row normalisation, the four-leg-chain rule).

- **Question:** [`research/questions/RQ-002-automatic-body-weights.md`](../../research/questions/RQ-002-automatic-body-weights.md)
- **Kind:** confirmatory for H1 and H2
- **Gate C approved:** 2026-10-10, Emiliano, with the choices below (decision D-006)

## Why amend

The first run reproduced exactly, but the review recommended revise:
- BBW ran at 8 solver iterations instead of the library default of 100.
- Self-intersection was saturated or biased on the dataset's seam-split meshes.
- Bone heat's stretch on split meshes hid cracks along seams.
- H2 could not be tested because no BBW result was valid.

## Hypotheses

Unchanged: H1 (BBW fails or degrades more than bone heat on multi-shell, non-manifold meshes), H2 (agreement with artist weights and deformation quality rank the methods differently), and the null.

## What changes

1. **Every mesh is welded before any method runs.**
   - Vertices at exactly the same position are merged (`tools/fetch_articulation.py convert --weld`). Faces that collapse are dropped.
   - The artist weights of merged vertices agree to 1e-15 on all 14 characters, so the reference is unchanged.
   - The protocol always allowed a clean-up applied to every method beforehand. This makes it the standard.
2. **BBW runs at libigl's default:** `active_set_params::max_iter` = 100.
3. **Negative-weight tolerance is explicit.**
   - Before the validity check, weights above -1e-6 are set to zero, matching the 1e-6 tolerance on row sums.
   - Anything at or below -1e-6 still makes the weights invalid.
4. **Self-intersection ignores contact.**
   - A face pair counts only if the two triangles cross. A segment must pass through the other triangle's interior, with barycentric and segment parameters kept at least 1e-9 away from their bounds.
   - Faces that merely touch along an edge or at a vertex no longer count.
   - Faces sharing a vertex are still skipped. Because the meshes are welded, that now covers seams.
   - A unit test covers touching, crossing and separated triangles.
5. **Bone heat's run time is measured** inside the Blender script, from `parent_set` to its return.
6. **Results are reported per character first.** The review found that the label-based groups don't stand for body plans:
   - Two "creatures" are Mixamo humanoids.
   - One "quadruped" (c84ca8a7) has bones named hands and is a 525-vertex soup.

   Group medians are reported only under the grouping the director picks below.
7. **Size filter after welding:** the 1,000 to 100,000 vertex limit applies to the welded mesh.
8. **H2 is judged per character:**
   - H2 holds if, on any character where B0, B1 and B2 are all valid, their order by L1 against artist weights differs from their order by mean stretch p95.
   - It is also reported per group, under the chosen grouping.
   - This keeps H2 testable when BBW is valid on only a few characters.

## Choices made at Gate C

1. **Characters: both sets, reported separately.**
   - **Same:** the first experiment's dataset characters plus MakeHuman.
   - **Fresh:** the first 5 per group, in ascending uuid order, from the rest of the test split.
   - The exploratory runs saw the same characters, so only the fresh set is untouched by the choice of settings.
2. **Groups by skeleton topology,** not by dataset label. The operational rule is below.
3. **c84ca8a7 is dropped.**

Under change 7, 32623191 also leaves the same set: it has 688 vertices once welded, below the 1,000 floor.

### Grouping rule

Written into `tools/fetch_articulation.py select-topology`, with the list committed as [`characters.csv`](characters.csv) before any run.

- **Limb pair:** two children of the same joint whose subtrees have the same number of joints, and whose reach (the largest distance from that joint to a joint in the subtree) differs by at most 10%.
- **Leg pair:** seen along one of the six axis directions, a limb pair where:
  - both lowest joints lie in the lowest 15% of the mesh's extent;
  - they are at least 5% of the bounding-box diagonal apart;
  - each is at least 10% of the diagonal below the joint the pair hangs from.

  The count is the largest over the six directions.
- **Quadruped:** 2 leg pairs.
- **Biped:** 1 leg pair and at least 2 limb pairs. A rig with no leg pair by joints but 1 by skin also counts: the lowest vertex of each limb's skin (the vertices whose largest artist weight belongs to the limb) stands in for its lowest joint. That covers rigs that stop at the ankle or knee. Skin is used only to find bipeds, because hanging arms also reach low.
- **Other:** everything else.
- **Fresh candidates:** models whose label names a living thing (character, animal, mythical creature, anthropomorphic or humanoid) and that pass the licence, joint and welded-size filters.
- **MakeHuman** is a biped by construction.

**Known errors.** The rule has no access to bone names, but names are a useful check. Under it, three fresh characters land in "other" although their bone names say biped: a5ed8e22 (rigify thigh/shin), ba707933 (arm and leg joints), and c5ec6938 (Mixamo). The likely cause is that their hanging arms or missing feet defeat the leg test. They stay where the rule puts them, because the list was fixed before the run, and they are flagged in the results. Group-level figures are descriptive only. The claims rest on per-character results.

## Decision rules

- **H1 holds** if BBW's failure count across the dataset characters exceeds bone heat's. The failures are reported by cause (TetGen reject, TetGen crash, no convergence, bone without samples inside, other), and H1's mechanism is credited only with the TetGen causes.
- **H2** as in change 8.
- No further clean-up and no repair of individual meshes.

## Stopping rule

One run per method per character, at one commit, wrapped in `tools/record_run.sh` and run from a scratch working directory so TetGen's temporary files can't dirty the tree. No re-runs with changed settings.

## Code needed before the runs (Gate D)

- The weld step in `run_auto_weight.sh` (already in the fetcher).
- BBW default iterations set to 100 for this experiment, and the -1e-6 negative tolerance. Both are selected by flags, so the first experiment's numbers can still be reproduced.
- The contact-insensitive self-intersection test, behind a flag for the same reason, with its unit test.
- Bone heat timing in the Blender script.
- Unit tests for `boundedBiharmonicWeights`: position matching and the handle-to-joint column map on a small closed mesh.
- Fixes for the harness issues the review found: the table header taken from a crashed character, and TetGen's temporary files written in the repository root.

## Known risks and threats to validity

- The welded-mesh and BBW-100 settings were chosen after an exploratory look at results. Only fresh characters (decision 1b) are untouched by that.
- Welding can join parts that touch by coincidence, such as separate shells sharing a vertex position. That changes topology for every method alike.
- The quadruped group has at most 4 characters, and possibly 3.
- Plus the first protocol's risks (artist weight quality, impossible poses, Blender's bone heat as a tool).

## Deviations

| Date | Change | Reason | Approved by |
|------|--------|--------|-------------|
| | | | |
