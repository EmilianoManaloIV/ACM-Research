# Bounded biharmonic weights for real-time deformation

- **Authors:** Alec Jacobson, Ilya Baran, Jovan Popović, Olga Sorkine
- **Venue / year:** ACM Transactions on Graphics 30(4), article 78, SIGGRAPH 2011
- **Link:** DOI [10.1145/1964921.1964973](https://doi.org/10.1145/1964921.1964973)
- **Area:** graphics
- **Tags:** skinning weights, biharmonic, bounded
- **Read:** abstract only (search listings)
- **Verified:** 2026-10-10, DOI and author copy via web search listings

## Summary

Computes smooth skinning weights for points, bones and cages by minimising a biharmonic energy subject to bounds and partition of unity.

## Key ideas

- Weights stay in [0, 1] and sum to one by construction.
- Needs a volumetric (tetrahedral) mesh of the interior.

## Results

Not read.

## Limitations and open questions

- SkinCells (Larionov et al.) reports that BBW depends on tetrahedralisation, which hurts robustness on production meshes, and that clamping to four influences causes tearing.
- The geodesic cut-cell preprint calls it expensive because of volumetric meshing and linear solves.

## Relevance to our work

libigl, already a project dependency, implements BBW, so it is the cheapest baseline to add. Its robustness on messy creature meshes is itself a testable question.
