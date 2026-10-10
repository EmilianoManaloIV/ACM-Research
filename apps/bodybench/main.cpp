// bodybench: computes skin weights for a rigged body from its skeleton alone
// (RQ-002), then scores each method's weights over a set of poses.
//
// Usage:
//   bodybench --body body.gltf --poses poses.json [--pose-set test,named]
//             [--method floor,bbw,artist,csv:bone_heat=weights.csv] [--character name]
//             [--out results.csv] [--save-weights dir] [--export-bones bones.json] [--export-mesh body.obj]
//             [--skinning lbs|dqs] [--bbw-samples 10] [--artist-weights artist.csv]
//
//   floor   B0, inverse squared distance to each bone
//   bbw     B2, bounded biharmonic weights (libigl, TetGen)
//   artist  the weights shipped in the glTF, scored as a reference row;
//           --artist-weights replaces them with a CSV, for references with
//           more than the four influences per vertex glTF holds
//   csv:NAME=PATH   weights computed elsewhere (B1, Blender bone heat), one
//                   row per vertex and one column per joint, in joint order
//
// --export-bones and --export-mesh write the bone segments every method uses
// and the body as OBJ (vertices in glTF order), for the Blender script, and
// exit if no method is given.
//
// Output: one "weights" row per method (validity, unweighted vertices, run
// time, agreement with the artist weights when the glTF has them) and one
// row per (method, pose) with stretch, torn edges, self-intersection above
// rest, and distance from the artist-weighted pose. A method whose weights
// are invalid gets no pose rows: the protocol records it as a failure.

#include "acm/skin/autoweight.h"
#include "acm/skin/gltf.h"
#include "acm/skin/metrics.h"

#include <igl/bounding_box_diagonal.h>

#include <chrono>
#include <cmath>
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
    std::string poses;
    std::vector<std::string> poseSets = {"test"};
    std::vector<std::string> methods;
    std::string character;
    std::string out;
    std::string saveWeights;
    std::string exportBones;
    std::string exportMesh;
    std::string artistWeights;
    int bbwSamples = 10;
    acm::skin::SkinningMode skinning = acm::skin::SkinningMode::LinearBlend;
};

[[noreturn]] void usage(const char* argv0) {
    std::fprintf(stderr,
                 "usage: %s --body body.gltf --poses poses.json [--pose-set test,named]\n"
                 "          [--method floor,bbw,artist,csv:name=path] [--character name] [--out results.csv]\n"
                 "          [--save-weights dir] [--export-bones bones.json] [--export-mesh body.obj] [--skinning lbs|dqs] [--bbw-samples 10]\n"
                 "          [--artist-weights artist.csv]\n",
                 argv0);
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
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        const auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                usage(argv[0]);
            }
            return argv[++i];
        };
        if (a == "--body") o.body = next();
        else if (a == "--poses") o.poses = next();
        else if (a == "--pose-set") o.poseSets = split(next(), ',');
        else if (a == "--method") o.methods = split(next(), ',');
        else if (a == "--character") o.character = next();
        else if (a == "--out") o.out = next();
        else if (a == "--save-weights") o.saveWeights = next();
        else if (a == "--export-bones") o.exportBones = next();
        else if (a == "--export-mesh") o.exportMesh = next();
        else if (a == "--artist-weights") o.artistWeights = next();
        else if (a == "--bbw-samples") o.bbwSamples = std::atoi(next().c_str());
        else if (a == "--skinning") {
            if (!acm::skin::parseSkinningMode(next(), o.skinning)) usage(argv[0]);
        } else usage(argv[0]);
    }
    if (o.body.empty() || (o.methods.empty() && o.exportBones.empty() && o.exportMesh.empty())) {
        usage(argv[0]);
    }
    if (!o.methods.empty() && o.poses.empty()) {
        usage(argv[0]);
    }
    if (o.character.empty()) {
        const size_t slash = o.body.find_last_of('/');
        const std::string dir = slash == std::string::npos ? "." : o.body.substr(0, slash);
        const size_t s2 = dir.find_last_of('/');
        o.character = s2 == std::string::npos ? dir : dir.substr(s2 + 1);
    }
    return o;
}

