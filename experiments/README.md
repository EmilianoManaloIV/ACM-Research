# Experiments

One folder per experiment, named `YYYY-MM-DD-short-name/`. Copy [`_template/`](_template) to start a new one.

Each experiment keeps its own `README.md` with the hypothesis, method and results, plus any scripts or configs it needs. Shared, reusable code belongs in `src/`; large inputs and outputs go in `data/` (ignored by git).

## Log

| Date | Experiment | Status | Outcome |
|------|------------|--------|---------|
| 2026-10-10 | [weight-transfer-baselines](2026-10-10-weight-transfer-baselines) | done | Benchmark set up; closest point and Epic inpainting scored on 6 synthetic garments |
| 2026-10-10 | [bounded-inpainting](2026-10-10-bounded-inpainting) | done | Weights kept in [0, 1]; pose metrics unchanged; kept as default solve |
| 2026-10-10 | [anchored-components](2026-10-10-anchored-components) | done | Detached pieces no longer make the solve singular; button still follows the wrong bone |
| 2026-10-10 | [relative-matching](2026-10-10-relative-matching) | done, not adopted | Fixes the loose sleeve but tears the skirt and dress between the legs; needs M4 first |
| 2026-10-10 | [consistency-check](2026-10-10-consistency-check) | done | Drops matches where the closest point jumps between legs; first method to beat both baselines on the skirt; kept as default |
| 2026-10-10 | [offset-blend](2026-10-10-offset-blend) | done, not adopted | Wider blend on loose garments increases LBS collapse at the elbow; the limit is the skinning model, not the weights |
| 2026-10-10 | [skinning-model](2026-10-10-skinning-model) | done | Benchmark scores under LBS and DQS; DQS punishes the wide blends LBS rewarded; synthetic bodies found to be hard-skinned at the joints |
