#include "acm/skin/metrics.h"

#include <igl/bounding_box_diagonal.h>
#include <igl/cotmatrix.h>
#include <igl/edges.h>
#include <igl/signed_distance.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace acm::skin {

namespace {

// Signed distance of each point to the mesh; negative means inside. The fast
// winding number sign is robust to small holes and self-intersections.
Eigen::VectorXd signedDistances(const Eigen::MatrixXd& P, const Eigen::MatrixXd& V, const Eigen::MatrixXi& F) {
    Eigen::VectorXd S;
    Eigen::VectorXi I;
    Eigen::MatrixXd C;
    Eigen::MatrixXd N;
    igl::signed_distance(P, V, F, igl::SIGNED_DISTANCE_TYPE_FAST_WINDING_NUMBER, S, I, C, N);
    return S;
}

double percentile(std::vector<double> values, double p) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double rank = p * static_cast<double>(values.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(rank));
    const size_t hi = std::min(lo + 1, values.size() - 1);
    return values[lo] + (values[hi] - values[lo]) * (rank - static_cast<double>(lo));
}

}  // namespace

PoseMetrics evaluatePose(const Mesh& bodyRest, const Eigen::MatrixXd& bodyPosed, const Mesh& garmentRest,
                         const Eigen::MatrixXd& garmentPosed) {
    PoseMetrics m;
    const double scale = igl::bounding_box_diagonal(bodyRest.V);

    const Eigen::VectorXd sRest = signedDistances(garmentRest.V, bodyRest.V, bodyRest.F);
    const Eigen::VectorXd sPosed = signedDistances(garmentPosed, bodyPosed, bodyRest.F);
    int inside = 0;
    double depthSum = 0.0;
    double driftSum = 0.0;
    for (int i = 0; i < sPosed.size(); ++i) {
        if (sPosed(i) < 0.0) {
            ++inside;
            depthSum += -sPosed(i);
            m.penetrationMaxDepth = std::max(m.penetrationMaxDepth, -sPosed(i) / scale);
        }
        driftSum += std::abs(std::abs(sPosed(i)) - std::abs(sRest(i)));
    }
    if (sPosed.size() > 0) {
        m.penetrationFraction = static_cast<double>(inside) / static_cast<double>(sPosed.size());
        m.offsetDrift = driftSum / static_cast<double>(sPosed.size()) / scale;
    }
    if (inside > 0) {
        m.penetrationMeanDepth = depthSum / inside / scale;
    }

    Eigen::MatrixXi E;
    igl::edges(garmentRest.F, E);
    std::vector<double> stretch;
    stretch.reserve(E.rows());
    for (int e = 0; e < E.rows(); ++e) {
        const double rest = (garmentRest.V.row(E(e, 0)) - garmentRest.V.row(E(e, 1))).norm();
        const double posed = (garmentPosed.row(E(e, 0)) - garmentPosed.row(E(e, 1))).norm();
        if (rest > 0.0) {
            stretch.push_back(std::abs(posed / rest - 1.0));
        }
    }
    m.stretchP95 = percentile(stretch, 0.95);
    m.stretchMax = stretch.empty() ? 0.0 : *std::max_element(stretch.begin(), stretch.end());
    return m;
}

WeightMetrics evaluateWeights(const Mesh& garment, const Eigen::MatrixXd& W, int influenceLimit, double epsilon) {
    WeightMetrics m;
    if (W.rows() == 0) {
        return m;
    }
    Eigen::SparseMatrix<double> L;
    igl::cotmatrix(garment.V, garment.F, L);
    m.dirichlet = (W.transpose() * (-L) * W).trace() / static_cast<double>(W.rows());

    m.minWeight = W.minCoeff();
    m.maxWeight = W.maxCoeff();
    int over = 0;
    for (int i = 0; i < W.rows(); ++i) {
        m.maxRowSumError = std::max(m.maxRowSumError, std::abs(W.row(i).sum() - 1.0));
        const int count = static_cast<int>((W.row(i).array().abs() > epsilon).count());
        m.maxInfluences = std::max(m.maxInfluences, count);
        if (count > influenceLimit) {
            ++over;
        }
    }
    m.overLimitFraction = static_cast<double>(over) / static_cast<double>(W.rows());
    return m;
}

}  // namespace acm::skin
