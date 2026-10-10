#include "acm/skin/rig.h"

#include <igl/per_vertex_normals.h>

#include <cmath>
#include <fstream>
#include <functional>
#include <sstream>

namespace acm::skin {

void Mesh::computeNormals() {
    igl::per_vertex_normals(V, F, N);
}

int Skeleton::find(const std::string& name) const {
    for (int j = 0; j < size(); ++j) {
        if (names[j] == name) {
            return j;
        }
    }
    return -1;
}

Pose identityPose(const Skeleton& skeleton, const std::string& name) {
    Pose pose;
    pose.name = name;
    pose.rotation.assign(skeleton.size(), Eigen::Quaterniond::Identity());
    return pose;
}

std::vector<Eigen::Matrix4d> restGlobals(const Skeleton& skeleton) {
    // Walks restLocal only, so it is safe to call while driven joints are
    // being added (skinningMatrices would try to drive them).
    const int n = skeleton.size();
    std::vector<Eigen::Matrix4d> global(n);
    std::vector<char> done(n, 0);
    const std::function<const Eigen::Matrix4d&(int)> globalOf = [&](int j) -> const Eigen::Matrix4d& {
        if (!done[j]) {
            global[j] = skeleton.parent[j] < 0 ? skeleton.restLocal[j]
                                               : globalOf(skeleton.parent[j]) * skeleton.restLocal[j];
            done[j] = 1;
        }
        return global[j];
    };
    for (int j = 0; j < n; ++j) {
        globalOf(j);
    }
    return global;
}

std::vector<Eigen::Vector3d> restJointPositions(const Skeleton& skeleton) {
    const std::vector<Eigen::Matrix4d> G = restGlobals(skeleton);
    std::vector<Eigen::Vector3d> position(G.size());
    for (size_t j = 0; j < G.size(); ++j) {
        position[j] = G[j].block<3, 1>(0, 3);
    }
    return position;
}

Eigen::VectorXd boneLengths(const Skeleton& skeleton) {
    const int n = skeleton.size();
    const std::vector<Eigen::Vector3d> position = restJointPositions(skeleton);
    Eigen::VectorXd length = Eigen::VectorXd::Zero(n);
    for (int j = 0; j < n; ++j) {
        const int p = skeleton.parent[j];
        if (p >= 0 && length(p) == 0.0) {
            length(p) = (position[j] - position[p]).norm();
        }
    }
    return length;
}

namespace {

Eigen::Matrix4d rotateAbout(const Eigen::Vector3d& pivot, const Eigen::Quaterniond& q) {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    m.block<3, 3>(0, 0) = q.toRotationMatrix();
    m.block<3, 1>(0, 3) = pivot - q * pivot;
    return m;
}

// Tilt of a hem bone toward its azimuth: the largest leg swing in that
// direction, with each leg's swing angle scaled by how much of it points
// along the bone's azimuth (a leg swinging straight at the bone counts in
// full, one swinging sideways to it not at all).
double hemTilt(const DrivenJoint& d, const std::vector<Eigen::Vector3d>& rest,
               const std::vector<Eigen::Matrix4d>& M, int tier) {
    double tilt = 0.0;
    for (const auto& leg : d.legs) {
        const int a = tier == 0 ? leg[0] : leg[1];
        const int b = tier == 0 ? leg[1] : leg[2];
        const Eigen::Vector3d seg0 = (rest[b] - rest[a]).normalized();
        const Eigen::Matrix3d Rseg = M[a].block<3, 3>(0, 0);
        Eigen::Vector3d seg = Rseg * seg0;
        if (tier == 1 && !d.lowerHangs) {
            // The shank's swing relative to its thigh, in the thigh's rest frame.
            seg = M[leg[0]].block<3, 3>(0, 0).transpose() * seg;
        }
        const double swing = std::acos(std::max(-1.0, std::min(1.0, seg.dot(seg0))));
        Eigen::Vector3d horizontal = seg - seg.dot(d.down) * d.down;
        const double h = horizontal.norm();
        if (h < 1e-9) {
            continue;
        }
        horizontal /= h;
        tilt = std::max(tilt, swing * std::pow(std::max(0.0, horizontal.dot(d.direction)), d.falloff));
    }
    return d.follow * tilt;
}

}  // namespace

namespace {

// Where a hem bone pivots: the hip-line midpoint, or (v2) a point slid
// along the hip line toward the bone's side, so side bones turn about
// their own hip and front/back bones about the middle.
Eigen::Vector3d hemPivot(const DrivenJoint& d, const std::vector<Eigen::Vector3d>& rest) {
    if (!d.pivotOnHips || d.legs.size() < 2) {
        return d.pivot;
    }
    Eigen::Vector3d across = rest[d.legs.back()[0]] - rest[d.legs.front()[0]];
    across -= across.dot(d.down) * d.down;
    return d.pivot + 0.5 * d.direction.dot(across.normalized()) * across;
}

}  // namespace

namespace {

double pointSegmentDistance(const Eigen::Vector3d& p, const Eigen::Vector3d& a, const Eigen::Vector3d& b) {
    const Eigen::Vector3d ab = b - a;
    const double t = std::max(0.0, std::min(1.0, (p - a).dot(ab) / std::max(ab.squaredNorm(), 1e-12)));
    return (p - (a + t * ab)).norm();
}

// v2 contact: the smallest tilt at or above `tilt` for which the panel the
// hem bone carries clears every thigh it is being pushed by, sampled down
// the panel at the bone's horizontal radius (the bone axis itself runs down
// the middle between the thighs, so sampling it would never clear).
double hemContactTilt(const DrivenJoint& d, const std::vector<Eigen::Vector3d>& rest,
                      const std::vector<Eigen::Matrix4d>& M, double tilt, const Eigen::Vector3d& pivot) {
    if (d.legRadius.size() != d.legs.size()) {
        return tilt;
    }
    for (double t = tilt; t <= M_PI / 2; t += M_PI / 90) {
        const Eigen::Vector3d dir = std::cos(t) * d.down + std::sin(t) * d.direction;
        const Eigen::Matrix3d R = Eigen::Quaterniond::FromTwoVectors(d.down, dir).toRotationMatrix();
        bool clear = true;
        for (size_t l = 0; l < d.legs.size() && clear; ++l) {
            const auto& leg = d.legs[l];
            const Eigen::Vector3d a = (M[leg[0]] * rest[leg[0]].homogeneous()).head<3>();
            const Eigen::Vector3d b = (M[leg[0]] * rest[leg[1]].homogeneous()).head<3>();
            // Only a thigh swung toward this bone can push it.
            Eigen::Vector3d horizontal = (b - a);
            horizontal -= horizontal.dot(d.down) * d.down;
            if (horizontal.norm() < 1e-9 || horizontal.normalized().dot(d.direction) <= 0.0) {
                continue;
            }
            for (double s = 0.25; s <= 1.0 && clear; s += 0.25) {
                // The panel this bone carries, `radius` out from the pivot at its azimuth, rotated with the bone.
                const Eigen::Vector3d p = pivot + R * (d.radius * d.direction + s * d.length * d.down);
                clear = pointSegmentDistance(p, a, b) >= d.legRadius[l] + d.contactClearance;
            }
        }
        if (clear) {
            return t;
        }
    }
    return tilt;
}

}  // namespace

// Replaces the matrices of the driven joints with their leg-driven poses.
void driveHemJoints(const Skeleton& skeleton, std::vector<Eigen::Matrix4d>& M) {
    const std::vector<Eigen::Vector3d> rest = restJointPositions(skeleton);
    for (size_t k = 0; k < skeleton.driven.size(); ++k) {
        const DrivenJoint& d = skeleton.driven[k];
        if (d.tier == 0) {
            double tilt = hemTilt(d, rest, M, 0);
            tilt = hemContactTilt(d, rest, M, tilt, hemPivot(d, rest));
            const Eigen::Vector3d dir = std::cos(tilt) * d.down + std::sin(tilt) * d.direction;
            M[d.joint] = rotateAbout(hemPivot(d, rest), Eigen::Quaterniond::FromTwoVectors(d.down, dir));
        } else {
            const DrivenJoint& parent = skeleton.driven[d.parentDriven];
            const double tilt = hemTilt(d, rest, M, 1);
            const Eigen::Vector3d dir = std::cos(tilt) * d.down + std::sin(tilt) * d.direction;
            const Eigen::Vector3d knee = hemPivot(parent, rest) + parent.length * d.down;
            if (d.lowerHangs) {
                // Hang from the posed knee point, oriented in world: the
                // parent's tilt is undone and only a shank's own swing tilts it.
                const Eigen::Vector3d kneePosed = (M[parent.joint] * knee.homogeneous()).head<3>();
                Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
                m.block<3, 3>(0, 0) = Eigen::Quaterniond::FromTwoVectors(d.down, dir).toRotationMatrix();
                m.block<3, 1>(0, 3) = kneePosed - m.block<3, 3>(0, 0) * knee;
                M[d.joint] = m;
            } else {
                M[d.joint] = M[parent.joint] * rotateAbout(knee, Eigen::Quaterniond::FromTwoVectors(d.down, dir));
            }
        }
    }
}

std::vector<Eigen::Matrix4d> skinningMatrices(const Skeleton& skeleton, const Pose& pose) {
    const int n = skeleton.size();
    std::vector<Eigen::Matrix4d> global(n);
    std::vector<char> done(n, 0);
    // Joint order is whatever the file used, so resolve parents on demand.
    const std::function<const Eigen::Matrix4d&(int)> globalOf = [&](int j) -> const Eigen::Matrix4d& {
        if (!done[j]) {
            Eigen::Matrix4d extra = Eigen::Matrix4d::Identity();
            if (j < static_cast<int>(pose.rotation.size())) {
                extra.block<3, 3>(0, 0) = pose.rotation[j].normalized().toRotationMatrix();
            }
            const Eigen::Matrix4d local = skeleton.restLocal[j] * extra;
            global[j] = skeleton.parent[j] < 0 ? local : globalOf(skeleton.parent[j]) * local;
            done[j] = 1;
        }
        return global[j];
    };
    std::vector<Eigen::Matrix4d> result(n);
    for (int j = 0; j < n; ++j) {
        result[j] = globalOf(j) * skeleton.inverseBind[j];
    }
    if (!skeleton.driven.empty()) {
        driveHemJoints(skeleton, result);
    }
    return result;
}

Eigen::MatrixXd linearBlendSkin(const Eigen::MatrixXd& V, const Eigen::MatrixXd& W,
                                const std::vector<Eigen::Matrix4d>& matrices) {
    Eigen::MatrixXd out = Eigen::MatrixXd::Zero(V.rows(), 3);
    const int joints = static_cast<int>(matrices.size());
    for (int i = 0; i < V.rows(); ++i) {
        const Eigen::Vector4d p(V(i, 0), V(i, 1), V(i, 2), 1.0);
        Eigen::Vector4d q = Eigen::Vector4d::Zero();
        for (int j = 0; j < joints && j < W.cols(); ++j) {
            const double w = W(i, j);
            if (w != 0.0) {
                q += w * (matrices[j] * p);
            }
        }
        out.row(i) = q.head<3>().transpose();
    }
    return out;
}

namespace {

// A rigid transform as a unit dual quaternion: real part the rotation,
// dual part 0.5 * t * q.
struct DualQuat {
    Eigen::Quaterniond real;
    Eigen::Quaterniond dual;
};

DualQuat toDualQuat(const Eigen::Matrix4d& m) {
    DualQuat d;
    d.real = Eigen::Quaterniond(m.block<3, 3>(0, 0)).normalized();
    const Eigen::Quaterniond t(0.0, m(0, 3), m(1, 3), m(2, 3));
    const Eigen::Quaterniond tq = t * d.real;
    d.dual = Eigen::Quaterniond(0.5 * tq.w(), 0.5 * tq.x(), 0.5 * tq.y(), 0.5 * tq.z());
    return d;
}

}  // namespace

Eigen::MatrixXd dualQuaternionSkin(const Eigen::MatrixXd& V, const Eigen::MatrixXd& W,
                                   const std::vector<Eigen::Matrix4d>& matrices) {
    const int joints = static_cast<int>(matrices.size());
    std::vector<DualQuat> dq(joints);
    for (int j = 0; j < joints; ++j) {
        dq[j] = toDualQuat(matrices[j]);
    }
    Eigen::MatrixXd out = Eigen::MatrixXd::Zero(V.rows(), 3);
    for (int i = 0; i < V.rows(); ++i) {
        // Pivot on the heaviest joint so antipodal quaternions blend the short way.
        int pivot = -1;
        double best = 0.0;
        for (int j = 0; j < joints && j < W.cols(); ++j) {
            if (W(i, j) > best) {
                best = W(i, j);
                pivot = j;
            }
        }
        if (pivot < 0) {
            out.row(i) = V.row(i);
            continue;
        }
        Eigen::Vector4d real = Eigen::Vector4d::Zero();
        Eigen::Vector4d dual = Eigen::Vector4d::Zero();
        for (int j = 0; j < joints && j < W.cols(); ++j) {
            double w = W(i, j);
            if (w == 0.0) {
                continue;
            }
            if (dq[j].real.dot(dq[pivot].real) < 0.0) {
                w = -w;
            }
            real += w * dq[j].real.coeffs();
            dual += w * dq[j].dual.coeffs();
        }
        const double norm = real.norm();
        if (norm < 1e-12) {
            out.row(i) = linearBlendSkin(V.row(i), W.row(i), matrices).row(0);
            continue;
        }
        real /= norm;
        dual /= norm;
        const Eigen::Quaterniond q(real);
        const Eigen::Quaterniond e(dual);
        // Translation is 2 * dual * conj(real).
        const Eigen::Quaterniond tq = e * q.conjugate();
        const Eigen::Vector3d t(2.0 * tq.x(), 2.0 * tq.y(), 2.0 * tq.z());
        out.row(i) = (q * Eigen::Vector3d(V.row(i)) + t).transpose();
    }
    return out;
}

bool parseSkinningMode(const std::string& text, SkinningMode& mode) {
    if (text == "lbs") {
        mode = SkinningMode::LinearBlend;
        return true;
    }
    if (text == "dqs") {
        mode = SkinningMode::DualQuaternion;
        return true;
    }
    return false;
}

const char* skinningModeName(SkinningMode mode) {
    return mode == SkinningMode::DualQuaternion ? "dqs" : "lbs";
}

Eigen::MatrixXd skin(const Eigen::MatrixXd& V, const Eigen::MatrixXd& W,
                     const std::vector<Eigen::Matrix4d>& matrices, SkinningMode mode) {
    return mode == SkinningMode::DualQuaternion ? dualQuaternionSkin(V, W, matrices)
                                                : linearBlendSkin(V, W, matrices);
}

bool saveWeightsCSV(const std::string& path, const Eigen::MatrixXd& W) {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out.precision(17);
    for (int i = 0; i < W.rows(); ++i) {
        for (int j = 0; j < W.cols(); ++j) {
            out << (j ? "," : "") << W(i, j);
        }
        out << '\n';
    }
    return static_cast<bool>(out);
}

bool loadWeightsCSV(const std::string& path, Eigen::MatrixXd& W) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            row.push_back(std::stod(cell));
        }
        rows.push_back(row);
    }
    if (rows.empty()) {
        return false;
    }
    W.resize(static_cast<int>(rows.size()), static_cast<int>(rows[0].size()));
    for (int i = 0; i < W.rows(); ++i) {
        if (static_cast<int>(rows[i].size()) != W.cols()) {
            return false;
        }
        for (int j = 0; j < W.cols(); ++j) {
            W(i, j) = rows[i][j];
        }
    }
    return true;
}

}  // namespace acm::skin
