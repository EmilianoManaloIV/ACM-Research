#include "acm/skin/autoweight.h"

#include <igl/bbw.h>
#include <igl/boundary_conditions.h>
#include <igl/copyleft/tetgen/tetrahedralize.h>

#include <algorithm>
#include <array>
#include <map>
#include <cmath>
#include <cstdio>
#include <limits>

namespace acm::skin {

namespace {

double segmentDistance(const Eigen::Vector3d& p, const Eigen::Vector3d& a, const Eigen::Vector3d& b) {
    const Eigen::Vector3d ab = b - a;
    const double len2 = ab.squaredNorm();
    const double t = len2 > 0.0 ? std::clamp((p - a).dot(ab) / len2, 0.0, 1.0) : 0.0;
    return (p - (a + t * ab)).norm();
}

}  // namespace

std::vector<BoneSegment> boneSegments(const Skeleton& skeleton) {
    const int n = skeleton.size();
    const std::vector<Eigen::Vector3d> position = restJointPositions(skeleton);
    std::vector<std::vector<int>> children(n);
    for (int j = 0; j < n; ++j) {
        if (skeleton.parent[j] >= 0) {
            children[skeleton.parent[j]].push_back(j);
        }
    }
    std::vector<BoneSegment> bones(n);
    for (int j = 0; j < n; ++j) {
        BoneSegment& bone = bones[j];
        bone.joint = j;
        bone.a = position[j];
        bone.b = position[j];
        if (!children[j].empty()) {
            Eigen::Vector3d mean = Eigen::Vector3d::Zero();
            for (const int c : children[j]) {
                mean += position[c];
            }
            mean /= static_cast<double>(children[j].size());
            bone.b = (mean - position[j]).norm() > 1e-9 ? mean : position[children[j].front()];
        } else if (skeleton.parent[j] >= 0) {
            bone.b = position[j] + 0.5 * (position[j] - position[skeleton.parent[j]]);
        }
    }
    return bones;
}

bool saveBoneSegments(const std::string& path, const Skeleton& skeleton, const std::vector<BoneSegment>& bones) {
    std::FILE* f = std::fopen(path.c_str(), "w");
    if (!f) {
        return false;
    }
    const std::vector<Eigen::Matrix4d> global = restGlobals(skeleton);
    std::fprintf(f, "{\"bones\": [\n");
    for (size_t j = 0; j < bones.size(); ++j) {
        const int p = skeleton.parent[j];
        const Eigen::Vector3d local = (global[j].inverse() * bones[j].b.homogeneous()).head<3>();
        std::fprintf(f,
                     "  {\"name\": \"%s\", \"parent\": %s%s%s, \"head\": [%.9g, %.9g, %.9g], \"tail\": [%.9g, %.9g, %.9g], "
                     "\"local_tail\": [%.9g, %.9g, %.9g]}%s\n",
                     skeleton.names[j].c_str(), p >= 0 ? "\"" : "", p >= 0 ? skeleton.names[p].c_str() : "null",
                     p >= 0 ? "\"" : "", bones[j].a.x(), bones[j].a.y(), bones[j].a.z(), bones[j].b.x(), bones[j].b.y(),
                     bones[j].b.z(), local.x(), local.y(), local.z(), j + 1 < bones.size() ? "," : "");
    }
    std::fprintf(f, "]}\n");
    return std::fclose(f) == 0;
}

Eigen::MatrixXd inverseDistanceWeights(const Eigen::MatrixXd& V, const std::vector<BoneSegment>& bones, int joints,
                                       double power) {
    Eigen::MatrixXd W = Eigen::MatrixXd::Zero(V.rows(), joints);
    for (int i = 0; i < V.rows(); ++i) {
        const Eigen::Vector3d p = V.row(i).transpose();
        for (const BoneSegment& bone : bones) {
            const double d = std::max(segmentDistance(p, bone.a, bone.b), 1e-12);
            W(i, bone.joint) += 1.0 / std::pow(d, power);
        }
    }
    normaliseRows(W);
    return W;
}

AutoWeightResult boundedBiharmonicWeights(const Mesh& body, const std::vector<BoneSegment>& bones, int joints,
                                          int samplesPerBone) {
    AutoWeightResult result;
    const int n = static_cast<int>(body.V.rows());

    // Handles: one bone edge per non-degenerate segment, a point handle for
    // the rest. C holds both endpoints of every bone, then the points.
    std::vector<int> edgeJoint, pointJoint;
    for (const BoneSegment& bone : bones) {
        ((bone.b - bone.a).norm() > 1e-9 ? edgeJoint : pointJoint).push_back(bone.joint);
    }
    const int ne = static_cast<int>(edgeJoint.size());
    const int np = static_cast<int>(pointJoint.size());
    Eigen::MatrixXd C(2 * ne + np, 3);
    Eigen::MatrixXi BE(ne, 2);
    Eigen::VectorXi P(np);
    for (int e = 0; e < ne; ++e) {
        C.row(2 * e) = bones[edgeJoint[e]].a.transpose();
        C.row(2 * e + 1) = bones[edgeJoint[e]].b.transpose();
        BE(e, 0) = 2 * e;
        BE(e, 1) = 2 * e + 1;
    }
    for (int k = 0; k < np; ++k) {
        C.row(2 * ne + k) = bones[pointJoint[k]].a.transpose();
        P(k) = 2 * ne + k;
    }

    // Surface plus bone samples (endpoints included) as isolated interior points.
    const int samples = std::max(samplesPerBone, 2);
    Eigen::MatrixXd V(n + ne * samples + np, 3);
    V.topRows(n) = body.V;
    int row = n;
    for (int e = 0; e < ne; ++e) {
        for (int s = 0; s < samples; ++s) {
            const double t = static_cast<double>(s) / (samples - 1);
            V.row(row++) = ((1.0 - t) * bones[edgeJoint[e]].a + t * bones[edgeJoint[e]].b).transpose();
        }
    }
    for (int k = 0; k < np; ++k) {
        V.row(row++) = bones[pointJoint[k]].a.transpose();
    }

    Eigen::MatrixXd TV;
    Eigen::MatrixXi TT, TF;
    // p: tetrahedralise the surface; q: quality; Y: keep the surface as given.
    const int status = igl::copyleft::tetgen::tetrahedralize(V, body.F, "pq1.414Y", TV, TT, TF);
    if (status != 0) {
        result.note = "tetgen failed with status " + std::to_string(status);
        return result;
    }
    // TetGen keeps input points first, but merges coincident ones (meshes
    // split at UV seams have many), so find each surface vertex's tet vertex
    // by position. One with no match was dropped, and the solve has no value
    // for it.
    std::vector<int> tetOf(n, -1);
    {
        std::map<std::array<double, 3>, int> at;
        for (int i = 0; i < TV.rows(); ++i) {
            at.emplace(std::array<double, 3>{TV(i, 0), TV(i, 1), TV(i, 2)}, i);
        }
        for (int i = 0; i < n; ++i) {
            const auto it = at.find({body.V(i, 0), body.V(i, 1), body.V(i, 2)});
            if (it == at.end()) {
                result.note = "tetgen dropped surface vertex " + std::to_string(i);
                return result;
            }
            tetOf[i] = it->second;
        }
    }

    Eigen::VectorXi b;
    Eigen::MatrixXd bc;
    const Eigen::MatrixXi CE, CF;
    if (!igl::boundary_conditions(TV, TT, C, P, BE, CE, CF, b, bc)) {
        result.note = "boundary conditions suspicious: a bone has no samples inside the tetrahedral mesh";
        return result;
    }

    igl::BBWData data;
    data.active_set_params.max_iter = 8;  // libigl tutorial 403
    data.verbosity = 0;
    Eigen::MatrixXd Wh;
    if (!igl::bbw(TV, TT, b, bc, data, Wh)) {
        result.note = "bbw solve failed";
        return result;
    }

    // Handle columns are ordered [P; BE]; map them back to joints.
    result.W = Eigen::MatrixXd::Zero(n, joints);
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < np; ++k) {
            result.W(i, pointJoint[k]) += Wh(tetOf[i], k);
        }
        for (int e = 0; e < ne; ++e) {
            result.W(i, edgeJoint[e]) += Wh(tetOf[i], np + e);
        }
    }
    normaliseRows(result.W);
    return result;
}

