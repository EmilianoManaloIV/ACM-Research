#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <array>
#include <string>
#include <vector>

namespace acm::skin {

// A triangle mesh with optional per-vertex skin weights (one column per joint).
struct Mesh {
    Eigen::MatrixXd V;  // #V by 3 positions
    Eigen::MatrixXi F;  // #F by 3 triangle indices
    Eigen::MatrixXd W;  // #V by #joints skin weights, or empty if unskinned
    Eigen::MatrixXd N;  // #V by 3 per-vertex normals, see computeNormals()

    void computeNormals();
    bool isSkinned() const { return W.rows() == V.rows() && W.cols() > 0; }
};

// A helper joint whose pose is computed from the legs instead of read from a
// Pose: a hem bone that hangs from the hip line and is pushed by whichever
// leg swings toward it (see hem.h). Tier 0 pivots on the hip line, tier 1
// hangs from its tier-0 bone at knee height and follows the shanks.
struct DrivenJoint {
    int joint = -1;                 // index in the skeleton
    int tier = 0;
    int parentDriven = -1;          // index into Skeleton::driven for tier 1
    Eigen::Vector3d direction;      // unit horizontal azimuth of this bone
    Eigen::Vector3d down;           // unit rest direction of the legs
    Eigen::Vector3d pivot;          // world rest pivot (hip-line midpoint)
    double length = 0.0;            // tier-0 bone length (hip line to knee height)
    double follow = 1.0;            // share of a leg's swing the bone takes (cloth slides over the thigh)
    double falloff = 1.0;           // exponent on the azimuth cosine: higher, narrower push
    bool pivotOnHips = false;       // v2: pivot slides along the hip line toward the bone's side
    bool sideDrive = false;         // v3: a leg pushes the bones on its side of the hip line in its own swing direction
    Eigen::Vector3d across;         // unit hip line (first leg to last), for sideDrive
    bool lowerHangs = false;        // v2: tier 1 hangs down in world unless a shank pushes it
    std::vector<double> legRadius;  // v2 contact: thigh radius per leg; empty = no contact test
    double contactClearance = 0.0;  // extra distance the hem keeps from the thigh axis
    double radius = 0.0;            // v2 contact: horizontal distance of the panel this bone carries from the pivot
    std::vector<std::array<int, 3>> legs;  // (hip, knee, ankle) joint indices
};

// Joint hierarchy with the transforms needed for linear blend skinning.
struct Skeleton {
    std::vector<std::string> names;
    std::vector<int> parent;                   // -1 for a root joint
    std::vector<Eigen::Matrix4d> restLocal;    // joint transform relative to its parent
    std::vector<Eigen::Matrix4d> inverseBind;  // world-to-joint at bind time
    std::vector<DrivenJoint> driven;           // helper joints posed from other joints

    int size() const { return static_cast<int>(names.size()); }
    int find(const std::string& name) const;  // -1 if missing
};

// A pose is an extra rotation per joint, applied on top of the rest rotation,
// so the identity pose reproduces the bind pose.
struct Pose {
    std::string name;
    std::vector<Eigen::Quaterniond> rotation;  // one per joint
};

Pose identityPose(const Skeleton& skeleton, const std::string& name = "rest");

// World (rest-pose) transform of every joint, from restLocal alone.
std::vector<Eigen::Matrix4d> restGlobals(const Skeleton& skeleton);

// World position of every joint at rest.
std::vector<Eigen::Vector3d> restJointPositions(const Skeleton& skeleton);

// Length of each joint's bone at rest: the distance to its first child, or
// 0 for a leaf joint.
Eigen::VectorXd boneLengths(const Skeleton& skeleton);

// Poses the driven (hem) joints from the regular joints' matrices; called by
// skinningMatrices when the skeleton has any.
void driveHemJoints(const Skeleton& skeleton, std::vector<Eigen::Matrix4d>& matrices);

// Per-joint matrices that take a bind-space point to its posed position:
// G[j] * inverseBind[j], where G walks the hierarchy.
std::vector<Eigen::Matrix4d> skinningMatrices(const Skeleton& skeleton, const Pose& pose);

// Linear blend skinning on the CPU. W has one column per joint.
Eigen::MatrixXd linearBlendSkin(const Eigen::MatrixXd& V, const Eigen::MatrixXd& W,
                                const std::vector<Eigen::Matrix4d>& matrices);

// Dual quaternion skinning (Kavan et al. 2008): blends rigid transforms as
// dual quaternions, so bent joints keep their volume instead of collapsing.
// Each matrix must be rigid (rotation plus translation); scale is dropped.
Eigen::MatrixXd dualQuaternionSkin(const Eigen::MatrixXd& V, const Eigen::MatrixXd& W,
                                   const std::vector<Eigen::Matrix4d>& matrices);

enum class SkinningMode { LinearBlend, DualQuaternion };

// Parses "lbs" or "dqs"; returns false on anything else.
bool parseSkinningMode(const std::string& text, SkinningMode& mode);
const char* skinningModeName(SkinningMode mode);

Eigen::MatrixXd skin(const Eigen::MatrixXd& V, const Eigen::MatrixXd& W,
                     const std::vector<Eigen::Matrix4d>& matrices, SkinningMode mode);

// A rigged character: the body mesh and the skeleton it is bound to.
struct Rig {
    Mesh body;
    Skeleton skeleton;
};

// Weights as CSV, one row per vertex, one column per joint. Returns false on I/O error.
bool saveWeightsCSV(const std::string& path, const Eigen::MatrixXd& W);
bool loadWeightsCSV(const std::string& path, Eigen::MatrixXd& W);

}  // namespace acm::skin
