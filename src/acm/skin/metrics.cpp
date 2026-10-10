#include "acm/skin/metrics.h"

#include <igl/bounding_box_diagonal.h>
#include <igl/cotmatrix.h>
#include <igl/edges.h>
#include <igl/is_border_vertex.h>
#include <igl/signed_distance.h>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
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

// Whether segment pq crosses triangle abc (Moller-Trumbore, with the
// parameter limited to the segment). Coplanar contact is not counted.
bool segmentHitsTriangle(const Eigen::Vector3d& p, const Eigen::Vector3d& q, const Eigen::Vector3d& a,
                         const Eigen::Vector3d& b, const Eigen::Vector3d& c) {
    const Eigen::Vector3d d = q - p, e1 = b - a, e2 = c - a;
    const Eigen::Vector3d h = d.cross(e2);
    const double det = e1.dot(h);
    const double eps = 1e-12 * std::max(1.0, e1.norm() * e2.norm() * d.norm());
    if (std::abs(det) <= eps) {
        return false;
    }
    const double inv = 1.0 / det;
    const Eigen::Vector3d sv = p - a;
    const double u = inv * sv.dot(h);
    if (u < 0.0 || u > 1.0) {
        return false;
    }
    const Eigen::Vector3d qv = sv.cross(e1);
    const double v = inv * d.dot(qv);
    if (v < 0.0 || u + v > 1.0) {
        return false;
    }
    const double t = inv * e2.dot(qv);
    return t >= 0.0 && t <= 1.0;
}

// Two non-coplanar triangles intersect exactly when an edge of one crosses the other.
bool trianglesIntersect(const Eigen::Vector3d* A, const Eigen::Vector3d* B) {
    for (int k = 0; k < 3; ++k) {
        if (segmentHitsTriangle(A[k], A[(k + 1) % 3], B[0], B[1], B[2]) ||
            segmentHitsTriangle(B[k], B[(k + 1) % 3], A[0], A[1], A[2])) {
            return true;
        }
    }
    return false;
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

void stretchMetrics(const Eigen::MatrixXi& F, const Eigen::MatrixXd& rest, const Eigen::MatrixXd& posed, PoseMetrics& m) {
    Eigen::MatrixXi E;
    igl::edges(F, E);
    std::vector<double> stretch;
    stretch.reserve(E.rows());
    for (int e = 0; e < E.rows(); ++e) {
        const double r = (rest.row(E(e, 0)) - rest.row(E(e, 1))).norm();
        const double p = (posed.row(E(e, 0)) - posed.row(E(e, 1))).norm();
        if (r > 0.0) {
            stretch.push_back(std::abs(p / r - 1.0));
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
}

double selfIntersectionFraction(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F) {
    const int nf = static_cast<int>(F.rows());
    if (nf < 2) {
        return 0.0;
    }
    // Broad phase: hash each face's bounding box into a uniform grid whose
    // cell is twice the mean edge length; test pairs that share a cell.
    double edgeSum = 0.0;
    for (int f = 0; f < nf; ++f) {
        for (int k = 0; k < 3; ++k) {
            edgeSum += (V.row(F(f, k)) - V.row(F(f, (k + 1) % 3))).norm();
        }
    }
    const double cell = std::max(2.0 * edgeSum / (3.0 * nf), 1e-12);
    const Eigen::RowVector3d origin = V.colwise().minCoeff();
    std::unordered_map<long long, std::vector<int>> grid;
    const auto key = [](long long x, long long y, long long z) { return (x * 73856093LL) ^ (y * 19349663LL) ^ (z * 83492791LL); };
    std::vector<Eigen::Array3i> lo(nf), hi(nf);
    for (int f = 0; f < nf; ++f) {
        Eigen::RowVector3d mn = V.row(F(f, 0)), mx = mn;
        for (int k = 1; k < 3; ++k) {
            mn = mn.cwiseMin(V.row(F(f, k)));
            mx = mx.cwiseMax(V.row(F(f, k)));
        }
        lo[f] = ((mn - origin) / cell).array().floor().cast<int>().transpose();
        hi[f] = ((mx - origin) / cell).array().floor().cast<int>().transpose();
        for (int x = lo[f].x(); x <= hi[f].x(); ++x)
            for (int y = lo[f].y(); y <= hi[f].y(); ++y)
                for (int z = lo[f].z(); z <= hi[f].z(); ++z) grid[key(x, y, z)].push_back(f);
    }
    std::vector<char> hit(nf, 0);
    std::unordered_set<long long> tested;
    for (const auto& entry : grid) {
        const std::vector<int>& faces = entry.second;
        for (size_t i = 0; i < faces.size(); ++i) {
            for (size_t j = i + 1; j < faces.size(); ++j) {
                const int a = std::min(faces[i], faces[j]), b = std::max(faces[i], faces[j]);
                if (hit[a] && hit[b]) {
                    continue;
                }
                if ((lo[a] > hi[b]).any() || (lo[b] > hi[a]).any()) {
                    continue;
                }
                bool adjacent = false;
                for (int u = 0; u < 3 && !adjacent; ++u)
                    for (int v = 0; v < 3 && !adjacent; ++v) adjacent = F(a, u) == F(b, v);
                if (adjacent || !tested.insert(static_cast<long long>(a) * nf + b).second) {
                    continue;
                }
                const Eigen::Vector3d A[3] = {V.row(F(a, 0)), V.row(F(a, 1)), V.row(F(a, 2))};
                const Eigen::Vector3d B[3] = {V.row(F(b, 0)), V.row(F(b, 1)), V.row(F(b, 2))};
                if (trianglesIntersect(A, B)) {
                    hit[a] = hit[b] = 1;
                }
            }
        }
    }
    return static_cast<double>(std::count(hit.begin(), hit.end(), 1)) / nf;
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

    stretchMetrics(garmentRest.F, garmentRest.V, garmentPosed, m);
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
