#pragma once

#include "acm/skin/rig.h"

#include <vector>

namespace acm::skin {

// M6: pose-aware refinement. For each training pose the garment, skinned
// with the current weights, is relaxed against the posed body (vertices
// inside or too close to the body are pushed out to a share of their rest
// clearance, edges are pulled back toward their rest lengths), and the
// weights are then refit per vertex so that linear blend skinning
// reproduces the relaxed positions over all training poses, staying on the
// simplex and within the vertex's current support.
struct RefineParams {
    int relaxIterations = 20;        // collision + stretch rounds per pose
    int stretchPasses = 4;           // edge-length projections per round
    double stretchStiffness = 0.5;   // share of each edge's error removed per pass
    double clearanceShare = 0.5;     // vertices closer to the body than this share of their rest clearance are pushed out
    int support = 8;                 // bones a vertex may use: the largest in the current weights
    double prior = 0.1;              // pull toward the current weights, relative to the fit term per pose
    int fitIterations = 300;         // projected-gradient steps per vertex
    SkinningMode skinning = SkinningMode::LinearBlend;  // model the targets are relaxed from and fit under (LBS only for the fit)
};

struct RefineResult {
    Eigen::MatrixXd W;
    double targetChange = 0.0;  // mean distance the relaxation moved a vertex, over poses, as a fraction of the body's diagonal
    double fitResidual = 0.0;   // mean distance between the fit and the relaxed targets, same units
};

RefineResult refinePoseAware(const Rig& rig, const Mesh& garment, const Eigen::MatrixXd& W0,
                             const std::vector<Pose>& trainPoses, const RefineParams& params);

}  // namespace acm::skin
