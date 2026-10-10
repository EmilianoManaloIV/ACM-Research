// weightbench: transfers skin weights from a rigged body to a garment with
// one or more methods, then scores the result over a set of poses.
//
// Usage:
//   weightbench --body body.gltf --garment skirt.obj --poses poses.json [--pose-set test]
//               [--method closest,inpaint|all] [--out results.csv] [--save-weights dir] [--save-posed dir]
//               [--prune 4] [--distance 0.05] [--angle 30] [--no-smooth]
//               [--relative-slack 1.5] [--jump 2] [--margin 2] [--offset-blend 4] [--length-cap 0.5] [--solve-limit 4] [--skinning lbs|dqs]
//               [--refine [--train-set train] [--refine-support 8] [--refine-prior 0.1] [--refine-clearance 0.5]]
//
//   weightbench --source sphere.obj --source-weights w.csv --garment grid.obj
//               --method inpaint --save-weights dir
//     Transfer only, with no skeleton or poses, for checking a method against
//     a reference implementation.
//
// Output: one CSV row per (method, pose) with the pose metrics, plus one
// "weights" row per method with the weight metrics and run time.

#include "acm/skin/gltf.h"
#include "acm/skin/metrics.h"
#include "acm/skin/refine.h"
#include "acm/skin/transfer.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string body;
    std::string source;
    std::string sourceWeights;
    std::string garment;
    std::string poses;
    std::string poseSet = "poses";
    std::vector<std::string> methods;
    std::string out;
    std::string saveWeights;
    std::string savePosed;  // directory for per-pose OBJs and the matched mask, for inspection
    int prune = 0;
    bool refine = false;            // M6: refine the transferred weights on the training poses
    std::string trainSet = "train";
    acm::skin::RefineParams refineParams;
    acm::skin::SkinningMode skinning = acm::skin::SkinningMode::LinearBlend;  // for body and garment alike
    acm::skin::TransferParams params;
};

[[noreturn]] void usage(const char* argv0) {
    std::fprintf(stderr,
                 "usage: %s --body body.gltf --garment garment.(obj|gltf) --poses poses.json\n"
                 "          [--pose-set name] [--method a,b|all] [--out results.csv] [--save-weights dir] [--save-posed dir]\n"
                 "          [--prune N] [--distance 0.05] [--angle 30] [--no-smooth] [--relative-slack 1.5] [--jump 2] [--margin 2] [--offset-blend 4] [--length-cap 0.5] [--solve-limit 4] [--skinning lbs|dqs]\n"
                 "          [--refine [--train-set train] [--refine-support 8] [--refine-prior 0.1] [--refine-clearance 0.5]]\n"
                 "   or: %s --source mesh.obj --source-weights w.csv --garment mesh.obj --method m --save-weights dir\n",
                 argv0, argv0);
    std::exit(2);
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, sep)) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

Options parseArgs(int argc, char** argv) {
    Options o;
    auto value = [&](int& i) -> const char* {
        if (i + 1 >= argc) {
            usage(argv[0]);
        }
        return argv[++i];
    };
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (!std::strcmp(a, "--body")) o.body = value(i);
        else if (!std::strcmp(a, "--source")) o.source = value(i);
        else if (!std::strcmp(a, "--source-weights")) o.sourceWeights = value(i);
        else if (!std::strcmp(a, "--garment")) o.garment = value(i);
        else if (!std::strcmp(a, "--poses")) o.poses = value(i);
        else if (!std::strcmp(a, "--pose-set")) o.poseSet = value(i);
        else if (!std::strcmp(a, "--method")) o.methods = split(value(i), ',');
        else if (!std::strcmp(a, "--out")) o.out = value(i);
        else if (!std::strcmp(a, "--save-weights")) o.saveWeights = value(i);
        else if (!std::strcmp(a, "--save-posed")) o.savePosed = value(i);
        else if (!std::strcmp(a, "--prune")) o.prune = std::atoi(value(i));
        else if (!std::strcmp(a, "--distance")) o.params.distanceFraction = std::atof(value(i));
        else if (!std::strcmp(a, "--angle")) o.params.angleDegrees = std::atof(value(i));
        else if (!std::strcmp(a, "--no-smooth")) o.params.smooth = false;
        else if (!std::strcmp(a, "--relative-slack")) o.params.relativeSlack = std::atof(value(i));
        else if (!std::strcmp(a, "--jump")) o.params.consistencyJump = std::atof(value(i));
        else if (!std::strcmp(a, "--margin")) o.params.consistencyMargin = std::atof(value(i));
        else if (!std::strcmp(a, "--offset-blend")) o.params.offsetBlend = std::atof(value(i));
        else if (!std::strcmp(a, "--length-cap")) o.params.lengthCap = std::atof(value(i));
        else if (!std::strcmp(a, "--kin-steps")) o.params.blendKinSteps = std::atoi(value(i));
        else if (!std::strcmp(a, "--solve-limit")) o.params.solveInfluenceLimit = std::atoi(value(i));
        else if (!std::strcmp(a, "--refine")) o.refine = true;
        else if (!std::strcmp(a, "--train-set")) o.trainSet = value(i);
        else if (!std::strcmp(a, "--refine-support")) o.refineParams.support = std::atoi(value(i));
        else if (!std::strcmp(a, "--refine-prior")) o.refineParams.prior = std::atof(value(i));
        else if (!std::strcmp(a, "--refine-clearance")) o.refineParams.clearanceShare = std::atof(value(i));
        else if (!std::strcmp(a, "--refine-iterations")) o.refineParams.relaxIterations = std::atoi(value(i));
        else if (!std::strcmp(a, "--skinning")) {
            if (!acm::skin::parseSkinningMode(value(i), o.skinning)) usage(argv[0]);
        }
        else usage(argv[0]);
    }
    if (o.garment.empty() || (o.body.empty() && o.source.empty())) {
        usage(argv[0]);
    }
    if (o.methods.empty() || (o.methods.size() == 1 && o.methods[0] == "all")) {
        o.methods = acm::skin::transferMethodNames();
    }
    return o;
}

