#pragma once

#include "acm/skin/rig.h"

#include <string>
#include <vector>

namespace acm::skin {

// Loads a rigged character from a .gltf/.glb file: the first skinned mesh and
// the skin it is bound to. Throws std::runtime_error with a message on failure.
Rig loadRig(const std::string& path);

// Loads a mesh from .obj, .gltf or .glb (the first mesh in the file). A skinned
// glTF mesh keeps its weights; anything else comes back unskinned.
Mesh loadMesh(const std::string& path);

// Poses from a JSON file:
//   {"poses": [{"name": "elbow90", "joints": {"elbow": {"axis": [0,0,1], "deg": 90}}}]}
// A joint may also be given as a quaternion [x, y, z, w]. Joints not named stay
// at rest. `key` selects which top-level list to read, so one file can hold
// "train" and "test" sets. Joint names must exist in the skeleton.
std::vector<Pose> loadPoses(const std::string& path, const Skeleton& skeleton,
                            const std::string& key = "poses");

}  // namespace acm::skin
