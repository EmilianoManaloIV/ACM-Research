#include "acm/skin/gltf.h"
#include "acm/skin/hem.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include <igl/read_triangle_mesh.h>

#include <cmath>
#include <fstream>
#include <functional>
#include <stdexcept>

namespace acm::skin {

namespace {

using json = nlohmann::json;

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

tinygltf::Model readModel(const std::string& path) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;
    const bool ok = endsWith(path, ".glb") ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
                                            : loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!ok) {
        throw std::runtime_error("Could not read " + path + ": " + err);
    }
    return model;
}

// Reads accessor `index` as a dense double matrix with one row per element,
// converting integer and normalized types the way glTF specifies.
Eigen::MatrixXd readAccessor(const tinygltf::Model& model, int index) {
    const tinygltf::Accessor& accessor = model.accessors.at(index);
    const tinygltf::BufferView& view = model.bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer& buffer = model.buffers.at(view.buffer);
    const int components = tinygltf::GetNumComponentsInType(accessor.type);
    const int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    const size_t stride = view.byteStride ? view.byteStride : static_cast<size_t>(components * componentSize);
    const unsigned char* base = buffer.data.data() + view.byteOffset + accessor.byteOffset;

    Eigen::MatrixXd out(accessor.count, components);
    for (size_t i = 0; i < accessor.count; ++i) {
        const unsigned char* element = base + i * stride;
        for (int c = 0; c < components; ++c) {
            const unsigned char* p = element + c * componentSize;
            double value = 0.0;
            switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT: { float f; std::memcpy(&f, p, 4); value = f; break; }
                case TINYGLTF_COMPONENT_TYPE_DOUBLE: { std::memcpy(&value, p, 8); break; }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: { value = *p; if (accessor.normalized) value /= 255.0; break; }
                case TINYGLTF_COMPONENT_TYPE_BYTE: { signed char v; std::memcpy(&v, p, 1); value = v; if (accessor.normalized) value = std::max(value / 127.0, -1.0); break; }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: { unsigned short v; std::memcpy(&v, p, 2); value = v; if (accessor.normalized) value /= 65535.0; break; }
                case TINYGLTF_COMPONENT_TYPE_SHORT: { short v; std::memcpy(&v, p, 2); value = v; if (accessor.normalized) value = std::max(value / 32767.0, -1.0); break; }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: { unsigned int v; std::memcpy(&v, p, 4); value = v; break; }
                case TINYGLTF_COMPONENT_TYPE_INT: { int v; std::memcpy(&v, p, 4); value = v; break; }
                default: throw std::runtime_error("Unsupported accessor component type");
            }
            out(static_cast<int>(i), c) = value;
        }
    }
    return out;
}

Eigen::Matrix4d nodeLocalMatrix(const tinygltf::Node& node) {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    if (node.matrix.size() == 16) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                m(r, c) = node.matrix[c * 4 + r];  // column-major in the file
            }
        }
        return m;
    }
    Eigen::Affine3d t = Eigen::Affine3d::Identity();
    if (node.translation.size() == 3) {
        t.translate(Eigen::Vector3d(node.translation[0], node.translation[1], node.translation[2]));
    }
    if (node.rotation.size() == 4) {
        t.rotate(Eigen::Quaterniond(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]));
    }
    if (node.scale.size() == 3) {
        t.scale(Eigen::Vector3d(node.scale[0], node.scale[1], node.scale[2]));
    }
    return t.matrix();
}

std::vector<int> nodeParents(const tinygltf::Model& model) {
    std::vector<int> parent(model.nodes.size(), -1);
    for (size_t n = 0; n < model.nodes.size(); ++n) {
        for (int child : model.nodes[n].children) {
            parent[child] = static_cast<int>(n);
        }
    }
    return parent;
}

Eigen::Matrix4d nodeGlobalMatrix(const tinygltf::Model& model, const std::vector<int>& parent, int node) {
    Eigen::Matrix4d m = nodeLocalMatrix(model.nodes[node]);
    for (int p = parent[node]; p >= 0; p = parent[p]) {
        m = nodeLocalMatrix(model.nodes[p]) * m;
    }
    return m;
}

