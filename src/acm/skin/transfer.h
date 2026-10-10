#pragma once

#include "acm/skin/rig.h"

#include <string>
#include <vector>

namespace acm::skin {

// Knobs shared by the transfer methods. Defaults follow Epic's released code.
struct TransferParams {
    double distanceFraction = 0.05;  // match threshold as a fraction of the target's bounding-box diagonal
    double angleDegrees = 30.0;      // max angle between source and target normals for a match
    bool smooth = true;              // run the post-inpainting neighbour smoothing
    int smoothSteps = 10;
    double smoothAlpha = 0.2;
};

struct TransferResult {
    Eigen::MatrixXd W;                        // #target by #joints
    Eigen::Array<bool, Eigen::Dynamic, 1> matched;  // which target vertices were copied directly
    double seconds = 0.0;
};

// M0: copy weights from the closest point on the source, no checks.
TransferResult transferClosestPoint(const Mesh& source, const Mesh& target, const TransferParams& params);

// M1: Abdrashitov et al., "Robust Skin Weights Transfer via Weight Inpainting"
// (SIGGRAPH Asia 2023): closest-point matching with distance and normal
// thresholds, biharmonic inpainting of the rest, then local smoothing.
TransferResult transferInpaint(const Mesh& source, const Mesh& target, const TransferParams& params);

// M2: the inpainting energy solved per bone with 0 <= w <= 1 (active set, as
// in bounded biharmonic weights), rows then normalised to sum to 1.
TransferResult transferBounded(const Mesh& source, const Mesh& target, const TransferParams& params);

// Dispatch by name ("closest", "inpaint", "bounded"); throws on an unknown name.
TransferResult runTransfer(const std::string& method, const Mesh& source, const Mesh& target,
                           const TransferParams& params);
std::vector<std::string> transferMethodNames();

// Keeps the `maxInfluences` largest weights per vertex and renormalises each
// row to sum to 1, as a game engine's import step would.
Eigen::MatrixXd pruneInfluences(const Eigen::MatrixXd& W, int maxInfluences);

}  // namespace acm::skin
