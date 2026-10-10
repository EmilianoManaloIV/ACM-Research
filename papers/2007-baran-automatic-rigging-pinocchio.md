# Automatic rigging and animation of 3D characters (Pinocchio)

- **Authors:** Ilya Baran, Jovan Popović
- **Venue / year:** ACM Transactions on Graphics 26(3), article 72, SIGGRAPH 2007
- **Link:** [Author copy](https://people.csail.mit.edu/ibaran/papers/2007-SIGGRAPH-Pinocchio.pdf); DOI not confirmed
- **Area:** graphics
- **Tags:** automatic rigging, skinning weights, bone heat
- **Read:** abstract only (search listings)
- **Verified:** 2026-10-10, author copy URL and SIGGRAPH history listing via web search; the ACM DOI could not be resolved from this environment

## Summary

Given a static character mesh and a generic skeleton, Pinocchio fits the skeleton into the mesh and attaches the surface to it, so motion data can animate the character. The prototype rigs a character in under a minute.

## Key ideas

- Skeleton embedding and surface attachment are solved separately.
- The attachment step is widely known as "bone heat" weighting, and Blender's automatic weights descend from it. That attribution comes from Blender developer discussions, not from the paper text, which I have not read.

## Results

Not read.

## Limitations and open questions

- Relies on a predefined skeleton template (per the RigAnything paper's related work).
- Blender users report "Bone Heat Weighting: Failed to find solution" on some meshes, which a later paper attributes to the finite-element solve.

## Relevance to our work

The de facto default for automatic weights, so the first baseline for RQ-002. Blender's implementation can serve as a reference implementation.
