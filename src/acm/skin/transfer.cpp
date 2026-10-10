// Weight transfer methods. transferInpaint() follows Epic's released Python
// (github.com/rin-23/RobustSkinWeightsTransferCode, MIT) step for step so the
// C++ can be checked against it; see tools/verify_inpaint.py.

#include "acm/skin/transfer.h"

#include <igl/active_set.h>
#include <igl/adjacency_list.h>
#include <igl/adjacency_matrix.h>
#include <igl/connected_components.h>
#include <igl/barycentric_coordinates.h>
#include <igl/bounding_box_diagonal.h>
#include <igl/cotmatrix.h>
#include <igl/massmatrix.h>
#include <igl/min_quad_with_fixed.h>
#include <igl/point_mesh_squared_distance.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace acm::skin {

namespace {

struct ClosestPoints {
    Eigen::VectorXd sqrD;   // squared distance per target vertex
    Eigen::MatrixXd C;      // closest point on the source per target vertex
    Eigen::MatrixXd W;      // source weights interpolated at the closest point
    Eigen::MatrixXd N;      // source normals interpolated at the closest point
};

ClosestPoints closestPoints(const Mesh& source, const Mesh& target) {
    Eigen::VectorXi I;
    ClosestPoints out;
    igl::point_mesh_squared_distance(target.V, source.V, source.F, out.sqrD, I, out.C);
    const Eigen::MatrixXd& C = out.C;

    Eigen::MatrixXd A(C.rows(), 3), B(C.rows(), 3), D(C.rows(), 3);
    for (int i = 0; i < C.rows(); ++i) {
        A.row(i) = source.V.row(source.F(I(i), 0));
        B.row(i) = source.V.row(source.F(I(i), 1));
        D.row(i) = source.V.row(source.F(I(i), 2));
    }
    Eigen::MatrixXd bary;
    igl::barycentric_coordinates(C, A, B, D, bary);

    out.W.resize(C.rows(), source.W.cols());
    out.N.resize(C.rows(), 3);
    for (int i = 0; i < C.rows(); ++i) {
        const Eigen::RowVector3i f = source.F.row(I(i));
        out.W.row(i) = bary(i, 0) * source.W.row(f(0)) + bary(i, 1) * source.W.row(f(1)) + bary(i, 2) * source.W.row(f(2));
        out.N.row(i) = bary(i, 0) * source.N.row(f(0)) + bary(i, 1) * source.N.row(f(1)) + bary(i, 2) * source.N.row(f(2));
    }
    return out;
}

double elapsed(const std::chrono::steady_clock::time_point& start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

void requireSkinnedSource(const Mesh& source) {
    if (!source.isSkinned()) {
        throw std::runtime_error("transfer: source mesh has no skin weights");
    }
}

}  // namespace

TransferResult transferClosestPoint(const Mesh& source, const Mesh& target, const TransferParams&) {
    requireSkinnedSource(source);
    const auto start = std::chrono::steady_clock::now();
    TransferResult result;
    result.W = closestPoints(source, target).W;
    result.matched = Eigen::Array<bool, Eigen::Dynamic, 1>::Constant(target.V.rows(), true);
    result.seconds = elapsed(start);
    return result;
}

namespace {

// Shared first half of the inpainting methods: Epic's matching and the
// biharmonic system Q = -L + L M^-1 L with L = 2 * cotmatrix.
struct InpaintSetup {
    ClosestPoints cp;
    Eigen::Array<bool, Eigen::Dynamic, 1> matched;
    Eigen::SparseMatrix<double> Q;
    Eigen::VectorXi known;
    Eigen::MatrixXd knownValues;
    double distance = 0.0;
};

InpaintSetup prepareInpaint(const Mesh& source, const Mesh& target, const TransferParams& params) {
    InpaintSetup s;
    const int n = static_cast<int>(target.V.rows());
    s.distance = params.distanceFraction * igl::bounding_box_diagonal(target.V);
    const double sqrDistance = s.distance * s.distance;

    // Section 3.1: closest point matching with distance and normal thresholds.
    s.cp = closestPoints(source, target);
    s.matched = Eigen::Array<bool, Eigen::Dynamic, 1>::Constant(n, false);

    // M3: a region-relative threshold. Each vertex also matches if it is no
    // further from the body than `relativeSlack` times the nearest distance
    // found in its neighbourhood (vertices within `radius` of it along the
    // mesh), so a sleeve that floats 5 cm off the arm everywhere still
    // matches, while a fold that dips towards the body stands out.
    Eigen::VectorXd localNearest = s.cp.sqrD;
    if (params.relativeMatch) {
        const double radius = params.relativeRadiusFraction * igl::bounding_box_diagonal(target.V);
        std::vector<std::vector<int>> adjacency;
        igl::adjacency_list(target.F, adjacency);
        std::vector<int> seen(n, -1);
        for (int seed = 0; seed < n; ++seed) {
            std::vector<int> stack{seed};
            seen[seed] = seed;
            while (!stack.empty()) {
                const int v = stack.back();
                stack.pop_back();
                localNearest(seed) = std::min(localNearest(seed), s.cp.sqrD(v));
                for (int nb : adjacency[v]) {
                    if (seen[nb] != seed && (target.V.row(seed) - target.V.row(nb)).norm() < radius) {
                        seen[nb] = seed;
                        stack.push_back(nb);
                    }
                }
            }
        }
    }
    for (int i = 0; i < n; ++i) {
        const Eigen::Vector3d n1 = s.cp.N.row(i).normalized();
        const Eigen::Vector3d n2 = target.N.row(i).normalized();
        const double degrees = std::acos(std::clamp(n1.dot(n2), -1.0, 1.0)) * 180.0 / M_PI;
        bool close = s.cp.sqrD(i) <= sqrDistance;
        if (params.relativeMatch) {
            close = close || s.cp.sqrD(i) <= localNearest(i) * params.relativeSlack * params.relativeSlack;
        }
        s.matched(i) = close && degrees <= params.angleDegrees;
    }

    // M4: a match is only trusted where the closest-point map is continuous.
    // Between the legs of a skirt, neighbouring vertices snap to opposite
    // legs: their closest points are far apart although they are one edge
    // apart. Those matches are dropped, and so is everything within
    // consistencyMargin times the jump distance of them, so the two sides
    // blend through inpainting over a strip that scales with the jump.
    if (params.consistencyCheck) {
        std::vector<std::vector<int>> adjacency;
        igl::adjacency_list(target.F, adjacency);
        std::vector<double> reject(n, 0.0);  // dilation radius per seed, 0 = keep
        for (int i = 0; i < n; ++i) {
            if (!s.matched(i)) {
                continue;
            }
            for (int nb : adjacency[i]) {
                if (!s.matched(nb)) {
                    continue;
                }
                const double edge = (target.V.row(i) - target.V.row(nb)).norm();
                const double jump = (s.cp.C.row(i) - s.cp.C.row(nb)).norm();
                if (jump > params.consistencyJump * edge) {
                    reject[i] = std::max(reject[i], params.consistencyMargin * jump);
                }
            }
        }
        for (int seed = 0; seed < n; ++seed) {
            if (reject[seed] <= 0.0) {
                continue;
            }
            std::vector<int> stack{seed};
            s.matched(seed) = false;
            while (!stack.empty()) {
                const int v = stack.back();
                stack.pop_back();
                for (int nb : adjacency[v]) {
                    if (s.matched(nb) && (target.V.row(seed) - target.V.row(nb)).norm() < reject[seed]) {
                        s.matched(nb) = false;
                        stack.push_back(nb);
                    }
                }
            }
        }
    }

    Eigen::SparseMatrix<double> L;
    Eigen::SparseMatrix<double> M;
    igl::cotmatrix(target.V, target.F, L);
    L = 2.0 * L;
    igl::massmatrix(target.V, target.F, igl::MASSMATRIX_TYPE_VORONOI, M);
    Eigen::SparseMatrix<double> Minv(M.rows(), M.cols());
    {
        std::vector<Eigen::Triplet<double>> t;
        t.reserve(M.rows());
        for (int i = 0; i < M.rows(); ++i) {
            t.emplace_back(i, i, 1.0 / M.coeff(i, i));
        }
        Minv.setFromTriplets(t.begin(), t.end());
    }
    s.Q = -L + L * Minv * L;

    const int matchedCount = static_cast<int>(s.matched.count());
    if (matchedCount == 0) {
        throw std::runtime_error("transfer: no target vertex matched the source, nothing to inpaint from");
    }
    s.known.resize(matchedCount);
    s.knownValues.resize(matchedCount, s.cp.W.cols());
    for (int i = 0, k = 0; i < n; ++i) {
        if (s.matched(i)) {
            s.known(k) = i;
            s.knownValues.row(k) = s.cp.W.row(i);
            ++k;
        }
    }
    return s;
}

// Epic's post-inpainting smoothing of the unmatched region and its neighbours.
void smoothInpainted(const Mesh& target, const Eigen::Array<bool, Eigen::Dynamic, 1>& matched, double distance,
                     const TransferParams& params, Eigen::MatrixXd& W) {
    const int n = static_cast<int>(target.V.rows());
    std::vector<std::vector<int>> adjacency;
    igl::adjacency_list(target.F, adjacency);
    std::vector<char> toSmooth(n, 0);
    for (int i = 0; i < n; ++i) {
        toSmooth[i] = !matched(i);
    }
    for (int seed = 0; seed < n; ++seed) {
        if (matched(seed)) {
            continue;
        }
        // Grow from each unmatched vertex over neighbours within `distance` of the seed.
        std::vector<int> stack{seed};
        while (!stack.empty()) {
            const int v = stack.back();
            stack.pop_back();
            for (int nb : adjacency[v]) {
                if (!toSmooth[nb] && (target.V.row(seed) - target.V.row(nb)).norm() < distance) {
                    toSmooth[nb] = 1;
                    stack.push_back(nb);
                }
            }
        }
    }
    for (int step = 0; step < params.smoothSteps; ++step) {
        for (int i = 0; i < n; ++i) {
            if (!toSmooth[i] || adjacency[i].empty()) {
                continue;
            }
            Eigen::RowVectorXd blended = (1.0 - params.smoothAlpha) * W.row(i);
            for (int nb : adjacency[i]) {
                blended += (params.smoothAlpha / adjacency[i].size()) * W.row(nb);
            }
            W.row(i) = blended;  // in place, so later vertices see earlier updates
        }
    }
}

}  // namespace

TransferResult transferInpaint(const Mesh& source, const Mesh& target, const TransferParams& params) {
    requireSkinnedSource(source);
    const auto start = std::chrono::steady_clock::now();
    const InpaintSetup s = prepareInpaint(source, target, params);
    TransferResult result;
    result.matched = s.matched;

    // Section 3.2: inpaint the unmatched vertices, matched ones held fixed.
    const Eigen::SparseMatrix<double> Aeq;
    const Eigen::VectorXd Beq;
    const Eigen::MatrixXd B = Eigen::MatrixXd::Zero(target.V.rows(), s.cp.W.cols());
    if (s.known.size() == target.V.rows()) {
        result.W = s.cp.W;
    } else if (!igl::min_quad_with_fixed(s.Q, B, s.known, s.knownValues, Aeq, Beq, true, result.W)) {
        throw std::runtime_error("transfer: inpainting solve failed");
    }
    if (params.smooth) {
        smoothInpainted(target, s.matched, s.distance, params, result.W);
    }
    result.seconds = elapsed(start);
    return result;
}

namespace {

// Solves the inpainting energy per bone with 0 <= w <= 1 (active set), then
// normalises rows to sum to 1 and runs Epic's smoothing.
TransferResult solveBounded(const InpaintSetup& s, const Mesh& target, const TransferParams& params) {
    TransferResult result;
    result.matched = s.matched;
    const int n = static_cast<int>(target.V.rows());
    const int bones = static_cast<int>(s.cp.W.cols());
    result.W.resize(n, bones);
    const Eigen::SparseMatrix<double> Aeq;
    const Eigen::VectorXd Beq;
    const Eigen::SparseMatrix<double> Aieq;
    const Eigen::VectorXd Bieq;
    const Eigen::VectorXd lx = Eigen::VectorXd::Zero(n);
    const Eigen::VectorXd ux = Eigen::VectorXd::Ones(n);
    const Eigen::VectorXd B = Eigen::VectorXd::Zero(n);
    igl::active_set_params asp;
    asp.max_iter = 50;
    for (int j = 0; j < bones; ++j) {
        // Matched values can sit a hair outside [0, 1] after interpolation; clamp them.
        const Eigen::VectorXd y = s.knownValues.col(j).cwiseMax(0.0).cwiseMin(1.0);
        // active_set only checks for bound violations against a previous
        // iterate, so without a warm start it accepts the unconstrained solve.
        Eigen::VectorXd z = Eigen::VectorXd::Zero(n);
        const igl::SolverStatus status = igl::active_set(s.Q, B, s.known, y, Aeq, Beq, Aieq, Bieq, lx, ux, asp, z);
        if (status == igl::SOLVER_STATUS_ERROR) {
            throw std::runtime_error("transfer: bounded solve failed for bone " + std::to_string(j));
        }
        result.W.col(j) = z;
    }
    for (int i = 0; i < n; ++i) {
        const double sum = result.W.row(i).sum();
        if (sum > 1e-12) {
            result.W.row(i) /= sum;
        }
    }
    if (params.smooth) {
        smoothInpainted(target, s.matched, s.distance, params, result.W);
    }
    return result;
}

// Rebuilds `known`/`knownValues` after `matched` has been edited.
void refreshKnown(InpaintSetup& s) {
    const int matchedCount = static_cast<int>(s.matched.count());
    s.known.resize(matchedCount);
    s.knownValues.resize(matchedCount, s.cp.W.cols());
    for (int i = 0, k = 0; i < s.matched.size(); ++i) {
        if (s.matched(i)) {
            s.known(k) = i;
            s.knownValues.row(k) = s.cp.W.row(i);
            ++k;
        }
    }
}

}  // namespace

TransferResult transferBounded(const Mesh& source, const Mesh& target, const TransferParams& params) {
    requireSkinnedSource(source);
    const auto start = std::chrono::steady_clock::now();
    const InpaintSetup s = prepareInpaint(source, target, params);
    TransferResult result = solveBounded(s, target, params);
    result.seconds = elapsed(start);
    return result;
}

TransferResult transferAnchored(const Mesh& source, const Mesh& target, const TransferParams& params) {
    requireSkinnedSource(source);
    const auto start = std::chrono::steady_clock::now();
    InpaintSetup s = prepareInpaint(source, target, params);

    // Every connected component needs at least one matched vertex or its
    // rows of the system are singular. For a component with none, match the
    // vertices nearest the body (within anchorSlack of the nearest), with the
    // thresholds waived: a button has to follow *something*, and the closest
    // bone is the least surprising choice.
    Eigen::SparseMatrix<double> A;
    igl::adjacency_matrix(target.F, A);
    Eigen::VectorXi component;
    Eigen::VectorXi componentSize;
    const int components = igl::connected_components(A, component, componentSize);
    std::vector<char> hasMatch(components, 0);
    std::vector<double> nearest(components, std::numeric_limits<double>::infinity());
    for (int i = 0; i < s.matched.size(); ++i) {
        hasMatch[component(i)] |= s.matched(i);
        nearest[component(i)] = std::min(nearest[component(i)], s.cp.sqrD(i));
    }
    for (int i = 0; i < s.matched.size(); ++i) {
        const int c = component(i);
        if (!hasMatch[c] && s.cp.sqrD(i) <= nearest[c] * params.anchorSlack * params.anchorSlack) {
            s.matched(i) = true;
        }
    }
    refreshKnown(s);

    TransferResult result = solveBounded(s, target, params);
    result.seconds = elapsed(start);
    return result;
}

TransferResult runTransfer(const std::string& method, const Mesh& source, const Mesh& target,
                           const TransferParams& params) {
    if (method == "closest") {
        return transferClosestPoint(source, target, params);
    }
    if (method == "inpaint") {
        return transferInpaint(source, target, params);
    }
    if (method == "bounded") {
        return transferBounded(source, target, params);
    }
    if (method == "anchored") {
        return transferAnchored(source, target, params);
    }
    if (method == "relative") {
        TransferParams p = params;
        p.relativeMatch = true;
        return transferAnchored(source, target, p);
    }
    if (method == "consistent") {
        TransferParams p = params;
        p.relativeMatch = true;
        p.consistencyCheck = true;
        return transferAnchored(source, target, p);
    }
    throw std::runtime_error("unknown transfer method: " + method);
}

std::vector<std::string> transferMethodNames() {
    return {"closest", "inpaint", "bounded", "anchored", "relative", "consistent"};
}

Eigen::MatrixXd pruneInfluences(const Eigen::MatrixXd& W, int maxInfluences) {
    Eigen::MatrixXd out = Eigen::MatrixXd::Zero(W.rows(), W.cols());
    std::vector<int> order(W.cols());
    for (int i = 0; i < W.rows(); ++i) {
        for (int j = 0; j < W.cols(); ++j) {
            order[j] = j;
        }
        std::partial_sort(order.begin(), order.begin() + std::min<int>(maxInfluences, W.cols()), order.end(),
                          [&](int a, int b) { return W(i, a) > W(i, b); });
        double sum = 0.0;
        for (int k = 0; k < maxInfluences && k < W.cols(); ++k) {
            const double w = std::max(W(i, order[k]), 0.0);
            out(i, order[k]) = w;
            sum += w;
        }
        if (sum > 0.0) {
            out.row(i) /= sum;
        }
    }
    return out;
}

}  // namespace acm::skin