void normaliseRows(Eigen::MatrixXd& W, double epsilon) {
    for (int i = 0; i < W.rows(); ++i) {
        for (int j = 0; j < W.cols(); ++j) {
            if (W(i, j) < 0.0 && W(i, j) > -epsilon) {
                W(i, j) = 0.0;
            }
        }
        const double sum = W.row(i).sum();
        if (sum > 0.0 && std::isfinite(sum)) {
            W.row(i) /= sum;
        }
    }
}

bool validWeights(const Eigen::MatrixXd& W, int& unweighted, double tolerance) {
    unweighted = 0;
    bool valid = W.size() > 0;
    for (int i = 0; i < W.rows(); ++i) {
        double sum = 0.0;
        for (int j = 0; j < W.cols(); ++j) {
            const double w = W(i, j);
            if (!std::isfinite(w) || w < 0.0) {
                valid = false;
            }
            sum += w;
        }
        if (sum == 0.0) {
            ++unweighted;
        }
        if (!(std::abs(sum - 1.0) <= tolerance)) {
            valid = false;
        }
    }
    return valid;
}

WeightAgreement compareWeights(const Eigen::MatrixXd& W, const Eigen::MatrixXd& reference, double threshold) {
    WeightAgreement a;
    if (W.rows() != reference.rows() || W.cols() != reference.cols() || W.rows() == 0) {
        a.l1 = std::numeric_limits<double>::quiet_NaN();
        a.precision = a.recall = a.l1;
        return a;
    }
    long predicted = 0, actual = 0, both = 0;
    for (int i = 0; i < W.rows(); ++i) {
        for (int j = 0; j < W.cols(); ++j) {
            const bool p = W(i, j) > threshold;
            const bool r = reference(i, j) > threshold;
            predicted += p;
            actual += r;
            both += p && r;
        }
    }
    a.l1 = (W - reference).cwiseAbs().rowwise().sum().mean();
    a.precision = predicted > 0 ? static_cast<double>(both) / predicted : 0.0;
    a.recall = actual > 0 ? static_cast<double>(both) / actual : 0.0;
    return a;
}

}  // namespace acm::skin
