"""B1 for RQ-002: Blender's bone heat ("With Automatic Weights") on a body.

Run headless:
  blender -b --factory-startup --python tools/blender_bone_heat.py -- body.obj bones.json weights.csv

body.obj    the body mesh, written by bodybench --export-mesh, vertices in the
            same order as the glTF
bones.json  the bone segments every method uses (bodybench --export-bones)
weights.csv one row per vertex, one column per joint in bones.json order;
            vertices Blender leaves unweighted come out as zero rows, which
            bodybench counts as a failure

The log line gives the time of the weighting call alone, in seconds.

The armature gets one bone per joint, from its head to its tail as given, all
deforming, parented as in bones.json. Nothing else is configured: this is the
tool's default behaviour, which is what the protocol measures.
"""

import json
import sys
import time

import bpy


def main(argv):
    mesh_path, bones_path, out_path = argv
    bpy.ops.wm.read_factory_settings(use_empty=True)

    # Read the OBJ by hand so Blender cannot reorder, merge or split vertices.
    verts, faces = [], []
    with open(mesh_path) as f:
        for line in f:
            if line.startswith("v "):
                verts.append(tuple(float(x) for x in line.split()[1:4]))
            elif line.startswith("f "):
                faces.append(tuple(int(x.split("/")[0]) - 1 for x in line.split()[1:]))
    mesh = bpy.data.meshes.new("body")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    body = bpy.data.objects.new("body", mesh)
    bpy.context.scene.collection.objects.link(body)

    with open(bones_path) as f:
        spec = json.load(f)["bones"]
    arm_data = bpy.data.armatures.new("rig")
    arm = bpy.data.objects.new("rig", arm_data)
    bpy.context.scene.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode="EDIT")
    edit = {}
    for b in spec:
        bone = arm_data.edit_bones.new(b["name"])
        bone.head = b["head"]
        tail = b["tail"]
        if sum((h - t) ** 2 for h, t in zip(b["head"], tail)) < 1e-12:
            # Blender drops zero-length bones; give a point joint a tiny length.
            tail = (tail[0], tail[1] + 1e-4, tail[2])
        bone.tail = tail
        bone.use_deform = True
        edit[b["name"]] = bone
    for b in spec:
        if b["parent"] is not None:
            edit[b["name"]].parent = edit[b["parent"]]
            edit[b["name"]].use_connect = False
    bpy.ops.object.mode_set(mode="OBJECT")
    if len(arm_data.bones) != len(spec):
        sys.exit(f"armature has {len(arm_data.bones)} bones, expected {len(spec)} (duplicate names?)")

    bpy.ops.object.select_all(action="DESELECT")
    body.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    start = time.perf_counter()
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    seconds = time.perf_counter() - start

    names = [b["name"] for b in spec]
    column = {}
    for g in body.vertex_groups:
        column[g.index] = names.index(g.name) if g.name in names else None
    rows = [[0.0] * len(names) for _ in verts]
    for v in mesh.vertices:
        for g in v.groups:
            c = column.get(g.group)
            if c is not None:
                rows[v.index][c] = g.weight
    with open(out_path, "w") as f:
        for r in rows:
            f.write(",".join(f"{w:.9g}" for w in r) + "\n")
    unweighted = sum(1 for r in rows if not any(r))
    print(f"bone heat: {len(verts)} vertices, {len(names)} bones, {unweighted} unweighted; "
          f"{seconds:.3f} seconds; wrote {out_path}")


if __name__ == "__main__":
    main(sys.argv[sys.argv.index("--") + 1:])
