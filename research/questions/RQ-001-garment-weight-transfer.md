# RQ-001: Automatic skin weights for clothing on any rigged character

- **Opened:** 2026-10-10 (work began before the gated workflow; this file was reconstructed afterwards)
- **Current gate:** B (not started). Gate A passed 2026-10-10; this question waits for RQ-002, which goes first
- **Companion question:** [RQ-002](RQ-002-automatic-body-weights.md), automatic weighting of the body itself
- **Evidence:** `research/evidence/RQ-001.md` (not started; Gate B)
- **Experiments:** 20 exploratory experiments on branch `claude/project-thread-27uwpy` (draft PR #2), listed in that branch's `experiments/README.md`

## How this file was made

The first 20 experiments ran as an automatic loop from an experiment plan that was never committed. This file reconstructs the question, metrics and keep rule from those experiments' write-ups so the director can approve, change or replace them at Gate A. Under the workflow in `CLAUDE.md` those 20 experiments count as **exploratory**: their protocols were not frozen before they ran, and their parameters were tuned on the test poses they were scored on. They can motivate hypotheses but cannot confirm them.

## Objective

Given a rigged character (a biped, a quadruped or an unusual creature) and an unrigged garment or accessory fitted to it, compute the garment's skinning weights, and any extra garment bones such as the hem rig, automatically, so that it follows the character through new poses with less interpenetration, tearing and exposed body than two baselines: closest-point transfer and Epic's weight inpainting (Abdrashitov et al. 2023, DOI 10.1145/3610543.3626180).

Weighting the character's own mesh to its skeleton, with no existing weights to transfer from, is the companion question RQ-002. The director asked for both on 2026-10-10.

## Scope

- **In scope:** per-vertex weight transfer methods and their parameters; garment bones added automatically, such as the leg-driven hem rig (approved 2026-10-10); scoring under linear blend skinning (LBS) and dual quaternion skinning (DQS); human bodies (the two synthetic capsule bodies, MakeHuman, and a second real body) and non-human bodies (at least one quadruped and one unusual creature) with garments suited to each.
- **Out of scope:** cloth simulation; per-pose correctives (displacements learned or authored on top of skinning), which the last experiment proposed as the next line. That is a different question and would be opened as its own RQ if the director wants it.
- **Open:** which quadruped and creature bodies to use. Candidates need a licence that allows use and redistribution of results; RigNet's ModelsResource-RigNetv1 set (Xu et al. 2020, arXiv 2005.00559) has quadrupeds and fictional characters, but its licence terms have not been checked. Garments for non-human bodies (blankets, saddles, harnesses, armour) have to be made, so their design is part of Gate C.

## Definitions

- **Held-out poses:** the `test` set in each body's `poses.json` (7 or 8 poses on the capsules, 12 on MakeHuman). The `train` set (8 poses on MakeHuman) is the only set parameters may be tuned on from now on.
- **Penetration:** share of garment vertices inside the posed body (fast winding number). It does not subtract interpenetration already present at rest.
- **Exposure:** share of the body vertices a garment covers at rest that end up on the outer side of the posed garment (added in experiment 18 because penetration misses a leg passing out through a skirt).
- **Stretch:** per edge, |posed length / rest length − 1|; reported as p95, max and mean. **Torn:** share of edges with stretch above 1.
- **Offset drift:** mean change in garment-to-body distance from rest, as a share of the body's bounding-box diagonal.
- Source: `src/acm/skin/metrics.h` and `metrics.cpp` on the experiment branch.

## Metrics

Reconstructed from the experiments, for the director to confirm or change:

- **As used so far:** worst test pose for penetration and stretch p95 were called the primary metrics; mean stretch, torn fraction and exposure were added during the loop and later decided several calls.
- **Proposed primary for confirmatory work:** penetration, exposure and stretch p95, each reported per held-out pose and summarised as the mean and the worst pose over those poses, per garment and per body.
- **Secondary:** max depth, mean stretch, torn fraction, offset drift, weight smoothness (Dirichlet energy), influences per vertex, run time.

## Completion criteria

Proposed, for the director to set at Gate A:

1. A method answers the question if, on a fresh set of held-out poses never used for any tuning, it is no worse than the better baseline by more than 5% (relative) on any primary metric for any garment, and better on at least one primary metric for the garments that bridge the legs (skirt and dress), on the synthetic bodies, on two real human bodies, and on the quadruped and creature bodies. A result that holds for humans but not the other bodies is reported as such.
2. The result is reproduced by a separate review session from the recorded commit.
3. A negative answer is also a completion: if no method meets (1), the question closes with that finding and its limits.

The 5% threshold is the keep rule the loop used; the director confirmed it on 2026-10-10. It was broken twice by the loop (consistency-check, makehuman-body); under this file it is binding.

## What the exploratory experiments suggest

These are leads, not findings. Each is one deterministic run, tuned on its test poses, on one garment per case.

- Kin-limited blending (`kinblend`, the current default) fixes the shirt's armpit leak on MakeHuman, but makes the capsule dress worse.
- On the capsule bodies, the hem rig at full follow gave the first skirt and dress with no exposure and no penetration.
- On the real (MakeHuman) skirt, plain `kinblend` keeps exposure lowest (11% against 60% for the hem rig as first driven, 14% after side drivers), and nothing tried handles the sit pose.
- Earlier "beats both baselines" calls rested on metrics or bodies that were later replaced (body v1; worst-pose-only scoring before exposure existed). They need re-checking under the final metrics.

## Gate log

Only the research director's decisions go here.

| Date | Gate | Decision | By | Note |
|------|------|----------|----|------|
| 2026-10-10 | A | Hem rig in scope; per-pose primary metrics accepted; completion criteria and 5% threshold accepted; a second real body required; objective to cover quadrupeds and unusual creatures, bodies as well as clothes | Emiliano | Objective rewritten from this answer |
| 2026-10-10 | A | Passed: broadened objective confirmed as written; RQ-002 goes first | Emiliano | |
