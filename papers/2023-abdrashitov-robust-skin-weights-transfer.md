# Robust Skin Weights Transfer via Weight Inpainting

- **Authors:** Rinat Abdrashitov, Kim Raichstat, Jared Monsen, David Hill (Epic Games)
- **Venue / year:** SIGGRAPH Asia 2023 Technical Communications
- **Link:** https://doi.org/10.1145/3610543.3626180 · code: https://github.com/rin-23/RobustSkinWeightsTransferCode (MIT)
- **Area:** graphics
- **Tags:** skinning, weight transfer, garments, inpainting

## Summary

Copies skin weights from a rigged body to a garment. Existing tools use some
form of closest-point lookup, which breaks on loose clothing. The method only
copies where the closest point is trustworthy and fills in the rest by solving
for the smoothest weights that agree with the copied ones. Unreal's Chaos
Cloth ships it as the InpaintWeights option.

## Key ideas

- **Match** each garment vertex to its closest point on the body; accept it only
  if the distance is at most 5% of the garment's bounding-box diagonal and the
  normals differ by at most 30 degrees.
- **Inpaint** unmatched vertices by minimising a biharmonic-style energy
  `w^T (-L + L M^-1 L) w` with `L = 2 * cotmatrix` and `M` the Voronoi mass
  matrix, holding matched vertices fixed (libigl `min_quad_with_fixed`).
- **Smooth** the inpainted region and its neighbours within the distance
  threshold: 10 passes of `w <- 0.8 w + 0.2 mean(neighbours)`, updated in place.

## Results

The paper shows garments on a body with fewer artifacts than closest point and
commercial tools; the released code only ships a sphere-to-plane toy. No
quantitative metrics are reported in the materials available to us.

## Limitations and open questions

- One global distance threshold, so loose regions get few matches and are
  filled by inpainting alone, which tends to lock them to a single bone.
- Nothing bounds the solve: weights can go below 0 or above 1, and the number
  of bones per vertex is uncontrolled.
- Any connected component with no matched vertex makes the system singular,
  so garments with detached pieces (buttons, belts) fail outright.
- Poses are never consulted.
- The released C++ port uses `L = cotmatrix`, not `2 * cotmatrix` as the Python
  does, so the two give slightly different inpainted weights; we follow the Python.

## Relevance to our work

The M1 baseline in `experiments/2026-10-10-weight-transfer-baselines`. Our
C++ port (`src/acm/skin/transfer.cpp`, method `inpaint`) matches the Python
reference to 1e-13 on the sphere-to-plane example (`tools/verify_inpaint.py`).
