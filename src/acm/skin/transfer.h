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
    double anchorSlack = 1.5;        // anchored: match a component's vertices within this factor of its nearest distance
    bool relativeMatch = false;      // relative: also match within relativeSlack of the neighbourhood's nearest distance
    double relativeSlack = 1.5;
    double relativeRadiusFraction = 0.05;  // neighbourhood radius as a fraction of the target's bounding-box diagonal
    bool consistencyCheck = false;   // consistent: drop matches where the closest-point map jumps
    double consistencyJump = 2.0;    // a jump is a closest-point separation over this many edge lengths
    double consistencyMargin = 2.0;  // matches within this many jump distances of a jump are dropped too
    double offsetBlend = 0.0;        // offsetblend: average matched weights within this many offset distances (0 = off)
    bool blendJointCap = false;      // jointcap: stop the blend region once it spans more than two dominant bones
    double lengthCap = 0.0;          // lengthcap: blend radius at most this fraction of the dominant bone's length (0 = off)
    Eigen::VectorXd boneLength;      // one per joint, from boneLengths(); needed by lengthcap
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

// M5: bounded, plus every connected component with no match gets its
// nearest vertices matched regardless of thresholds, so detached pieces
// (buttons, belts) have something to inpaint from instead of a singular system.
TransferResult transferAnchored(const Mesh& source, const Mesh& target, const TransferParams& params);

// M3: anchored, plus a region-relative distance threshold (see TransferParams),
// so loose garments match the limb under them instead of the one bone that
// happens to pass the global threshold.
// (method name "relative"; implemented as transferAnchored with relativeMatch set)

// M4: relative, plus matches are dropped where neighbouring vertices snap to
// far-apart body points (the wrong-leg case), with a margin of half the jump,
// so the gap is inpainted as a blend. (method name "consistent")

// M7: consistent, plus each matched vertex's copied weights are averaged over
// a radius proportional to its distance from the body, so a loose garment
// blends across a joint over a wider region than the skin. Current default
// method; the default scale is 4. (method name "offsetblend")

// M8: offsetblend, with the blend region grown outward by distance and cut
// off as soon as it would take in a third dominant bone, so a large scale
// cannot reach past the next joint. (method name "jointcap")

// M9: offsetblend, with the blend radius capped at a fraction of the length
// of the vertex's dominant bone, so the region cannot reach past the next
// joint however far the garment sits from the body. (method name "lengthcap")

// Dispatch by name ("closest", "inpaint", "bounded", "anchored", "relative",
// "consistent", "offsetblend", "jointcap", "lengthcap"); throws on an unknown name.
TransferResult runTransfer(const std::string& method, const Mesh& source, const Mesh& target,
                           const TransferParams& params);
std::vector<std::string> transferMethodNames();

// Keeps the `maxInfluences` largest weights per vertex and renormalises each
// row to sum to 1, as a game engine's import step would.
Eigen::MatrixXd pruneInfluences(const Eigen::MatrixXd& W, int maxInfluences);

}  // namespace acm::skin
