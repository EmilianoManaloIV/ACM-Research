# Working in ACM-Research

This file holds the standing instructions for any Claude session in this repository. The research director (Emiliano) owns every scientific conclusion, change of direction and gate decision. Sessions prepare material and ask; they never pass a gate themselves.

## Where things live

```
research/questions/   One file per research question (RQ-NNN): objective, scope, metrics, gate log
research/evidence/    One file per question: search log and evidence table
research/decisions.md Decision log for research and architecture decisions, newest first
papers/               One note per paper, from papers/_template.md
experiments/          One dated folder per experiment: protocol.md, README.md (record), review.md, results.csv
src/ apps/ tests/     Code; shared code in src/acm/
data/                 Large inputs and outputs, ignored by git; provenance in data/README.md
```

Keep the existing conventions in `README.md`. Do not move or restructure files without a decision log entry the director approved.

## Starting a session

Read this file, the open question file in `research/questions/` (its gate log says where the work stands), and the latest entries in `research/decisions.md`. Those are the project's memory; conversations are not.

## Lifecycle and gates

Question → literature and evidence → hypotheses and protocol → implementation → runs → independent review → write-up → acceptance, with a gate after each stage:

| Gate | Passed when this exists and the director has approved it in the question's gate log |
|------|------------------------------------------------------------------------------------|
| A, question | Question file: objective, scope, definitions, measurable completion criteria |
| B, evidence | Search log, paper notes with read status, evidence table, open uncertainties |
| C, protocol | `protocol.md`: competing hypotheses, metrics, baselines, splits, keep and stopping rules. Nothing confirmatory runs before this. |
| D, implementation | PR with the change, tests that were actually run (with output), files touched, any default change called out |
| E, results | Experiment record, comparison against the protocol's baselines, `review.md` from a separate review session |
| F, acceptance | Write-up whose claims each cite an experiment and metric, with limitations and next steps |

A gate can send work back to any earlier stage. Prepare the gate's material, then ask the director; record the decision only as the director gave it.

## Integrity rules

- Never invent a citation, finding, dataset or benchmark. Give a DOI or stable URL. Say in each paper note whether the full text or only the abstract was read.
- Separate established evidence, interpretation, hypothesis and speculation. A generated hypothesis is not a finding.
- Never describe a test or run as passing unless it ran in this session and the output is recorded. Say what was not verified.
- Never change a default method, metric, benchmark or a question's objective without an approved entry in `research/decisions.md`.
- Record disagreements between sources and threats to validity rather than smoothing them over.

## Experiment rules

- Copy `experiments/_template/`. Write and commit `protocol.md` before the first confirmatory run; after Gate C it is frozen, and changes go in a dated "Deviations" section.
- Tune parameters on training data only. Test data that was used for tuning no longer counts as held out.
- Label runs exploratory or confirmatory. Only confirmatory runs can support a hypothesis.
- Fill the reproducibility record at the top of the experiment README: commit (and whether the tree had uncommitted changes), exact command, environment, data version, raw output location.
- Commit the experiment's small `results.csv` next to its README. Large outputs stay in `data/`.
- Report per-item results, not only the worst case, and state when a result comes from a single deterministic run.
- Stop at each gate, and stop after the number of runs the protocol allows.

## Independent review

Review happens in a separate session that did not do the work. It receives the protocol, the PR diff, the experiment record and the raw outputs, not the research session's reasoning. It must:

1. Check out the recorded commit cleanly and re-run the recorded command. If it cannot, say so; an unrun review is an opinion, not a verification.
2. Compare the numbers with the record and the claims with the protocol's keep rule.
3. Run the tests and list what they do and do not cover.
4. List alternative explanations, confounds and protocol deviations.
5. Recommend accept, revise or reject in `review.md`, with reasons.

Re-reading one's own work in the same session is not independent review.