std::string fmt(double v) {
    if (std::isnan(v)) return "";
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.6g", v);
    return buf;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace acm::skin;
    const Options options = parseArgs(argc, argv);
    try {
        Rig rig = loadRig(options.body);
        if (!options.artistWeights.empty()) {
            Eigen::MatrixXd W;
            if (!loadWeightsCSV(options.artistWeights, W) || W.rows() != rig.body.V.rows() ||
                W.cols() != rig.skeleton.size()) {
                throw std::runtime_error(options.artistWeights + " does not match the body's vertices and joints");
            }
            normaliseRows(W);
            rig.body.W = W;
        }
        const int joints = rig.skeleton.size();
        const std::vector<BoneSegment> bones = boneSegments(rig.skeleton);
        if (!options.exportBones.empty()) {
            if (!saveBoneSegments(options.exportBones, rig.skeleton, bones)) {
                throw std::runtime_error("cannot write " + options.exportBones);
            }
            std::printf("wrote %s (%d bones)\n", options.exportBones.c_str(), joints);
        }
        if (!options.exportMesh.empty()) {
            std::ofstream obj(options.exportMesh);
            obj.precision(9);
            for (int i = 0; i < rig.body.V.rows(); ++i) {
                obj << "v " << rig.body.V(i, 0) << " " << rig.body.V(i, 1) << " " << rig.body.V(i, 2) << "\n";
            }
            for (int f = 0; f < rig.body.F.rows(); ++f) {
                obj << "f " << rig.body.F(f, 0) + 1 << " " << rig.body.F(f, 1) + 1 << " " << rig.body.F(f, 2) + 1 << "\n";
            }
            if (!obj) {
                throw std::runtime_error("cannot write " + options.exportMesh);
            }
            std::printf("wrote %s\n", options.exportMesh.c_str());
        }
        if (options.methods.empty()) {
            return 0;
        }

        std::vector<Pose> poses;
        for (const std::string& set : options.poseSets) {
            for (Pose& p : loadPoses(options.poses, rig.skeleton, set)) {
                p.name = set + "/" + p.name;
                poses.push_back(std::move(p));
            }
        }
        const bool hasArtist = rig.body.isSkinned();
        const double scale = igl::bounding_box_diagonal(rig.body.V);
        const double restSelf = selfIntersectionFraction(rig.body.V, rig.body.F);
        std::printf("%s: %d vertices, %d faces, %d joints, %zu poses, artist weights: %s, rest self-intersection %.4f\n",
                    options.character.c_str(), static_cast<int>(rig.body.V.rows()), static_cast<int>(rig.body.F.rows()),
                    joints, poses.size(), hasArtist ? "yes" : "no", restSelf);

        // Artist-weighted pose of every pose, for the reference distance.
        std::vector<Eigen::MatrixXd> artistPosed;
        if (hasArtist) {
            for (const Pose& pose : poses) {
                artistPosed.push_back(skin(rig.body.V, rig.body.W, skinningMatrices(rig.skeleton, pose), options.skinning));
            }
        }

        std::ofstream csv;
        if (!options.out.empty()) {
            csv.open(options.out);
            csv << "character,method,pose,skinning,valid,unweighted,stretch_p95,stretch_mean,stretch_max,torn,"
                   "self_intersection,l1,precision,recall,artist_pose_distance,seconds,note\n";
        }
        const auto row = [&](const std::string& method, const std::string& pose, const std::vector<std::string>& cols) {
            if (!csv.is_open()) return;
            csv << options.character << "," << method << "," << pose << "," << skinningModeName(options.skinning);
            for (const std::string& c : cols) csv << "," << c;
            csv << "\n";
        };

        for (const std::string& spec : options.methods) {
            std::string method = spec;
            AutoWeightResult result;
            const auto start = std::chrono::steady_clock::now();
            if (spec == "floor") {
                result.W = inverseDistanceWeights(rig.body.V, bones, joints);
            } else if (spec == "bbw") {
                result = boundedBiharmonicWeights(rig.body, bones, joints, options.bbwSamples);
            } else if (spec == "artist") {
                if (!hasArtist) {
                    std::fprintf(stderr, "skipping artist: %s has no weights\n", options.body.c_str());
                    continue;
                }
                result.W = rig.body.W;
            } else if (spec.rfind("csv:", 0) == 0) {
                const size_t eq = spec.find('=');
                if (eq == std::string::npos) usage(argv[0]);
                method = spec.substr(4, eq - 4);
                if (!loadWeightsCSV(spec.substr(eq + 1), result.W)) {
                    result.note = "cannot read " + spec.substr(eq + 1);
                    result.W.resize(0, 0);
                } else if (result.W.rows() != rig.body.V.rows() || result.W.cols() != joints) {
                    result.note = "weights are " + std::to_string(result.W.rows()) + " by " +
                                  std::to_string(result.W.cols()) + ", expected " +
                                  std::to_string(rig.body.V.rows()) + " by " + std::to_string(joints);
                    result.W.resize(0, 0);
                } else {
                    normaliseRows(result.W);
                }
            } else {
                usage(argv[0]);
            }
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

            int unweighted = 0;
            const bool valid = validWeights(result.W, unweighted);
            WeightAgreement agree;
            agree.l1 = agree.precision = agree.recall = std::nan("");
            if (hasArtist && result.W.rows() == rig.body.W.rows() && result.W.cols() == rig.body.W.cols()) {
                agree = compareWeights(result.W, rig.body.W);
            }
            std::printf("%-10s valid %d  unweighted %d  %.2fs  l1 %s  %s\n", method.c_str(), valid ? 1 : 0, unweighted,
                        seconds, fmt(agree.l1).c_str(), result.note.c_str());
            row(method, "weights",
                {valid ? "1" : "0", std::to_string(unweighted), "", "", "", "", "", fmt(agree.l1), fmt(agree.precision),
                 fmt(agree.recall), "", fmt(seconds), "\"" + result.note + "\""});
            if (!options.saveWeights.empty() && result.W.size() > 0) {
                saveWeightsCSV(options.saveWeights + "/" + options.character + "." + method + ".csv", result.W);
            }
            if (!valid) {
                continue;
            }
            for (size_t k = 0; k < poses.size(); ++k) {
                const Eigen::MatrixXd posed =
                    skin(rig.body.V, result.W, skinningMatrices(rig.skeleton, poses[k]), options.skinning);
                PoseMetrics m;
                stretchMetrics(rig.body.F, rig.body.V, posed, m);
                const double self = selfIntersectionFraction(posed, rig.body.F) - restSelf;
                double dist = std::nan("");
                if (hasArtist) {
                    dist = (posed - artistPosed[k]).rowwise().norm().mean() / scale;
                }
                row(method, poses[k].name,
                    {"1", std::to_string(unweighted), fmt(m.stretchP95), fmt(m.stretchMean), fmt(m.stretchMax),
                     fmt(m.tornFraction), fmt(self), "", "", "", fmt(dist), "", ""});
            }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