// Appends every primitive of `mesh` to V/F (and joints/weights when skinned).
void appendMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, Eigen::MatrixXd& V, Eigen::MatrixXi& F,
                Eigen::MatrixXd& J, Eigen::MatrixXd& Wraw) {
    for (const tinygltf::Primitive& prim : mesh.primitives) {
        if (prim.mode != TINYGLTF_MODE_TRIANGLES && prim.mode != -1) {
            continue;
        }
        const auto pos = prim.attributes.find("POSITION");
        if (pos == prim.attributes.end()) {
            continue;
        }
        const Eigen::MatrixXd P = readAccessor(model, pos->second);
        Eigen::MatrixXi faces;
        if (prim.indices >= 0) {
            const Eigen::MatrixXd idx = readAccessor(model, prim.indices);
            faces.resize(idx.rows() / 3, 3);
            for (int i = 0; i < faces.rows(); ++i) {
                faces.row(i) << static_cast<int>(idx(3 * i, 0)), static_cast<int>(idx(3 * i + 1, 0)),
                    static_cast<int>(idx(3 * i + 2, 0));
            }
        } else {
            faces.resize(P.rows() / 3, 3);
            for (int i = 0; i < faces.rows(); ++i) {
                faces.row(i) << 3 * i, 3 * i + 1, 3 * i + 2;
            }
        }
        const int offset = static_cast<int>(V.rows());
        V.conservativeResize(offset + P.rows(), 3);
        V.bottomRows(P.rows()) = P.leftCols(3);
        F.conservativeResize(F.rows() + faces.rows(), 3);
        F.bottomRows(faces.rows()) = faces.array() + offset;

        const auto joints = prim.attributes.find("JOINTS_0");
        const auto weights = prim.attributes.find("WEIGHTS_0");
        if (joints != prim.attributes.end() && weights != prim.attributes.end()) {
            const Eigen::MatrixXd j = readAccessor(model, joints->second);
            const Eigen::MatrixXd w = readAccessor(model, weights->second);
            J.conservativeResize(offset + j.rows(), 4);
            J.bottomRows(j.rows()) = j.leftCols(4);
            Wraw.conservativeResize(offset + w.rows(), 4);
            Wraw.bottomRows(w.rows()) = w.leftCols(4);
        }
    }
}

}  // namespace

Rig loadRig(const std::string& path) {
    const tinygltf::Model model = readModel(path);
    const std::vector<int> parent = nodeParents(model);

    int meshNode = -1;
    for (size_t n = 0; n < model.nodes.size(); ++n) {
        if (model.nodes[n].mesh >= 0 && model.nodes[n].skin >= 0) {
            meshNode = static_cast<int>(n);
            break;
        }
    }
    if (meshNode < 0) {
        throw std::runtime_error(path + " has no skinned mesh");
    }
    const tinygltf::Skin& skin = model.skins[model.nodes[meshNode].skin];

    Rig rig;
    Skeleton& skeleton = rig.skeleton;
    const int joints = static_cast<int>(skin.joints.size());
    std::vector<int> jointOfNode(model.nodes.size(), -1);
    for (int j = 0; j < joints; ++j) {
        jointOfNode[skin.joints[j]] = j;
    }
    Eigen::MatrixXd ibm;
    if (skin.inverseBindMatrices >= 0) {
        ibm = readAccessor(model, skin.inverseBindMatrices);
    }
    skeleton.names.resize(joints);
    skeleton.parent.resize(joints);
    skeleton.restLocal.resize(joints);
    skeleton.inverseBind.resize(joints);
    for (int j = 0; j < joints; ++j) {
        const int node = skin.joints[j];
        skeleton.names[j] = model.nodes[node].name.empty() ? "joint" + std::to_string(j) : model.nodes[node].name;
        // Walk up until a parent that is itself a joint; anything above the root
        // joint (an armature node, say) is folded into the root's rest transform.
        int p = parent[node];
        while (p >= 0 && jointOfNode[p] < 0) {
            p = parent[p];
        }
        skeleton.parent[j] = p >= 0 ? jointOfNode[p] : -1;
        skeleton.restLocal[j] = p >= 0 ? nodeLocalMatrix(model.nodes[node]) : nodeGlobalMatrix(model, parent, node);
        if (ibm.rows() == joints) {
            Eigen::Matrix4d m;
            for (int c = 0; c < 4; ++c) {
                for (int r = 0; r < 4; ++r) {
                    m(r, c) = ibm(j, c * 4 + r);
                }
            }
            skeleton.inverseBind[j] = m;
        } else {
            skeleton.inverseBind[j] = Eigen::Matrix4d::Identity();
        }
    }

    Eigen::MatrixXd J;
    Eigen::MatrixXd Wraw;
    Mesh& body = rig.body;
    appendMesh(model, model.meshes[model.nodes[meshNode].mesh], body.V, body.F, J, Wraw);
    if (J.rows() != body.V.rows()) {
        throw std::runtime_error(path + ": skinned mesh is missing JOINTS_0/WEIGHTS_0");
    }
    body.W = Eigen::MatrixXd::Zero(body.V.rows(), joints);
    for (int i = 0; i < body.V.rows(); ++i) {
        for (int k = 0; k < 4; ++k) {
            const int j = static_cast<int>(J(i, k));
            if (Wraw(i, k) != 0.0 && j >= 0 && j < joints) {
                body.W(i, j) += Wraw(i, k);
            }
        }
    }
    body.computeNormals();
    return rig;
}

