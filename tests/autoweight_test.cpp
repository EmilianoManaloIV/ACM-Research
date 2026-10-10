// Checks the RQ-002 building blocks on small cases with known answers: bone
// segments, the inverse-distance floor, validity, weight agreement, and the
// self-intersection metric.

#include "acm/skin/autoweight.h"
#include "acm/skin/metrics.h"

#include <cmath>
#include <cstdio>

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

}  // namespace

int main() {
    using namespace acm::skin;

    // A two-joint chain along x: a at 0, b at 1 (a leaf).
    Skeleton s;
    s.names = {"a", "b"};
    s.parent = {-1, 0};
    Eigen::Matrix4d ra = Eigen::Matrix4d::Identity(), rb = Eigen::Matrix4d::Identity();
    rb(0, 3) = 1.0;
    s.restLocal = {ra, rb};
    s.inverseBind = {ra.inverse(), rb.inverse()};

    const std::vector<BoneSegment> bones = boneSegments(s);
    check(bones.size() == 2, "one segment per joint");
    check((bones[0].b - Eigen::Vector3d(1, 0, 0)).norm() < 1e-12, "root bone ends at its child");
    check((bones[1].b - Eigen::Vector3d(1.5, 0, 0)).norm() < 1e-12, "leaf bone extends the parent by half");

    // Floor weights: a point on bone a is all a; a point equidistant from both is 50/50.
    Eigen::MatrixXd V(2, 3);
    V << 0.25, 0.0, 0.0,  // on bone a, 0.75 from bone b
        1.0, 1.0, 0.0;    // distance 1 from both segments
    const Eigen::MatrixXd W = inverseDistanceWeights(V, bones, 2);
    check(W(0, 0) > 0.999, "a point on a bone belongs to it");
    check(std::abs(W(1, 0) - 0.5) < 1e-9, "equidistant point is split evenly");

    int unweighted = 0;
    check(validWeights(W, unweighted) && unweighted == 0, "floor weights are valid");
    Eigen::MatrixXd Z = W;
    Z.row(1).setZero();
    check(!validWeights(Z, unweighted) && unweighted == 1, "a zero row is invalid and counted");

    // Agreement: identical weights give L1 0 and full precision and recall.
    const WeightAgreement same = compareWeights(W, W);
    check(same.l1 < 1e-12 && same.precision == 1.0 && same.recall == 1.0, "agreement with itself");
    Eigen::MatrixXd R(2, 2);
    R << 1, 0, 0, 1;
    Eigen::MatrixXd P(2, 2);
    P << 1, 0, 1, 0;
    const WeightAgreement off = compareWeights(P, R);
    check(std::abs(off.l1 - 1.0) < 1e-12, "L1 averages the per-vertex distance");
    check(std::abs(off.precision - 0.5) < 1e-12 && std::abs(off.recall - 0.5) < 1e-12, "precision and recall");

    // Self-intersection: two triangles crossing each other, and two apart.
    Eigen::MatrixXd T(6, 3);
    T << 0, 0, 0, 1, 0, 0, 0, 1, 0,  // in the z = 0 plane
        0.2, 0.2, -0.5, 0.2, 0.2, 0.5, 0.8, 0.2, 0.0;  // pierces it
    Eigen::MatrixXi F(2, 3);
    F << 0, 1, 2, 3, 4, 5;
    check(std::abs(selfIntersectionFraction(T, F) - 1.0) < 1e-12, "crossing triangles both count");
    T.bottomRows(3).col(2).array() += 5.0;
    check(selfIntersectionFraction(T, F) == 0.0, "separated triangles do not count");
    Eigen::MatrixXi G(2, 3);
    G << 0, 1, 2, 0, 2, 3;  // share an edge: adjacent faces are skipped
    Eigen::MatrixXd Q(4, 3);
    Q << 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0;
    check(selfIntersectionFraction(Q, G) == 0.0, "adjacent faces are not intersections");

    if (failures == 0) {
        std::printf("autoweight_test: all checks passed\n");
    }
    return failures == 0 ? 0 : 1;
}
