// Weight transfer methods. transferInpaint() follows Epic's released Python
// (github.com/rin-23/RobustSkinWeightsTransferCode, MIT) step for step so the
// C++ can be checked against it; see tools/verify_inpaint.py.

#include "acm/skin/transfer.h"

#include <igl/active_set.h>
#include <igl/adjacency_list.h>
#include <igl/barycentric_coordinates.h>
#include <igl/bounding_box_diagonal.h>
#include <igl/cotmatrix.h>
#include <igl/massmatrix.h>
#include <igl/min_quad_with_fixed.h>
#include <igl/point_mesh_squared_distance.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace acm::skin {

namespace {

struct ClosestPoints {
    Eigen::VectorXd sqrD;   // squared distance per target vertex
    Eigen::MatrixXd W;      // source weights interpolated at the closest point
    Eigen::MatrixXd N;      // source normals interpolated at the closest point
};

ClosestPoints closestPoints(const Mesh& source, const Mesh& target) {
    Eigen::VectorXi I;
    Eigen::MatrixXd C;
    ClosestPoints out;
    igl::point_mesh_squared_distance(target.V, source.V, source.F, out.sqrD, I, C);

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
    for (int i = 0; i < n; ++i) {
        const Eigen::Vector3d n1 = s.cp.N.row(i).normalized();
        const Eigen::Vector3d n2 = target.N.row(i).normalized();
        const double degrees = std::acos(std::clamp(n1.dot(n2), -1.0, 1.0)) * 180.0 / M_PI;
        s.matched(i) = s.cp.sqrD(i) <= sqrDistance && degrees <= params.angleDegrees;
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

TransferResult transferBounded(const Mesh& source, const Mesh& target, const TransferParams& params) {
    requireSkinnedSource(source);
    const auto start = std::chrono::steady_clock::now();
    const InpaintSetup s = prepareInpaint(source, target, params);
    TransferResult result;
    result.matched = s.matched;

    // Same energy as inpaint, but each bone is solved with 0 <= w <= 1 by an
    // active-set method (as bounded biharmonic weights do), then rows are
    // normalised to sum to 1.
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
    throw std::runtime_error("unknown transfer method: " + method);
}

std::vector<std::string> transferMethodNames() {
    return {"closest", "inpaint", "bounded"};
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
