# Geodesic voxel binding for production character meshes

- **Authors:** Olivier Dionne, Martin de Lasa
- **Venue / year:** ACM SIGGRAPH / Eurographics Symposium on Computer Animation 2013, pp. 173–180
- **Link:** DOI [10.1145/2485895.2485919](https://doi.org/10.1145/2485895.2485919)
- **Area:** graphics
- **Tags:** skinning weights, voxels, geodesic distance, production meshes
- **Read:** abstract only (search listings)
- **Verified:** 2026-10-10, Eurographics Digital Library listing via web search

## Summary

Voxelises the rest-pose character and derives skinning weights from geodesic distances through the voxel interior, so it tolerates the defects production meshes have.

## Key ideas

- Works on non-manifold, multi-shell meshes because it never needs a clean volume mesh.
- Shipped in Maya as "geodesic voxel bind".

## Results

Not read. Secondary reports: RigNet says its parameters had to be tuned per shape to match artist weights; a geodesic cut-cell preprint says voxelisation merges parts that are close in space but far apart along the body.

## Limitations and open questions

- Voxel resolution trades detail against merging, which matters for tails, wings and fingers close together.

## Relevance to our work

The production baseline built for messy meshes, which is exactly what creature assets look like. No open implementation is known; reimplementing it is a Gate C decision.