Mesh loadMesh(const std::string& path) {
    Mesh mesh;
    if (endsWith(path, ".gltf") || endsWith(path, ".glb")) {
        const tinygltf::Model model = readModel(path);
        const std::vector<int> parent = nodeParents(model);
        int meshNode = -1;
        for (size_t n = 0; n < model.nodes.size() && meshNode < 0; ++n) {
            if (model.nodes[n].mesh >= 0) {
                meshNode = static_cast<int>(n);
            }
        }
        if (meshNode < 0) {
            throw std::runtime_error(path + " has no mesh");
        }
        Eigen::MatrixXd J;
        Eigen::MatrixXd Wraw;
        appendMesh(model, model.meshes[model.nodes[meshNode].mesh], mesh.V, mesh.F, J, Wraw);
        if (model.nodes[meshNode].skin >= 0 && J.rows() == mesh.V.rows()) {
            const int joints = static_cast<int>(model.skins[model.nodes[meshNode].skin].joints.size());
            mesh.W = Eigen::MatrixXd::Zero(mesh.V.rows(), joints);
            for (int i = 0; i < mesh.V.rows(); ++i) {
                for (int k = 0; k < 4; ++k) {
                    const int j = static_cast<int>(J(i, k));
                    if (Wraw(i, k) != 0.0 && j >= 0 && j < joints) {
                        mesh.W(i, j) += Wraw(i, k);
                    }
                }
            }
        } else {
            // An unskinned mesh is placed by its node, so bake that transform in.
            const Eigen::Matrix4d m = nodeGlobalMatrix(model, parent, meshNode);
            for (int i = 0; i < mesh.V.rows(); ++i) {
                const Eigen::Vector4d p = m * Eigen::Vector4d(mesh.V(i, 0), mesh.V(i, 1), mesh.V(i, 2), 1.0);
                mesh.V.row(i) = p.head<3>().transpose();
            }
        }
    } else {
        Eigen::MatrixXd V;
        Eigen::MatrixXi F;
        if (!igl::read_triangle_mesh(path, V, F)) {
            throw std::runtime_error("Could not read " + path);
        }
        mesh.V = V;
        mesh.F = F;
    }
    if (mesh.F.rows() == 0) {
        throw std::runtime_error(path + " has no triangles");
    }
    mesh.computeNormals();
    return mesh;
}

std::vector<Pose> loadPoses(const std::string& path, const Skeleton& skeleton, const std::string& key) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not read " + path);
    }
    json doc;
    in >> doc;
    if (!doc.contains(key) || !doc[key].is_array()) {
        throw std::runtime_error(path + " has no \"" + key + "\" list");
    }
    std::vector<Pose> poses;
    for (const json& item : doc[key]) {
        Pose pose = identityPose(skeleton, item.value("name", "pose" + std::to_string(poses.size())));
        if (item.contains("joints")) {
            for (auto it = item["joints"].begin(); it != item["joints"].end(); ++it) {
                const int j = skeleton.find(it.key());
                if (j < 0) {
                    throw std::runtime_error(path + ": pose " + pose.name + " names unknown joint " + it.key());
                }
                const json& r = it.value();
                if (r.is_array() && r.size() == 4) {
                    pose.rotation[j] = Eigen::Quaterniond(r[3].get<double>(), r[0].get<double>(), r[1].get<double>(),
                                                          r[2].get<double>());
                } else if (r.is_object() && r.contains("axis") && r.contains("deg")) {
                    const Eigen::Vector3d axis(r["axis"][0].get<double>(), r["axis"][1].get<double>(),
                                               r["axis"][2].get<double>());
                    pose.rotation[j] = Eigen::Quaterniond(
                        Eigen::AngleAxisd(r["deg"].get<double>() * M_PI / 180.0, axis.normalized()));
                } else {
                    throw std::runtime_error(path + ": joint " + it.key() + " needs [x,y,z,w] or {axis, deg}");
                }
            }
        }
        poses.push_back(pose);
    }
    return poses;
}

HemRig loadHemRig(const std::string& path, const Skeleton& skeleton) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not read " + path);
    }
    json doc;
    in >> doc;
    HemRig rig;
    rig.count = doc.value("count", 8);
    rig.tiers = doc.value("tiers", 2);
    rig.applyToMatched = doc.value("apply", std::string("unmatched")) == "all";
    rig.follow = doc.value("follow", 1.0);
    rig.falloff = doc.value("falloff", 1.0);
    rig.pivotOnHips = doc.value("pivot", std::string("mid")) == "hips";
    rig.lowerHangs = doc.value("lower", std::string("shank")) == "gravity";
    rig.sideDrive = doc.value("drive", std::string("azimuth")) == "side";
    rig.waist = doc.value("waist", -1.0);
    rig.contact = doc.value("contact", false);
    rig.contactClearance = doc.value("clearance", 0.0);
    auto joint = [&](const std::string& name) {
        const int j = skeleton.find(name);
        if (j < 0) {
            throw std::runtime_error(path + " names unknown joint " + name);
        }
        return j;
    };
    if (doc.contains("parent")) {
        rig.parentJoint = joint(doc["parent"].get<std::string>());
    }
    for (const json& leg : doc["legs"]) {
        if (!leg.is_array() || leg.size() != 3) {
            throw std::runtime_error(path + ": each leg needs [hip, knee, ankle]");
        }
        rig.legs.push_back({joint(leg[0].get<std::string>()), joint(leg[1].get<std::string>()),
                            joint(leg[2].get<std::string>())});
    }
    if (doc.contains("move")) {
        for (const json& name : doc["move"]) {
            rig.moveFrom.push_back(joint(name.get<std::string>()));
        }
    }
    return rig;
}

}  // namespace acm::skin
