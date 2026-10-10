# RQ-002: Automatic skin weights for bipeds, quadrupeds and creatures

- **Opened:** 2026-10-10, drafted from the director's Gate A answer on RQ-001
- **Current gate:** A, awaiting the research director's decision
- **Evidence:** `research/evidence/RQ-002.md` (not started; Gate B)
- **Experiments:** none yet
- **Companion question:** [RQ-001](RQ-001-garment-weight-transfer.md), weights for clothing on these characters

## Objective

Given a character mesh and a skeleton already placed inside it, compute the mesh's skinning weights automatically, with no existing weights to transfer from, so that the character deforms well through new poses across body plans: bipeds, quadrupeds and unusual creatures (extra limbs, tails, wings, non-human proportions). The result should beat established automatic methods on the same characters.

Placing the skeleton (automatic rigging) is out of scope unless the director adds it; the skeleton is an input.

## Scope

- **In scope:** geometric and learned weight-computation methods; production-quality meshes with the defects real assets have (non-manifold parts, overlapping shells, separate eyes and teeth); scoring under LBS and DQS.
- **Out of scope:** skeleton placement; correctives and muscle or cloth simulation.

## Candidate baselines

Verified through search listings and abstracts only on 2026-10-10; none read in full.

| Method | Reference | Why it is a baseline |
|--------|-----------|----------------------|
| Bone heat (Pinocchio) | Baran and Popović 2007, Automatic rigging and animation of 3D characters, ACM TOG 26(3), article 72 ([author copy](https://people.csail.mit.edu/ibaran/papers/2007-SIGGRAPH-Pinocchio.pdf)); DOI not confirmed | The method behind Blender's "automatic weights", so the de facto default |
| Bounded biharmonic weights | Jacobson, Baran, Popović and Sorkine 2011, ACM TOG 30(4), article 78, DOI 10.1145/1964921.1964973 | Smooth, bounded weights; libigl, already a dependency, implements it |
| Geodesic voxel binding | Dionne and de Lasa 2013, SCA 2013, pp. 173–180, DOI 10.1145/2485895.2485919 | Built for imperfect production meshes; the method behind Maya's geodesic voxel bind |
| RigNet (learned) | Xu, Zhou, Kalogerakis, Landreth and Singh 2020, ACM TOG 39(4), [arXiv 2005.00559](https://arxiv.org/abs/2005.00559) | Trained on 2,703 rigged characters including quadrupeds, birds, fish and fictional creatures; released code and data |

## Definitions

To be fixed at Gate A. Proposed: the metrics of RQ-001 that apply to a single mesh (stretch p95, mean stretch, torn fraction, self-intersection in place of body penetration), plus agreement with artist-painted weights where the test character has them, reported separately because artist weights are a reference, not ground truth.

## Data

- **Proposed:** the MakeHuman body (artist weights exist, so agreement can be measured), a second human body, and quadruped and creature characters with skeletons and, where possible, artist weights.
- **Open:** a licensed source for quadrupeds and creatures. RigNet's ModelsResource-RigNetv1 set is the obvious candidate, but its licence has not been checked; the evidence pass at Gate B should settle it.

## Completion criteria

Proposed, mirroring RQ-001: on held-out poses never used for tuning, a method is no worse than the best baseline by more than 5% (relative) on any primary metric for any character, and better on at least one primary metric for each body plan (biped, quadruped, creature). A negative answer also completes the question.

## Open for Gate A

1. Is the objective right, with skeleton placement out of scope?
2. Which should come first, RQ-001 (clothing) or RQ-002 (bodies)? They share data: RQ-001 needs weighted quadruped and creature bodies, which RQ-002 would produce.
3. Do you accept the proposed metrics, including artist-weight agreement as a separate reference measure?

## Gate log

Only the research director's decisions go here.

| Date | Gate | Decision | By | Note |
|------|------|----------|----|------|
| | A | | | |
