#!/usr/bin/env python3
"""Checks weightbench's `inpaint` method against Epic's reference Python.

Runs the algorithm from github.com/rin-23/RobustSkinWeightsTransferCode (MIT,
Abdrashitov, Raichstat, Monsen and Hill 2023) on their sphere-to-plane example
with the libigl Python bindings, then runs the C++ port on the same inputs and
reports the largest difference. The reference functions below are copied from
that repository's src/utils.py with only cosmetic changes.

    python3 tools/verify_inpaint.py build/apps/weightbench/weightbench

Needs: pip install libigl numpy scipy
"""

import math
import os
import subprocess
import sys
import tempfile

import igl
import numpy as np
import scipy.sparse as sp

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MESHES = os.path.join(ROOT, "tests", "data", "epic")


# --- reference implementation (from Epic's utils.py) ------------------------

def find_closest_point_on_surface(P, V, F):
    sqrD, I, C = igl.point_mesh_squared_distance(P, V, F)
    Fc = F[I, :]
    B = igl.barycentric_coordinates(C, V[Fc[:, 0], :], V[Fc[:, 1], :], V[Fc[:, 2], :])
    return sqrD, I, C, B


def interpolate_attribute_from_bary(A, B, I, F):
    Fc = F[I, :]
    return A[Fc[:, 0], :] * B[:, [0]] + A[Fc[:, 1], :] * B[:, [1]] + A[Fc[:, 2], :] * B[:, [2]]


def find_matches_closest_surface(V1, F1, N1, V2, F2, N2, W1, dist_sqrd, angle_deg):
    matched = np.zeros(V2.shape[0], dtype=bool)
    sqrD, I, C, B = find_closest_point_on_surface(V2, V1, F1)
    W2 = interpolate_attribute_from_bary(W1, B, I, F1)
    N1i = interpolate_attribute_from_bary(N1, B, I, F1)
    for i in range(V2.shape[0]):
        n1 = N1i[i] / np.linalg.norm(N1i[i])
        n2 = N2[i] / np.linalg.norm(N2[i])
        deg = math.degrees(np.arccos(np.clip(np.dot(n1, n2), -1.0, 1.0)))
        if sqrD[i] <= dist_sqrd and deg <= angle_deg:
            matched[i] = True
    return matched, W2


def inpaint(V2, F2, W2, matched):
    L = 2 * igl.cotmatrix(V2, F2)
    M = igl.massmatrix(V2, F2, igl.MASSMATRIX_TYPE_VORONOI)
    Minv = sp.diags(1 / M.diagonal())
    Q = -L + L * Minv * L
    Aeq = sp.csc_matrix((0, V2.shape[0]))
    B = np.zeros((L.shape[0], W2.shape[1]))
    b = np.arange(V2.shape[0])[matched].astype(np.int64)
    bc = W2[matched, :]
    Beq = np.zeros((0, W2.shape[1]))
    W = igl.min_quad_with_fixed(Q, B, b, bc, Aeq, Beq, True)  # libigl 2.6 returns Z directly
    return W, True


def smooth(V2, F2, W2, matched, dist, steps=10, alpha=0.2):
    not_matched = ~matched
    to_smooth = np.array(not_matched, copy=True)
    adj = igl.adjacency_list(F2)
    for vid in range(V2.shape[0]):
        if not not_matched[vid]:
            continue
        queue = [vid]
        while queue:
            vv = queue.pop()
            for nn in adj[vv]:
                if not to_smooth[nn] and np.linalg.norm(V2[vid] - V2[nn]) < dist:
                    to_smooth[nn] = True
                    if nn not in queue:
                        queue.append(nn)
    W = np.array(W2, copy=True)
    for _ in range(steps):
        for i in range(V2.shape[0]):
            if to_smooth[i]:
                neigh = adj[i]
                new = (1 - alpha) * W[i]
                for n in neigh:
                    new = new + (W[n] / len(neigh)) * alpha
                W[i] = new
    return W, to_smooth


# --- comparison --------------------------------------------------------------

def load(name):
    V, F = igl.read_triangle_mesh(os.path.join(MESHES, name))
    return V, F


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    weightbench = sys.argv[1]
    V1, F1 = load("sphere.obj")
    V2, F2 = load("grid.obj")
    N1 = igl.per_vertex_normals(V1, F1)
    N2 = igl.per_vertex_normals(V2, F2)

    # Epic's demo uses constant weights, which any method reproduces; a weight
    # that varies over the sphere actually exercises the inpainting.
    x = V1[:, 0]
    w0 = (x - x.min()) / (x.max() - x.min())
    W1 = np.stack([w0, 1.0 - w0, np.full_like(w0, 0.0)], axis=1)
    W1[:, 2] = 0.5 * np.sin(3 * V1[:, 2]) ** 2
    W1 /= W1.sum(axis=1, keepdims=True)

    dist = 0.05 * igl.bounding_box_diagonal(V2)
    matched, W2 = find_matches_closest_surface(V1, F1, N1, V2, F2, N2, W1, dist * dist, 30)
    W_in, ok = inpaint(V2, F2, W2, matched)
    assert ok, "reference inpainting failed"
    W_ref, _ = smooth(V2, F2, W_in, matched, dist)
    print(f"reference: {matched.sum()} of {len(matched)} target vertices matched")

    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "sphere_weights.csv")
        np.savetxt(src, W1, delimiter=",")
        cmd = [weightbench, "--source", os.path.join(MESHES, "sphere.obj"), "--source-weights", src,
               "--garment", os.path.join(MESHES, "grid.obj"), "--method", "closest,inpaint",
               "--save-weights", tmp]
        subprocess.run(cmd, check=True)
        W_cpp = np.loadtxt(os.path.join(tmp, "grid.inpaint.csv"), delimiter=",")
        W_closest = np.loadtxt(os.path.join(tmp, "grid.closest.csv"), delimiter=",")

    diff = np.abs(W_cpp - W_ref).max()
    diff_closest = np.abs(W_closest - W2).max()
    print(f"max |cpp - reference|: inpaint {diff:.3e}, closest {diff_closest:.3e}")
    tol = 1e-8
    if diff > tol or diff_closest > tol:
        print("MISMATCH")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
