#include "acm/skin/metrics.h"

#include <igl/bounding_box_diagonal.h>
#include <igl/cotmatrix.h>
#include <igl/edges.h>
#include <igl/is_border_vertex.h>
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

// Pseudonormal signed distance of each point to an open mesh: the sign is
// the side of the closest point's (angle-weighted) normal, which is defined
// for open surfaces where the winding number is not. Also returns the
// closest face.
void pseudonormalDistances(const Eigen::MatrixXd& P, const Eigen::MatrixXd& V, const Eigen::MatrixXi& F,
                           Eigen::VectorXd& S, Eigen::VectorXi& I) {
    Eigen::MatrixXd C, N;
    igl::signed_distance(P, V, F, igl::SIGNED_DISTANCE_TYPE_PSEUDONORMAL, S, I, C, N);
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

Coverage coverage(const Mesh& bodyRest, const Mesh& garmentRest) {
    Coverage cover;
    const std::vector<bool> border = igl::is_border_vertex(garmentRest.F);
    cover.boundary.assign(garmentRest.F.rows(), 0);
    for (int f = 0; f < garmentRest.F.rows(); ++f) {
        for (int k = 0; k < 3; ++k) {
            if (border[garmentRest.F(f, k)]) {
                cover.boundary[f] = 1;
            }
        }
    }
    Eigen::VectorXd S;
    Eigen::VectorXi I;
    pseudonormalDistances(bodyRest.V, garmentRest.V, garmentRest.F, S, I);
    // Orientation: of the body vertices near the garment, more should be on
    // its inner side than its outer side; if not, the normals point inward.
    const double near = 0.05 * igl::bounding_box_diagonal(bodyRest.V);
    int innerSide = 0, outerSide = 0;
    for (int i = 0; i < S.size(); ++i) {
        if (cover.boundary[I(i)] || std::abs(S(i)) > near) {
            continue;
        }
        (S(i) < 0.0 ? innerSide : outerSide) += 1;
    }
    cover.flip = outerSide > innerSide;
    cover.covered.assign(S.size(), 0);
    for (int i = 0; i < S.size(); ++i) {
        const double s = cover.flip ? -S(i) : S(i);
        if (!cover.boundary[I(i)] && s < 0.0) {
            cover.covered[i] = 1;
            ++cover.count;
        }
    }
    return cover;
}

double exposureFraction(const Coverage& cover, const Eigen::MatrixXd& bodyPosed, const Mesh& garmentRest,
                        const Eigen::MatrixXd& garmentPosed) {
    if (cover.count == 0) {
        return 0.0;
    }
    Eigen::VectorXd S;
    Eigen::VectorXi I;
    pseudonormalDistances(bodyPosed, garmentPosed, garmentRest.F, S, I);
    int exposed = 0;
    for (int i = 0; i < S.size(); ++i) {
        const double s = cover.flip ? -S(i) : S(i);
        if (cover.covered[i] && !cover.boundary[I(i)] && s > 0.0) {
            ++exposed;
        }
    }
    return static_cast<double>(exposed) / cover.count;
}

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
    if (!stretch.empty()) {
        double sum = 0.0;
        int torn = 0;
        for (const double s : stretch) {
            sum += s;
            torn += s > 1.0 ? 1 : 0;
        }
        m.stretchMean = sum / static_cast<double>(stretch.size());
        m.tornFraction = static_cast<double>(torn) / static_cast<double>(stretch.size());
    }
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
