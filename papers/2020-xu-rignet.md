# RigNet: Neural rigging for articulated characters

- **Authors:** Zhan Xu, Yang Zhou, Evangelos Kalogerakis, Chris Landreth, Karan Singh
- **Venue / year:** ACM Transactions on Graphics 39(4), SIGGRAPH 2020
- **Link:** [arXiv 2005.00559](https://arxiv.org/abs/2005.00559); [code](https://github.com/zhan-xu/RigNet)
- **Area:** graphics
- **Tags:** learned rigging, skinning weights, dataset
- **Read:** abstract and code README only
- **Verified:** 2026-10-10, arXiv listing via web search; README fetched from GitHub

## Summary

Predicts a skeleton and skinning weights for an articulated character with graph neural networks, trained on 2,703 rigged characters (ModelsResource-RigNetv1) covering humanoids, quadrupeds, birds, fish, robots and other fictional characters.

## Key ideas

- Skinning is predicted per vertex from the mesh and the skeleton, as RQ-002 needs.
- Weight quality is measured against artist weights (precision, recall, L1 of the weights).

## Results

Reports better skinning than BBW, NeuroSkinning and geodesic voxel binding on all its measures (from the paper's abstract-level text; table not read).

## Limitations and open questions

- Code is GPLv3. The dataset README states no licence, and the models were mined from the Models-Resource site, so their redistribution rights are unclear.
- Evaluation is weight agreement with artists, not deformation quality.

## Relevance to our work

The standard learned baseline, and its test set is a ready source of quadrupeds and creatures, if the licence question is resolved.
