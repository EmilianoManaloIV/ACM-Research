# Decisions

Research and architecture decisions, newest first. Each entry says what was decided, what else was considered, the evidence, and who approved it. A default method, metric, benchmark or research objective changes only through an entry here.

## D-006 Body-weighting benchmark settings after the first review (2026-10-10)

- **Decision:** for RQ-002 body weighting, from experiment `2026-10-10-auto-weight-baselines-v2` on:
  - every mesh is welded (vertices at the same position merged) before any method runs;
  - BBW runs at libigl's default of 100 active-set iterations;
  - weights above -1e-6 count as zero before the validity check;
  - self-intersection counts only faces that cross, not faces that touch;
  - characters are grouped by skeleton topology (biped, quadruped, other), not by dataset label.
- **Considered:** keeping the first protocol's settings (split meshes, 8 iterations, a -1e-9 clip, contact counted as intersection, label groups), under which the first run's self-intersection was unreadable on 3 characters and the groups did not match body plans.
- **Evidence:** independent review on PR #6, and the exploratory welded runs in `experiments/2026-10-10-auto-weight-baselines/`.
- **Approved by:** Emiliano (Gate C for the amended protocol). He also judged that the first run does not pass H1.

## D-005 Broaden the research to bodies and clothes of any creature (2026-10-10)

- **Decision:** the research covers automatic weighting of bodies as well as clothes, for bipeds, quadrupeds and unusual creatures. RQ-001 (clothing) keeps the leg-driven hem rig in scope, uses per-pose penetration, exposure and stretch p95 as primary metrics, binds the 5% keep rule, and requires a second real human body. RQ-002 is drafted for body weighting.
- **Considered:** keeping RQ-001 to human garments only, as the first 20 experiments did.
- **Approved by:** Emiliano (answers to the RQ-001 Gate A questions, then both objectives confirmed as written). RQ-002 goes first, because RQ-001 needs weighted quadruped and creature bodies.

## D-004 Commit each experiment's results table (2026-10-10)

- **Decision:** every experiment commits its small `results.csv` next to its README. Meshes, posed outputs and other large files stay in `data/`, which git ignores.
- **Considered:** keeping all outputs in `data/` and copying tables into the README by hand, as experiments did until now.
- **Evidence:** none of the first 16 experiments on the experiment branch kept its raw table, so old results cannot be checked without regenerating them, and several can no longer be regenerated at the current code.
- **Approved by:** Emiliano

## D-003 Workflow files live on main (2026-10-10)

- **Decision:** `CLAUDE.md`, `research/` and the templates land on `main` in their own PR, separate from experiment code.
- **Considered:** adding them to the experiment branch.
- **Approved by:** Emiliano

## D-002 The running experiment loop finishes under its own plan (2026-10-10)

- **Decision:** the experiment loop on draft PR #2 (garment skin-weight transfer) finishes its current plan. The gated workflow applies to work started after it completes, beginning with reconstructing its question file and an independent re-check of its current default.
- **Considered:** pausing the loop after its current experiment; leaving it running alongside the new workflow.
- **Approved by:** Emiliano

## D-001 Adopt a minimal, human-gated research workflow (2026-10-10)

- **Decision:** one research session carries the coordinator, literature, design and engineering roles as explicit stages; a separate session reviews and re-runs results; the research director approves gates A to F. Further agents are added only if a measured, recurring failure calls for one.
- **Considered:** five separate agents, one per role.
- **Evidence:** the audit and proposal in the project thread "Research architecture: audit and proposal" (2026-10-10). In short: Agent Laboratory (Schmidgall et al., Findings of EMNLP 2025) and the Virtual Lab (Swanson et al., Nature 2025) report that human input improved outcomes; Huang et al. (ICLR 2024) find that a model correcting itself without outside feedback does not reliably help, so review needs a separate session and a re-run; Cemri et al. (arXiv 2503.13657) catalogue failures that come from coordinating many agents. These sources were checked through listings and abstracts only.
- **Approved by:** Emiliano
