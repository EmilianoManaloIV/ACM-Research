#include "acm/skin/refine.h"

#include <igl/bounding_box_diagonal.h>
#include <igl/edges.h>
#include <igl/signed_distance.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace acm::skin {

namespace {

// Euclidean projection onto the probability simplex (Duchi et al. 2008).
void projectSimplex(Eigen::VectorXd& w) {
    const int k = static_cast<int>(w.size());
    std::vector<double> u(w.data(), w.data() + k);
    std::sort(u.begin(), u.end(), std::greater<double>());
    double cumulative = 0.0;
    double theta = 0.0;
    for (int i = 0; i < k; ++i) {
        cumulative += u[i];
        const double t = (cumulative - 1.0) / static_cast<double>(i + 1);
        if (u[i] - t > 0.0) {
            theta = t;
        }
    }
    for (int i = 0; i < k; ++i) {
        w(i) = std::max(w(i) - theta, 0.0);
    }
}

}  // namespace

RefineResult refinePoseAware(const Rig& rig, const Mesh& garment, const Eigen::MatrixXd& W0,
                             const std::vector<Pose>& trainPoses, const RefineParams& params) {
    const int n = static_cast<int>(garment.V.rows());
    const int bones = static_cast<int>(W0.cols());
    const int poses = static_cast<int>(trainPoses.size());
    const double scale = igl::bounding_box_diagonal(rig.body.V);
    RefineResult result;
    result.W = W0;
    if (poses == 0 || n == 0) {
        return result;
    }

    Eigen::MatrixXi E;
    igl::edges(garment.F, E);
    Eigen::VectorXd restLength(E.rows());
    for (int e = 0; e < E.rows(); ++e) {
        restLength(e) = (garment.V.row(E(e, 0)) - garment.V.row(E(e, 1))).norm();
    }
    // Rest clearance from the body, the distance each vertex should keep.
    Eigen::VectorXd clearance;
    {
        Eigen::VectorXi I;
        Eigen::MatrixXd C, N;
        igl::signed_distance(garment.V, rig.body.V, rig.body.F, igl::SIGNED_DISTANCE_TYPE_FAST_WINDING_NUMBER,
                             clearance, I, C, N);
        clearance = clearance.cwiseMax(0.0) * params.clearanceShare;
    }

    // Targets: one relaxed garment per training pose.
    std::vector<std::vector<Eigen::Matrix4d>> matrices(poses);
    std::vector<Eigen::MatrixXd> targets(poses);
    double moved = 0.0;
    for (int p = 0; p < poses; ++p) {
        matrices[p] = skinningMatrices(rig.skeleton, trainPoses[p]);
        const Eigen::MatrixXd body = skin(rig.body.V, rig.body.W, matrices[p], params.skinning);
        const Eigen::MatrixXd start = skin(garment.V, W0, matrices[p], params.skinning);
        Eigen::MatrixXd G = start;
        for (int it = 0; it < params.relaxIterations; ++it) {
            // Collision: push vertices out along the body's closest-point direction.
            Eigen::VectorXd S;
            Eigen::VectorXi I;
            Eigen::MatrixXd C, N;
            igl::signed_distance(G, body, rig.body.F, igl::SIGNED_DISTANCE_TYPE_FAST_WINDING_NUMBER, S, I, C, N);
            for (int i = 0; i < n; ++i) {
                if (S(i) < clearance(i)) {
                    Eigen::RowVector3d dir = G.row(i) - C.row(i);
                    const double len = dir.norm();
                    if (len < 1e-12 || S(i) < 0.0) {
                        dir = N.row(i);  // inside: the surface normal points out
                    } else {
                        dir /= len;
                    }
                    G.row(i) = C.row(i) + clearance(i) * dir;
                }
            }
            // Stretch: Jacobi projection of every edge toward its rest length.
            for (int pass = 0; pass < params.stretchPasses; ++pass) {
                Eigen::MatrixXd delta = Eigen::MatrixXd::Zero(n, 3);
                Eigen::VectorXd count = Eigen::VectorXd::Zero(n);
                for (int e = 0; e < E.rows(); ++e) {
                    const int a = E(e, 0), b = E(e, 1);
                    Eigen::RowVector3d d = G.row(b) - G.row(a);
                    const double len = d.norm();
                    if (len < 1e-12) {
                        continue;
                    }
                    const Eigen::RowVector3d correction = 0.5 * params.stretchStiffness * (len - restLength(e)) * d / len;
                    delta.row(a) += correction;
                    delta.row(b) -= correction;
                    count(a) += 1.0;
                    count(b) += 1.0;
                }
                for (int i = 0; i < n; ++i) {
                    if (count(i) > 0.0) {
                        G.row(i) += delta.row(i) / count(i);
                    }
                }
            }
        }
        targets[p] = G;
        moved += (G - start).rowwise().norm().mean();
    }
    result.targetChange = moved / poses / scale;

    // Fit: per vertex, weights on the current support that reproduce the
    // targets under LBS, with a pull toward the current weights.
    double residual = 0.0;
    const int k = std::min(params.support, bones);
    std::vector<int> order(bones);
    for (int i = 0; i < n; ++i) {
        std::iota(order.begin(), order.end(), 0);
        std::partial_sort(order.begin(), order.begin() + k, order.end(),
                          [&](int a, int b) { return W0(i, a) > W0(i, b); });
        Eigen::MatrixXd A(3 * poses, k);
        Eigen::VectorXd b(3 * poses);
        const Eigen::Vector4d v(garment.V(i, 0), garment.V(i, 1), garment.V(i, 2), 1.0);
        for (int p = 0; p < poses; ++p) {
            for (int c = 0; c < k; ++c) {
                A.block<3, 1>(3 * p, c) = (matrices[p][order[c]] * v).head<3>();
            }
            b.segment<3>(3 * p) = targets[p].row(i).transpose();
        }
        Eigen::VectorXd w0(k);
        for (int c = 0; c < k; ++c) {
            w0(c) = W0(i, order[c]);
        }
        const double s0 = w0.sum();
        if (s0 > 0.0) {
            w0 /= s0;
        }
        // Scale the prior so it is comparable to one pose's squared error.
        const double mu = params.prior * (A.squaredNorm() / static_cast<double>(k));
        const Eigen::MatrixXd H = A.transpose() * A + mu * Eigen::MatrixXd::Identity(k, k);
        const Eigen::VectorXd g0 = A.transpose() * b + mu * w0;
        const double step = 1.0 / (H.operatorNorm() + 1e-12);
        Eigen::VectorXd w = w0;
        for (int it = 0; it < params.fitIterations; ++it) {
            w -= step * (H * w - g0);
            projectSimplex(w);
        }
        result.W.row(i).setZero();
        for (int c = 0; c < k; ++c) {
            result.W(i, order[c]) = w(c);
        }
        const Eigen::VectorXd r = A * w - b;
        for (int p = 0; p < poses; ++p) {
            residual += r.segment<3>(3 * p).norm() / poses;
        }
    }
    result.fitResidual = residual / n / scale;
    return result;
}

}  // namespace acm::skin
