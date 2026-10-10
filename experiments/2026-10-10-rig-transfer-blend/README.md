# Experiment: blending hem-rig and transfer weights per vertex

- **Date started:** 2026-10-10
- **Status:** done, not adopted (kept as the `blend` option); closes the hem-rig line
- **Related papers:** none in `papers/`

## Question and hypothesis

The hem rig gives the smooth field between the legs that no per-vertex
transfer can, and on the capsule garments it is the only thing that ever
reached zero penetration and zero exposure. On the real skirt the plain
`kinblend` transfer still wins the leg score (11.0% exposed against the
best rig's 15.6%): its vertices nearest a thigh follow that thigh, where
the rig's eight rigid panels cannot bend around a real thigh. If each
vertex takes the transfer's weights when it sits close to a thigh and
the rig's when it bridges the gap, the two should add up.

## Method

`"blend": b` in `hem.json`. For each garment vertex the rest gap to the
nearest thigh surface is measured (distance to the hip-knee axis minus the
thigh radius from `measureThighs`, in thigh radii); a vertex at gap 0
keeps the transfer's weights, one at gap b or more takes the rig's, with a
linear ramp between. Sweep b in 0.25, 0.5, 1, 2 on each body's current
best rig (MakeHuman: follow 1.0, side drive, waist 0.2; capsules: follow
1.0, azimuth drive), `kinblend --prune 8` underneath. Cells: penetration %
/ stretch p95 / stretch mean / torn edges / exposed body, worst test pose.

## Results

| Garment | kinblend, no rig | rig only | b 0.25 | b 0.5 | b 1.0 | b 2.0 |
| --- | --- | --- | --- | --- | --- | --- |
| Real skirt | 8.1 / 2.56 / 0.45 / 12.7% / **11.0%** | 6.7 / 3.01 / 0.51 / 15.1% / 15.6% | 7.1 / 3.01 / 0.53 / 14.9% / 13.9% | 7.2 / 2.99 / 0.51 / 14.2% / 13.8% | 6.7 / 2.86 / 0.50 / 14.1% / 12.5% | 6.8 / 2.75 / 0.49 / 13.8% / 12.6% |
| Capsule dress | 7.4 / 1.11 / 0.30 / 6.2% / 35.5% | **0.3 / 1.49 / 0.47 / 17.6% / 1.2%** | 0.3 / 1.49 / 0.47 / 17.6% / 1.2% | 0.3 / 1.49 / 0.47 / 17.6% / 1.2% | 0.4 / 1.49 / 0.47 / 17.6% / 1.2% | 1.0 / 1.49 / 0.48 / 19.2% / 1.2% |
| Capsule skirt | 4.5 / 0.51 / 0.21 / 0.5% / 14.0% | **0.0 / 0.75 / 0.22 / 1.2% / 0.0%** | 0.0 / 0.75 / 0.22 / 1.2% / 0.0% | 0.0 / 0.75 / 0.22 / 1.2% / 0.0% | 0.0 / 0.75 / 0.22 / 1.2% / 0.0% | 1.4 / 0.75 / 0.23 / 1.2% / 2.2% |

- **On the real skirt the blend slides between the two parents without
  beating either**: as b grows the numbers move from the rig's toward
  `kinblend`'s (exposure 15.6% to 12.5%, p95 3.01 to 2.75) and stop short
  of them. The skirt sits so close to the thighs (panel radius 1.1 to 2.2
  dm against a thigh of 0.88 at 0.55 from the centre line) that most of
  it is within one thigh radius of a thigh, so there is no "between the
  legs" region for the rig to own: the real skirt has no panel bridging
  the legs the way the capsule dress does.
- The sit pose decides the real skirt under every method tried (rig,
  transfer, blend: 12.5 to 15.6% exposed); the thigh rotating 90 degrees
  under a close-fitting panel is not reproduced by any rigid motion of
  that panel, and the transfer's answer (tear the panel between the legs)
  is what keeps the thighs clad.
- On the capsules the blend is a no-op up to b 1 (the rig region is far
  from the thighs anyway) and harmful at b 2 (the transfer's leg weights
  reach the bridging panel again).

## Conclusions and next steps

- **Not adopted**: `blend` stays an option at 0. The hem-rig line stops
  here, at the point where it stands: on the synthetic bodies the rig at
  full follow is the first exposure-free and penetration-free skirt and
  dress; on the real skirt plain `kinblend` keeps the leg score, the rig
  keeps penetration, and nothing tried closes the sit pose.
- What would move the real skirt is not another driver: it is cloth
  that can stretch over the thigh in a sit, which linear blend skinning
  with rigid bones cannot represent. A per-pose corrective (a learned or
  example-based displacement on top of the skinning) is the next line,
  and a different one from weight transfer.
- This closes the current line; no further lines are opened until the
  stage-gated research process approved in the "Research R&D architecture
  audit" thread lands on main.

## Reproducibility

- Commit: `6292c75` (branch `claude/project-thread-27uwpy`); code paths
  `src/acm/skin/hem.cpp` (`blendWithTransfer`), `apps/weightbench`.
- Environment: Ubuntu 24.04 container, x86_64, 4 cores, g++ 13.3.0, cmake
  3.28.3, Release build; libigl 2.5, Eigen and tinygltf via FetchContent.
- Data: `python3 tools/make_synthetic.py` and `python3 tools/make_makehuman.py`
  (MakeHuman base mesh, CC0); poses from each body's `poses.json`, test set.
- Every number is a single deterministic run (no randomness in the
  pipeline; the solver is direct). Results CSVs for this sweep were kept
  in the session scratch, not committed; the benchmark script
  (`tools/run_benchmark.sh out all lbs makehuman`) reproduces the table's
  `kinblend` and `+hem` rows.

## Limitations (flagged by the architecture audit, apply to this whole line)

- `follow`, `falloff`, `drive`, `waist` and `blend` were chosen on the
  same test poses the tables score, so the reported numbers are
  optimistic for those poses; the `train` set in `poses.json` exists but
  this line did not use it for selection.
- Each cell is one deterministic run on one garment per body; there is no
  variance estimate and no second real body, so the real-skirt
  conclusions rest on a single mesh.
