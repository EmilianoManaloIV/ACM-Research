# RQ-001: Automatic skin-weight transfer from a body to a garment

- **Opened:** 2026-10-10 (work began before the gated workflow; this file was reconstructed afterwards)
- **Current gate:** A, awaiting the research director's decision
- **Evidence:** `research/evidence/RQ-001.md` (not started; Gate B)
- **Experiments:** 20 exploratory experiments on branch `claude/project-thread-27uwpy` (draft PR #2), listed in that branch's `experiments/README.md`

## How this file was made

The first 20 experiments ran as an automatic loop from an experiment plan that was never committed. This file reconstructs the question, metrics and keep rule from those experiments' write-ups so the director can approve, change or replace them at Gate A. Under the workflow in `CLAUDE.md` those 20 experiments count as **exploratory**: their protocols were not frozen before they ran, and their parameters were tuned on the test poses they were scored on. They can motivate hypotheses but cannot confirm them.

## Objective

Given a rigged body and an unrigged garment fitted to it, compute skinning weights for the garment automatically so that it follows the body through new poses with less interpenetration, tearing and exposed body than two baselines: closest-point transfer and Epic's weight inpainting (Abdrashitov et al. 2023, DOI 10.1145/3610543.3626180).

## Scope

- **In scope:** per-vertex weight transfer methods and their parameters; scoring under linear blend skinning (LBS) and dual quaternion skinning (DQS); the two synthetic capsule bodies and the MakeHuman body (CC0) with their garments.
- **Out of scope:** cloth simulation; per-pose correctives (displacements learned or authored on top of skinning), which the last experiment proposed as the next line. That is a different question and would be opened as its own RQ if the director wants it.
- **Decision needed:** the hem rig adds bones to the garment, so it changes the rig rather than only transferring weights. It is in scope only if the director says so.

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

1. A method answers the question if, on a fresh set of held-out poses never used for any tuning, it is no worse than the better baseline by more than 5% (relative) on any primary metric for any garment, and better on at least one primary metric for the garments that bridge the legs (skirt and dress), on both the synthetic and the MakeHuman bodies.
2. The result is reproduced by a separate review session from the recorded commit.
3. A negative answer is also a completion: if no method meets (1), the question closes with that finding and its limits.

The 5% threshold is the keep rule the loop used. It was broken twice, in the consistency-check and makehuman-body experiments, with a note that the rule needed revisiting, so it should be confirmed or replaced here.

## What the exploratory experiments suggest

These are leads, not findings. Each is one deterministic run, tuned on its test poses, on one garment per case.

- Kin-limited blending (`kinblend`, the current default) fixes the shirt's armpit leak on MakeHuman, but makes the capsule dress worse.
- On the capsule bodies, the hem rig at full follow gave the first skirt and dress with no exposure and no penetration.
- On the real (MakeHuman) skirt, plain `kinblend` keeps exposure lowest (11% against 60% for the hem rig as first driven, 14% after side drivers), and nothing tried handles the sit pose.
- Earlier "beats both baselines" calls rested on metrics or bodies that were later replaced (body v1; worst-pose-only scoring before exposure existed). They need re-checking under the final metrics.

## Questions for Gate A

1. Is the objective above the one you want to pursue?
2. Is the hem rig in scope, or should RQ-001 be weight transfer only?
3. Do you accept the proposed primary metrics and their per-pose reporting?
4. Do you accept the completion criteria, including the 5% threshold?
5. Do you want a second real body before any conclusion about real garments?

## Gate log

Only the research director's decisions go here.

| Date | Gate | Decision | By | Note |
|------|------|----------|----|------|
| | A | | | |
