#include "acm/skin/hem.h"

#include <cmath>
#include <stdexcept>

namespace acm::skin {

void measureThighs(const Rig& body, HemRig& hem) {
    const std::vector<Eigen::Vector3d> rest = restJointPositions(body.skeleton);
    hem.legRadius.clear();
    for (const auto& leg : hem.legs) {
        const Eigen::Vector3d a = rest[leg[0]], b = rest[leg[1]];
        const Eigen::Vector3d ab = b - a;
        double sum = 0.0;
        int count = 0;
        for (int i = 0; i < body.body.V.rows(); ++i) {
            int dominant;
            body.body.W.row(i).maxCoeff(&dominant);
            if (dominant != leg[0]) {
                continue;
            }
            const Eigen::Vector3d p = body.body.V.row(i).transpose();
            const double t = std::max(0.0, std::min(1.0, (p - a).dot(ab) / ab.squaredNorm()));
            sum += (p - (a + t * ab)).norm();
            ++count;
        }
        hem.legRadius.push_back(count ? sum / count : 0.0);
    }
}

void addHemJoints(Skeleton& skeleton, HemRig& rig) {
    if (rig.legs.empty()) {
        throw std::runtime_error("hem rig needs at least one leg");
    }
    const std::vector<Eigen::Vector3d> rest = restJointPositions(skeleton);
    Eigen::Vector3d hips = Eigen::Vector3d::Zero(), knees = Eigen::Vector3d::Zero();
    for (const auto& leg : rig.legs) {
        hips += rest[leg[0]];
        knees += rest[leg[1]];
    }
    hips /= static_cast<double>(rig.legs.size());
    knees /= static_cast<double>(rig.legs.size());
    rig.pivot = hips;
    rig.down = (knees - hips).normalized();
    rig.length = (knees - hips).norm();
    // Horizontal basis: from the first hip to the last, then across.
    Eigen::Vector3d across = rest[rig.legs.back()[0]] - rest[rig.legs.front()[0]];
    across -= across.dot(rig.down) * rig.down;
    if (across.norm() < 1e-9) {
        across = std::abs(rig.down.x()) < 0.9 ? Eigen::Vector3d::UnitX() : Eigen::Vector3d::UnitZ();
        across -= across.dot(rig.down) * rig.down;
    }
    rig.e1 = across.normalized();
    rig.e2 = rig.down.cross(rig.e1);

    rig.firstJoint = skeleton.size();
    for (int tier = 0; tier < rig.tiers; ++tier) {
        for (int k = 0; k < rig.count; ++k) {
            const double a = 2.0 * M_PI * k / rig.count;
            DrivenJoint d;
            d.joint = skeleton.size();
            d.tier = tier;
            d.direction = std::cos(a) * rig.e1 + std::sin(a) * rig.e2;
            d.down = rig.down;
            d.pivot = rig.pivot;
            d.length = rig.length;
            d.follow = rig.follow;
            d.falloff = rig.falloff;
            d.pivotOnHips = rig.pivotOnHips;
            if (rig.contact) {
                d.legRadius = rig.legRadius;
                d.contactClearance = rig.contactClearance;
            }
            d.lowerHangs = rig.lowerHangs;
            d.legs = rig.legs;
            d.parentDriven = tier == 0 ? -1 : k;
            skeleton.names.push_back("hem_t" + std::to_string(tier) + "_" + std::to_string(k));
            skeleton.parent.push_back(rig.parentJoint);
            // Rest transforms are identity in world space: the joint's skinning
            // matrix is overwritten by driveHemJoints every pose anyway.
            Eigen::Matrix4d local = Eigen::Matrix4d::Identity();
            if (rig.parentJoint >= 0) {
                local = restGlobals(skeleton)[rig.parentJoint].inverse();
            }
            skeleton.restLocal.push_back(local);
            skeleton.inverseBind.push_back(Eigen::Matrix4d::Identity());
            skeleton.driven.push_back(d);
        }
    }
}

void measureHemRadius(const Mesh& garment, Skeleton& skeleton, const HemRig& rig) {
    std::vector<double> sum(rig.count, 0.0);
    std::vector<int> count(rig.count, 0);
    const double step = 2.0 * M_PI / rig.count;
    for (int i = 0; i < garment.V.rows(); ++i) {
        const Eigen::Vector3d r = garment.V.row(i).transpose() - rig.pivot;
        const double h = r.dot(rig.down) / rig.length;
        if (h < 0.0 || h > 1.2) {
            continue;
        }
        double azimuth = std::atan2(r.dot(rig.e2), r.dot(rig.e1));
        if (azimuth < 0.0) {
            azimuth += 2.0 * M_PI;
        }
        const int k = static_cast<int>(std::lround(azimuth / step)) % rig.count;
        sum[k] += (r - h * rig.length * rig.down).norm();
        ++count[k];
    }
    for (DrivenJoint& d : skeleton.driven) {
        const int k = d.joint - rig.firstJoint;
        if (d.tier == 0 && k >= 0 && k < rig.count) {
            d.radius = count[k] ? sum[k] / count[k] : 0.0;
        }
    }
}

Eigen::MatrixXd assignHemWeights(const Mesh& garment, const Eigen::MatrixXd& W,
                                 const Eigen::Array<bool, Eigen::Dynamic, 1>& matched, const Skeleton& skeleton,
                                 const HemRig& rig) {
    const int n = static_cast<int>(W.rows());
    const int joints = skeleton.size();
    Eigen::MatrixXd out = Eigen::MatrixXd::Zero(n, joints);
    out.leftCols(W.cols()) = W;
    // Joints whose weight moves: the leg chains (hips and everything below) plus the listed extras.
    std::vector<char> moves(joints, 0);
    for (int j : rig.moveFrom) {
        moves[j] = 1;
    }
    for (int j = 0; j < static_cast<int>(W.cols()); ++j) {
        for (int a = j; a >= 0; a = skeleton.parent[a]) {
            for (const auto& leg : rig.legs) {
                if (a == leg[0]) {
                    moves[j] = 1;
                }
            }
        }
    }
    const double step = 2.0 * M_PI / rig.count;
    for (int i = 0; i < n; ++i) {
        if (matched(i) && !rig.applyToMatched) {
            continue;
        }
        double mass = 0.0;
        for (int j = 0; j < static_cast<int>(W.cols()); ++j) {
            if (moves[j]) {
                mass += out(i, j);
                out(i, j) = 0.0;
            }
        }
        if (mass <= 0.0) {
            continue;
        }
        const Eigen::Vector3d r = garment.V.row(i).transpose() - rig.pivot;
        double azimuth = std::atan2(r.dot(rig.e2), r.dot(rig.e1));
        if (azimuth < 0.0) {
            azimuth += 2.0 * M_PI;
        }
        const double slot = azimuth / step;
        const int k0 = static_cast<int>(std::floor(slot)) % rig.count;
        const int k1 = (k0 + 1) % rig.count;
        const double frac = slot - std::floor(slot);
        // Below the knee the second tier takes over, blended over 20% of the thigh either side.
        double lower = 0.0;
        if (rig.tiers > 1) {
            const double h = r.dot(rig.down) / rig.length;
            lower = std::max(0.0, std::min(1.0, (h - 0.8) / 0.4));
        }
        const int t0 = rig.firstJoint, t1 = rig.firstJoint + rig.count;
        out(i, t0 + k0) += mass * (1.0 - lower) * (1.0 - frac);
        out(i, t0 + k1) += mass * (1.0 - lower) * frac;
        if (rig.tiers > 1) {
            out(i, t1 + k0) += mass * lower * (1.0 - frac);
            out(i, t1 + k1) += mass * lower * frac;
        }
    }
    return out;
}

}  // namespace acm::skin
