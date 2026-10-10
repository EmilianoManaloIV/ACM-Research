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
