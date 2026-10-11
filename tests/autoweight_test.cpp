// Checks the RQ-002 building blocks on small cases with known answers: bone
// segments, the inverse-distance floor, validity, weight agreement, the
// self-intersection metric (with and without contact), and BBW on a small
// closed mesh with a seam.

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

    // Touching faces: two triangles meeting along an edge without sharing a
    // vertex index (a seam). Counted by default, not when contact is ignored.
    Eigen::MatrixXd S(6, 3);
    S << 0, 0, 0, 1, 0, 0, 0, 1, 0,  // in z = 0
        0, 0, 0, 1, 0, 0, 0, 0, 1;   // same edge, folded up into y = 0
    check(std::abs(selfIntersectionFraction(S, F) - 1.0) < 1e-12, "touching faces count by default");
    check(selfIntersectionFraction(S, F, true) == 0.0, "touching faces do not count when contact is ignored");
    T.bottomRows(3).col(2).array() -= 5.0;  // back to the crossing pair
    check(std::abs(selfIntersectionFraction(T, F, true) - 1.0) < 1e-12, "crossing faces count when contact is ignored");

    // BBW on a closed box around the two-bone chain, with one corner
    // duplicated (a seam): TetGen merges the duplicate, and both copies must
    // get the same weights from the merged tet vertex.
    Mesh box;
    box.V.resize(9, 3);
    box.V << -0.2, -0.3, -0.3, 2.0, -0.3, -0.3, 2.0, 0.3, -0.3, -0.2, 0.3, -0.3,  //
        -0.2, -0.3, 0.3, 2.0, -0.3, 0.3, 2.0, 0.3, 0.3, -0.2, 0.3, 0.3,          //
        -0.2, -0.3, -0.3;                                                         // copy of 0
    box.F.resize(12, 3);
    box.F << 0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 8, 1, 5, 0, 5, 4,  // bottom, top, front (uses the copy)
        3, 6, 2, 3, 7, 6, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5;     // back, left, right
    const AutoWeightResult bbw = boundedBiharmonicWeights(box, bones, 2, 10, 100);
    check(bbw.note.empty() && bbw.W.rows() == 9 && bbw.W.cols() == 2, "BBW solves on a closed box");
    if (bbw.W.rows() == 9 && bbw.W.cols() == 2) {
        Eigen::MatrixXd Wb = bbw.W;
        normaliseRows(Wb, 1e-6);
        check(validWeights(Wb, unweighted) && unweighted == 0, "BBW weights are valid after the 1e-6 clip");
        check(Wb(0, 0) > 0.9 && Wb(3, 0) > 0.9, "the far end of bone a belongs to joint a");
        check(Wb(2, 1) > 0.9 && Wb(6, 1) > 0.9, "the far end of bone b belongs to joint b");
        check((bbw.W.row(8) - bbw.W.row(0)).cwiseAbs().maxCoeff() < 1e-12, "a duplicated vertex gets its twin's weights");
    }

    if (failures == 0) {
        std::printf("autoweight_test: all checks passed\n");
    }
    return failures == 0 ? 0 : 1;
}
