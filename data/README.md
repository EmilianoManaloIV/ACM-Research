# Data

Datasets, captures and generated outputs live here and are ignored by git (except this file). Document where each dataset comes from so it can be re-downloaded.

## Datasets

- `synthetic/` — procedural capsule bodies, garments and pose sets for the
  weight-transfer benchmark. Regenerate with `python3 tools/make_synthetic.py`.
  Body v2 (2026-10-10, ringed capsules with a centred blend at each joint)
  is the current version; experiments before `synthetic-body-v2` used v1.
- `results/` — benchmark output from `tools/run_benchmark.sh` (`results_dqs/` for the dual quaternion run).
