# RQ-002: Automatic skin weights for bipeds, quadrupeds and creatures

- **Opened:** 2026-10-10, drafted from the director's Gate A answer on RQ-001
- **Current gate:** D for the amended protocol (experiment `2026-10-10-auto-weight-baselines-v2`, Gate C passed 2026-10-10). The first experiment was sent back from Gate E: run 2 does not pass H1
- **Evidence:** [`research/evidence/RQ-002.md`](../evidence/RQ-002.md), ready for Gate B
- **Experiments:** [auto-weight-baselines](../../experiments/2026-10-10-auto-weight-baselines/protocol.md) (protocol approved; implementation in progress)
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

Approved at Gate A: the metrics of RQ-001 that apply to a single mesh (stretch p95, mean stretch, torn fraction, self-intersection in place of body penetration), plus agreement with artist-painted weights where the test character has them, reported separately because artist weights are a reference, not ground truth.

## Data

- **Proposed:** the MakeHuman body (artist weights exist, so agreement can be measured), a second human body, and quadruped and creature characters with skeletons and, where possible, artist weights.
- **Open:** a licensed source for quadrupeds and creatures. RigNet's ModelsResource-RigNetv1 set is the obvious candidate, but its licence has not been checked; the evidence pass at Gate B should settle it.

## Completion criteria

Proposed, mirroring RQ-001: on held-out poses never used for tuning, a method is no worse than the best baseline by more than 5% (relative) on any primary metric for any character, and better on at least one primary metric for each body plan (biped, quadruped, creature). A negative answer also completes the question.

## Gate log

Only the research director's decisions go here.

| Date | Gate | Decision | By | Note |
|------|------|----------|----|------|
| 2026-10-10 | A | Passed: objective as written, skeleton placement out of scope; goes before RQ-001; artist-weight agreement reported as a separate reference measure | Emiliano | |
| 2026-10-10 | B | Passed: evidence accepted; proceed to the first protocol. Asked to build on released models from papers and companies as well | Emiliano | Learned models planned as the second experiment |
| 2026-10-10 | C | Passed: auto-weight-baselines protocol approved; huggingface.co to be allowed for Articulation-XL2.0; a GPU machine is available later for learned models | Emiliano | |
| 2026-10-10 | D | Passed: auto-weight-baselines harness (PR #5); overnight protocol deviations approved (group wording, 4 quadrupeds, test split only, re-run after two harness fixes); the 8-iteration BBW cap recorded as a deviation | Emiliano | Answers to the coordinator's eight consolidated questions |
| 2026-10-10 | E | Sent back to C: "Overnight run concludes that H1 has not passed"; the amended run is the confirmatory test | Emiliano | Independent review (PR #6): reproduced, recommended revise |
| 2026-10-10 | C | Passed: amended protocol (auto-weight-baselines-v2): welded meshes for every method, BBW at 100 iterations, explicit negative-weight tolerance, self-intersection that ignores contact; same and fresh characters reported separately; groups by skeleton topology; c84ca8a7 dropped | Emiliano | |
