# Data

Datasets, captures and generated outputs live here and are ignored by git (except this file). Document where each dataset comes from so it can be re-downloaded.

## Datasets

- `synthetic/` — procedural capsule bodies, garments and pose sets for the
  weight-transfer benchmark. Regenerate with `python3 tools/make_synthetic.py`.
  Body v2 (2026-10-10, ringed capsules with a centred blend at each joint)
  is the current version; experiments before `synthetic-body-v2` used v1.
- `makehuman/` — the MakeHuman base mesh, default skeleton and skin weights
  (CC0 1.0, from https://github.com/makehumancommunity/makehuman, files
  `makehuman/data/3dobjs/base.obj`, `rigs/default.mhskel`,
  `rigs/default_weights.mhw`; raw copies in `makehuman/raw/`), rebuilt into
  a rigged glTF body and six garments by `python3 tools/make_makehuman.py`.
  Results from `tools/run_benchmark.sh out all lbs makehuman` go to
  `results_mh/` (and `results_mh_dqs/`).
- `articulation-xl2/` — Articulation-XL2.0 (CC-BY 4.0 as a dataset; each
  model keeps its own licence), from
  https://huggingface.co/datasets/Seed3D/Articulation-XL2.0 at revision
  `11f777d83a1e7ee7c5a22201da4e140cafac1837`: `meta_Articulation_XL_2.0.csv`
  saved as `meta.csv`, and `articulation_xlv2_test.npz` (1,154,853,607
  bytes). `characters/` holds the RQ-002 characters converted by
  `python3 tools/fetch_articulation.py convert data
  experiments/2026-10-10-auto-weight-baselines/characters.csv
  data/articulation-xl2/characters`.
- `objaverse-xl/` — per-object licences, `sketchfab/sketchfab.parquet` and
  `github/github.parquet` from https://huggingface.co/datasets/allenai/objaverse-xl,
  used only to filter Articulation-XL2.0 by licence.
- `results/` — benchmark output from `tools/run_benchmark.sh` (`results_dqs/` for the dual quaternion run).
