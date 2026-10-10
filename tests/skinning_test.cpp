// Checks the two skinning models on a two-bone chain: a 50/50 vertex bent 90
// degrees collapses toward the joint under linear blend skinning and rotates
// by 45 degrees at full radius under dual quaternion skinning.

#include "acm/skin/rig.h"

#include <cmath>
#include <cstdio>

int main() {
    using namespace acm::skin;
    Skeleton s;
    s.names = {"a", "b"};
    s.parent = {-1, 0};
    Eigen::Matrix4d restA = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d restB = Eigen::Matrix4d::Identity();
    restB(0, 3) = 1.0;  // joint b sits 1 unit along x
    s.restLocal = {restA, restB};
    s.inverseBind = {restA.inverse(), restB.inverse()};

    Pose pose = identityPose(s, "bend");
    pose.rotation[1] = Eigen::Quaterniond(Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));
    const auto M = skinningMatrices(s, pose);

    Eigen::MatrixXd V(1, 3);
    V << 1.0, 1.0, 0.0;  // one unit above joint b
    Eigen::MatrixXd W(1, 2);
    W << 0.5, 0.5;

    const Eigen::Vector3d lbs = linearBlendSkin(V, W, M).row(0);
    const Eigen::Vector3d dqs = dualQuaternionSkin(V, W, M).row(0);
    const Eigen::Vector3d lbsExpected(0.5, 0.5, 0.0);
    const Eigen::Vector3d dqsExpected(1.0 - std::sqrt(0.5), std::sqrt(0.5), 0.0);
    int failures = 0;
    if ((lbs - lbsExpected).norm() > 1e-9) {
        std::printf("LBS: got (%g, %g, %g), expected (0.5, 0.5, 0)\n", lbs.x(), lbs.y(), lbs.z());
        ++failures;
    }
    if ((dqs - dqsExpected).norm() > 1e-9) {
        std::printf("DQS: got (%g, %g, %g), expected (%g, %g, 0)\n", dqs.x(), dqs.y(), dqs.z(), dqsExpected.x(),
                    dqsExpected.y());
        ++failures;
    }
    // A single-influence vertex is the same under both models.
    W << 0.0, 1.0;
    if ((linearBlendSkin(V, W, M) - dualQuaternionSkin(V, W, M)).norm() > 1e-9) {
        std::printf("single influence differs between LBS and DQS\n");
        ++failures;
    }
    std::printf(failures ? "FAILED\n" : "OK\n");
    return failures ? 1 : 0;
}
