# Experiments

One folder per experiment, named `YYYY-MM-DD-short-name/`. Copy [`_template/`](_template) to start a new one.

Each experiment keeps a `protocol.md` written before it runs, a `README.md` with its reproducibility record and results, a `review.md` from the independent review, and its `results.csv`, plus any scripts or configs it needs. The stages and gates are in [`CLAUDE.md`](../CLAUDE.md). Shared, reusable code belongs in `src/`; large inputs and outputs go in `data/` (ignored by git).

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
| 2026-10-10 | [makehuman-body](2026-10-10-makehuman-body) | done | First real body (MakeHuman, 163 joints, CC0) with six garments: offset blend holds on limbs, leaks across the armpit on a shirt, and no method handles a skirt bridging the legs; Epic's threshold fails a loose sleeve |
| 2026-10-10 | [kin-blend](2026-10-10-kin-blend) | done | Blend limited to bones within two hierarchy steps: fixes the shirt's armpit leak, identical elsewhere, costs the capsule dress; now the default |
| 2026-10-10 | [influence-limit](2026-10-10-influence-limit) | done, not adopted | Limiting influences inside the solve matches post-hoc pruning to the second decimal and the smoothing undoes it; real-rig tables will use --prune 8 |
| 2026-10-10 | [pose-aware-refinement](2026-10-10-pose-aware-refinement) | done, optional | Relax each training pose and refit LBS weights: small stretch gains, but the fit residual equals the correction on the skirt, so no per-vertex weight reproduces a relaxed skirt; kept behind --refine |
| 2026-10-10 | [hem-rig](2026-10-10-hem-rig) | done | Leg-driven hem bones: first change to improve a garment bridging the legs on both bodies; `follow` dials penetration against stretch; kept for skirts and dresses |
| 2026-10-10 | [hem-rig-v2](2026-10-10-hem-rig-v2) | done | Gravity lower tier adopted (dress p95 0.92 to 0.80, torn 3.6% to 0.8%); per-hip pivot does nothing; thigh contact worsens the real skirt; a rigid hem scores best, which exposes a blind spot in the penetration metric |
| 2026-10-10 | [exposure-score](2026-10-10-exposure-score) | done | Leg-exposure score (covered body vertices outside the posed cloth): the rigid hem was a blind spot (70% exposed); full follow makes the capsule skirt and dress exposure- and penetration-free; the hem rig as driven exposes 60% of the real thighs against 11% for kinblend |
| 2026-10-10 | [hem-rig-v3](2026-10-10-hem-rig-v3) | done | Side-driven bones cut real-skirt exposure 58% to 14% and a waistband rule halves its penetration, but plain kinblend still wins the real skirt on exposure; both kept as per-body options, benchmark scores skirts with and without the rig |
| 2026-10-10 | [rig-transfer-blend](2026-10-10-rig-transfer-blend) | done, not adopted | Per-vertex blend of rig and transfer weights slides between the two on the real skirt without beating either; closes the hem-rig line (capsules: rig at full follow; real skirt: kinblend on exposure, sit pose unsolved) |
| 2026-10-10 | [auto-weight-baselines](2026-10-10-auto-weight-baselines) | planned, protocol at Gate C | RQ-002: bone heat, bounded biharmonic weights and an inverse-distance floor on humans, quadrupeds and creatures |
