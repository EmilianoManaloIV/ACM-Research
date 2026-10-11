#pragma once

#include "acm/skin/rig.h"

#include <vector>

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

// Which body vertices a garment covers in rest, for the exposure score:
// a body vertex is covered when it lies on the inner side of the garment
// surface (pseudonormal sign of its closest garment point), skipping
// vertices whose closest garment face touches the garment boundary, where
// the side is undefined. `flip` records the garment's winding so the
// score does not depend on which way its normals point.
struct Coverage {
    std::vector<char> covered;    // per body vertex
    std::vector<char> boundary;   // per garment face: touches the garment boundary
    bool flip = false;
    int count = 0;                // covered vertices
};

Coverage coverage(const Mesh& bodyRest, const Mesh& garmentRest);

// Share of the covered body vertices that are on the outer side of the posed
// garment: a leg swung out through a stiff skirt, which the penetration
// score (garment vertices inside the body) barely sees. Vertices whose
// closest posed face touches the boundary are skipped, as in rest.
double exposureFraction(const Coverage& cover, const Eigen::MatrixXd& bodyPosed, const Mesh& garmentRest,
                        const Eigen::MatrixXd& garmentPosed);

// Edge stretch of a posed mesh against its rest shape: fills stretchP95,
// stretchMax, stretchMean and tornFraction of m.
void stretchMetrics(const Eigen::MatrixXi& F, const Eigen::MatrixXd& rest, const Eigen::MatrixXd& posed, PoseMetrics& m);

// Share of faces that intersect a face they share no vertex with. RQ-002
// reports the posed value minus the rest value, so a mesh that already
// overlaps itself at rest (eyes, teeth, layered shells) is not penalised.
// Coplanar overlap is not detected; posed skin rarely produces it.
// By default faces that only touch (across a seam, say) count as
// intersecting; ignoreContact counts only faces that cross, with every
// barycentric and segment parameter at least 1e-9 inside its bounds.
double selfIntersectionFraction(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F, bool ignoreContact = false);

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
