#pragma once

#include "acm/skin/rig.h"

namespace acm::skin {

// How a garment behaves in one pose, given the posed body and garment. Depths
// and drift are fractions of the body's rest bounding-box diagonal so that
// characters of different sizes compare.
struct PoseMetrics {
    double penetrationFraction = 0.0;   // share of garment vertices inside the body
    double penetrationMeanDepth = 0.0;  // mean depth of those vertices
    double penetrationMaxDepth = 0.0;
    double stretchP95 = 0.0;            // 95th percentile of |posed edge / rest edge - 1|
    double stretchMax = 0.0;
    double stretchMean = 0.0;           // mean of the same, so a tear spread over many edges and one over few compare
    double tornFraction = 0.0;          // share of edges stretched or shrunk by more than 100%
    double offsetDrift = 0.0;           // mean change in garment-to-body distance from rest
};

PoseMetrics evaluatePose(const Mesh& bodyRest, const Eigen::MatrixXd& bodyPosed, const Mesh& garmentRest,
                         const Eigen::MatrixXd& garmentPosed);

// Properties of the weights themselves, independent of any pose.
struct WeightMetrics {
    double dirichlet = 0.0;      // sum over joints of w^T (-L) w, per vertex
    double minWeight = 0.0;
    double maxWeight = 0.0;
    double maxRowSumError = 0.0;  // max |sum of a row - 1|
    int maxInfluences = 0;        // most non-zero weights on any vertex
    double overLimitFraction = 0.0;  // share of vertices with more than `influenceLimit` non-zeros
};

WeightMetrics evaluateWeights(const Mesh& garment, const Eigen::MatrixXd& W, int influenceLimit = 4,
                              double epsilon = 1e-6);

}  // namespace acm::skin
