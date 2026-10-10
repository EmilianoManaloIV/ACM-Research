#pragma once

// Automatic skin weights for a body from its skeleton alone, with no weights
// to transfer from (RQ-002), and the measures that compare weights with an
// artist's.

#include "acm/skin/rig.h"

#include <string>
#include <vector>

namespace acm::skin {

// One bone per joint, shared by every method so they all see the same
// skeleton: from the joint to the mean of its children, or to its first
// child when that mean falls on the joint. A leaf joint continues its
// parent's direction for half the parent's length; a joint with neither
// children nor parent is a point (a == b).
struct BoneSegment {
    int joint = -1;
    Eigen::Vector3d a;
    Eigen::Vector3d b;
};

std::vector<BoneSegment> boneSegments(const Skeleton& skeleton);

// Writes the segments as JSON for external tools (the Blender bone heat
// script, the pose generator): {"bones": [{"name", "parent", "head", "tail",
// "local_tail"}, ...]} in joint order. head and tail are in world space at
// rest; local_tail is the tail in the joint's own frame, the frame pose
// rotations are applied in.
bool saveBoneSegments(const std::string& path, const Skeleton& skeleton, const std::vector<BoneSegment>& bones);

// B0, the floor: each joint's weight is 1 / d^power, d the distance from the
// vertex to its bone segment, then rows are normalised.
Eigen::MatrixXd inverseDistanceWeights(const Eigen::MatrixXd& V, const std::vector<BoneSegment>& bones,
                                       int joints, double power = 2.0);

struct AutoWeightResult {
    Eigen::MatrixXd W;  // #V by #joints; empty when the method failed outright
    std::string note;   // why it failed, or what it had to work around
};

// B2, bounded biharmonic weights (Jacobson et al. 2011) with libigl: the
// surface is tetrahedralised by TetGen with samples along every bone added
// as interior points, each bone gets weight 1 on its samples and 0 on the
// others, and the weights are solved with the active-set QP (no partition of
// unity constraint, rows normalised afterwards, as in libigl's tutorial).
// maxIterations caps the active-set loop; the tutorial's 8 can stop before
// convergence, leaving small negative weights.
AutoWeightResult boundedBiharmonicWeights(const Mesh& body, const std::vector<BoneSegment>& bones, int joints,
                                          int samplesPerBone = 10, int maxIterations = 8);

// Rows with a positive sum are normalised to sum to one, as Blender and
// libigl do before deforming; negative values above -epsilon are set to zero.
void normaliseRows(Eigen::MatrixXd& W, double epsilon = 1e-9);

// The protocol's validity test, after normaliseRows: every value finite and
// non-negative, every row summing to one within tolerance. unweighted counts
// rows that sum to zero.
bool validWeights(const Eigen::MatrixXd& W, int& unweighted, double tolerance = 1e-6);

// Agreement with a reference (artist) weight matrix: mean per-vertex L1
// distance, and precision and recall of the influences above threshold.
struct WeightAgreement {
    double l1 = 0.0;
    double precision = 0.0;
    double recall = 0.0;
};

WeightAgreement compareWeights(const Eigen::MatrixXd& W, const Eigen::MatrixXd& reference, double threshold = 1e-4);

}  // namespace acm::skin
