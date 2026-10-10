#include "acm/skin/rig.h"

#include <igl/per_vertex_normals.h>

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
