# Protocol: automatic body-weighting baselines, amended after review

Draft for Gate C, not approved. Nothing in this folder has run. It amends [the first protocol](../2026-10-10-auto-weight-baselines/protocol.md) along the lines the independent review proposed ([`review.md` on PR #6](https://github.com/EmilianoManaloIV/ACM-Research/pull/6), suggested revision b). Everything not listed under "What changes" stays as in the first protocol, including the approved deviations (limbs-only random poses, chain bends, row normalisation, the four-leg-chain rule).

- **Question:** [`research/questions/RQ-002-automatic-body-weights.md`](../../research/questions/RQ-002-automatic-body-weights.md)
- **Kind:** confirmatory for H1 and H2, once approved
- **Gate C approved:** not yet

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

## Decisions this draft needs (director)

1. **Which characters?** Every option below is affected by the exploratory runs: they showed results for the welded meshes and for BBW at 100 iterations on 3 quadrupeds, which this protocol would make the standard.
   - **(a) The same 14 plus MakeHuman.** Results are directly comparable with run 2, but the run is less clean as a confirmation.
   - **(b) Fresh characters.** Take the next ones by uuid order from the test split that pass the filters after welding, excluding the 14 already used. There are enough humanoids and creatures, but the test split has no further quadrupeds under the four-leg-chain rule, so the quadruped group would stay the same 4 (3 if c84ca8a7 is excluded).
   - **(c) Both:** report the old characters and the fresh ones separately.

   Recommended: (c).
2. **How to group?**
   - **(a) Keep the labels.**
   - **(b) Group by skeleton:** one pair of leg chains plus a pair of arm chains is a biped, two pairs of leg chains with no arms is a quadruped, anything else is "other". This is mechanical, but it is a new rule that would need checking against the characters by eye.
   - **(c) No groups:** per-character results only.

   Recommended: (c) for the claims, with (a) shown for reference.
3. **c84ca8a7.** Keep it as a quadruped, move it, or drop it as below the size floor once welded (525 vertices)? Under change 7 it drops out on its own.

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