void writeOBJ(const std::string& path, const Eigen::MatrixXd& V, const Eigen::MatrixXi& F) {
    std::ofstream out(path);
    out.precision(9);
    for (int i = 0; i < V.rows(); ++i) {
        out << "v " << V(i, 0) << ' ' << V(i, 1) << ' ' << V(i, 2) << '\n';
    }
    for (int i = 0; i < F.rows(); ++i) {
        out << "f " << F(i, 0) + 1 << ' ' << F(i, 1) + 1 << ' ' << F(i, 2) + 1 << '\n';
    }
}

std::string baseName(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    const std::string file = slash == std::string::npos ? path : path.substr(slash + 1);
    const size_t dot = file.find_last_of('.');
    return dot == std::string::npos ? file : file.substr(0, dot);
}

}  // namespace

int main(int argc, char** argv) {
    Options options = parseArgs(argc, argv);
    using namespace acm::skin;
    try {
        // Source of the weights: a rigged body, or a bare mesh plus a CSV.
        Rig rig;
        const bool transferOnly = options.body.empty();
        if (transferOnly) {
            rig.body = loadMesh(options.source);
            if (options.sourceWeights.empty() || !loadWeightsCSV(options.sourceWeights, rig.body.W)) {
                std::fprintf(stderr, "--source needs --source-weights with one row per vertex\n");
                return 1;
            }
            if (rig.body.W.rows() != rig.body.V.rows()) {
                std::fprintf(stderr, "source weights have %d rows, mesh has %d vertices\n",
                             static_cast<int>(rig.body.W.rows()), static_cast<int>(rig.body.V.rows()));
                return 1;
            }
        } else {
            rig = loadRig(options.body);
            options.params.boneLength = boneLengths(rig.skeleton);
            options.params.jointParent = Eigen::Map<const Eigen::VectorXi>(rig.skeleton.parent.data(),
                                                                           static_cast<int>(rig.skeleton.parent.size()));
        }
        const Mesh garment = loadMesh(options.garment);
        std::vector<Pose> poses;
        std::vector<Pose> trainPoses;
        if (!transferOnly && !options.poses.empty()) {
            poses = loadPoses(options.poses, rig.skeleton, options.poseSet);
            if (options.refine) {
                trainPoses = loadPoses(options.poses, rig.skeleton, options.trainSet);
            }
        }
        options.refineParams.skinning = options.skinning;
        std::printf("source: %d vertices, %d joints; garment: %d vertices; %d poses; skinning %s\n",
                    static_cast<int>(rig.body.V.rows()), static_cast<int>(rig.body.W.cols()),
                    static_cast<int>(garment.V.rows()), static_cast<int>(poses.size()),
                    skinningModeName(options.skinning));

        std::ofstream csv;
        if (!options.out.empty()) {
            csv.open(options.out);
            if (!csv) {
                std::fprintf(stderr, "Could not write %s\n", options.out.c_str());
                return 1;
            }
            csv << "garment,method,pose,penetration_fraction,penetration_mean_depth,penetration_max_depth,"
                   "stretch_p95,stretch_max,stretch_mean,torn_fraction,offset_drift,matched_fraction,seconds,dirichlet,min_weight,max_weight,"
                   "row_sum_error,max_influences,over_limit_fraction\n";
        }
        const std::string garmentName = baseName(options.garment);

        for (const std::string& baseMethod : options.methods) {
            const std::string method = options.refine ? baseMethod + "+refine" : baseMethod;
            TransferResult result;
            try {
                result = runTransfer(baseMethod, rig.body, garment, options.params);
            } catch (const std::exception& e) {
                // A method that cannot handle a garment is a result worth recording.
                std::printf("%-10s FAILED: %s\n", method.c_str(), e.what());
                if (csv) {
                    csv << garmentName << ',' << method << ",failed,,,,,,,,,,,,,,,,\n";
                }
                continue;
            }
            if (options.prune > 0) {
                result.W = pruneInfluences(result.W, options.prune);
            }
            if (options.refine) {
                const auto start = std::chrono::steady_clock::now();
                const RefineResult refined = refinePoseAware(rig, garment, result.W, trainPoses, options.refineParams);
                result.W = refined.W;
                result.seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                std::printf("%-10s refined on %d poses: targets moved %.3f%% of body, fit residual %.3f%%\n",
                            method.c_str(), static_cast<int>(trainPoses.size()), 100.0 * refined.targetChange,
                            100.0 * refined.fitResidual);
            }
            const double matchedFraction =
                static_cast<double>(result.matched.count()) / static_cast<double>(garment.V.rows());
            const WeightMetrics wm = evaluateWeights(garment, result.W, options.prune > 0 ? options.prune : 4);
            std::printf("%-10s matched %5.1f%%  %.3fs  dirichlet %.4g  weights [%.3f, %.3f]  rowsum err %.1e  "
                        "max influences %d\n",
                        method.c_str(), 100.0 * matchedFraction, result.seconds, wm.dirichlet, wm.minWeight,
                        wm.maxWeight, wm.maxRowSumError, wm.maxInfluences);
            if (csv) {
                csv << garmentName << ',' << method << ",weights,,,,,,,,," << matchedFraction << ',' << result.seconds
                    << ',' << wm.dirichlet << ',' << wm.minWeight << ',' << wm.maxWeight << ',' << wm.maxRowSumError
                    << ',' << wm.maxInfluences << ',' << wm.overLimitFraction << '\n';
            }
            if (!options.savePosed.empty()) {
                std::ofstream mask(options.savePosed + "/" + garmentName + "." + method + ".matched.csv");
                for (int i = 0; i < result.matched.size(); ++i) {
                    mask << (result.matched(i) ? 1 : 0) << '\n';
                }
            }
            if (!options.saveWeights.empty()) {
                const std::string path = options.saveWeights + "/" + garmentName + "." + method + ".csv";
                if (!saveWeightsCSV(path, result.W)) {
                    std::fprintf(stderr, "Could not write %s\n", path.c_str());
                    return 1;
                }
            }

            double worstPenetration = 0.0;
            double worstStretch = 0.0;
            for (const Pose& pose : poses) {
                const std::vector<Eigen::Matrix4d> matrices = skinningMatrices(rig.skeleton, pose);
                const Eigen::MatrixXd bodyPosed = skin(rig.body.V, rig.body.W, matrices, options.skinning);
                const Eigen::MatrixXd garmentPosed = skin(garment.V, result.W, matrices, options.skinning);
                const PoseMetrics pm = evaluatePose(rig.body, bodyPosed, garment, garmentPosed);
                if (!options.savePosed.empty()) {
                    const std::string stem = options.savePosed + "/" + garmentName + "." + method + "." + pose.name;
                    writeOBJ(stem + ".garment.obj", garmentPosed, garment.F);
                    writeOBJ(stem + ".body.obj", bodyPosed, rig.body.F);
                }
                worstPenetration = std::max(worstPenetration, pm.penetrationFraction);
                worstStretch = std::max(worstStretch, pm.stretchP95);
                if (csv) {
                    csv << garmentName << ',' << method << ',' << pose.name << ',' << pm.penetrationFraction << ','
                        << pm.penetrationMeanDepth << ',' << pm.penetrationMaxDepth << ',' << pm.stretchP95 << ','
                        << pm.stretchMax << ',' << pm.stretchMean << ',' << pm.tornFraction << ',' << pm.offsetDrift
                        << ",,,,,,,,\n";
                }
            }
            if (!poses.empty()) {
                std::printf("%-10s worst pose: penetration %.1f%% of vertices, stretch p95 %.3f\n", method.c_str(),
                            100.0 * worstPenetration, worstStretch);
            }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    return 0;
}
