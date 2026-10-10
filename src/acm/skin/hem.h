#pragma once

#include "acm/skin/rig.h"

#include <string>
#include <vector>

namespace acm::skin {

// A procedural skirt rig: `count` hem bones around the hip line, each
// pushed by whichever leg swings toward it, in `tiers` tiers (hip line and
// knee height). The weight a garment's inpainted vertices carried on the
// legs is moved onto these bones by azimuth and height, so a panel that
// bridges the legs follows a smooth field between them instead of tearing.
struct HemRig {
    int count = 8;
    int tiers = 2;
    int parentJoint = -1;                            // the hem bones hang from this joint
    std::vector<std::array<int, 3>> legs;            // (hip, knee, ankle) per leg
    std::vector<int> moveFrom;                       // joints whose weight is moved onto the hem bones
    double follow = 1.0;                             // see DrivenJoint
    double falloff = 1.0;
    bool applyToMatched = false;                     // "apply": "all" moves matched vertices' leg weight too
    int firstJoint = -1;                             // index of the first hem joint once added
    Eigen::Vector3d pivot, down, e1, e2;             // set by addHemJoints
    double length = 0.0;
};

// Reads {"count", "tiers", "parent", "legs": [[hip, knee, ankle], ...], "move": [...]}.
HemRig loadHemRig(const std::string& path, const Skeleton& skeleton);

// Appends the hem joints to the skeleton (as driven joints) and fills the
// rig's geometry from the rest pose.
void addHemJoints(Skeleton& skeleton, HemRig& rig);

// Returns W with columns for the hem joints; for every vertex the transfer
// did not match, its weight on `moveFrom` joints and the leg chains is moved
// onto the two nearest hem bones by azimuth, split between tiers by height.
Eigen::MatrixXd assignHemWeights(const Mesh& garment, const Eigen::MatrixXd& W,
                                 const Eigen::Array<bool, Eigen::Dynamic, 1>& matched, const Skeleton& skeleton,
                                 const HemRig& rig);

}  // namespace acm::skin
