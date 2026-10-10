# Experiments

One folder per experiment, named `YYYY-MM-DD-short-name/`. Copy [`_template/`](_template) to start a new one.

Each experiment keeps its own `README.md` with the hypothesis, method and results, plus any scripts or configs it needs. Shared, reusable code belongs in `src/`; large inputs and outputs go in `data/` (ignored by git).

## Log

Rows up to and including skinning-model were measured on synthetic body v1 (hard-skinned joints, see synthetic-body-v2); later rows use body v2.

| Date | Experiment | Status | Outcome |
|------|------------|--------|---------|
| 2026-10-10 | [weight-transfer-baselines](2026-10-10-weight-transfer-baselines) | done | Benchmark set up; closest point and Epic inpainting scored on 6 synthetic garments |
| 2026-10-10 | [bounded-inpainting](2026-10-10-bounded-inpainting) | done | Weights kept in [0, 1]; pose metrics unchanged; kept as default solve |
| 2026-10-10 | [anchored-components](2026-10-10-anchored-components) | done | Detached pieces no longer make the solve singular; button still follows the wrong bone |
| 2026-10-10 | [relative-matching](2026-10-10-relative-matching) | done, not adopted | Fixes the loose sleeve but tears the skirt and dress between the legs; needs M4 first |
| 2026-10-10 | [consistency-check](2026-10-10-consistency-check) | done | Drops matches where the closest point jumps between legs; first method to beat both baselines on the skirt; kept as default |
| 2026-10-10 | [offset-blend](2026-10-10-offset-blend) | done, not adopted | Wider blend on loose garments increases LBS collapse at the elbow; the limit is the skinning model, not the weights |
| 2026-10-10 | [skinning-model](2026-10-10-skinning-model) | done | Benchmark scores under LBS and DQS; DQS punishes the wide blends LBS rewarded; synthetic bodies found to be hard-skinned at the joints |
| 2026-10-10 | [synthetic-body-v2](2026-10-10-synthetic-body-v2) | done | Bodies regenerated with ringed capsules and a centred blend; every method re-run under LBS and DQS; skirt call stands, dress call flips to closest point, offset blend works under DQS |
| 2026-10-10 | [offset-blend-v2](2026-10-10-offset-blend-v2) | done | Offset-scaled blend re-swept on body v2: scale 4 improves both primary metrics on every garment under DQS and all but one under LBS; now the default method |
| 2026-10-10 | [joint-capped-blend](2026-10-10-joint-capped-blend) | done, not adopted | Capping the blend region at two bones breaks the dress (its useful blend spans three) and does not stop the sleeve overshoot (a length problem, not a bone count) |
| 2026-10-10 | [length-capped-blend](2026-10-10-length-capped-blend) | done, not adopted | Capping the blend at a fraction of the dominant bone length fixes the loose sleeve under DQS but the pelvis is too short a bone for the dress; the dress needs pose-aware blending (M6) |
