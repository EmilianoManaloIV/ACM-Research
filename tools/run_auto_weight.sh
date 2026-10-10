#!/usr/bin/env sh
# Runs experiment 2026-10-10-auto-weight-baselines (RQ-002): every method on
# every character, once, as the protocol allows.
# Usage: tools/run_auto_weight.sh <out_dir>
# Needs build/apps/bodybench, Blender 4.0.2 on PATH, data/makehuman/body.gltf
# (tools/make_makehuman.py) and data/articulation-xl2/characters/
# (tools/fetch_articulation.py convert). Writes one folder per character with
# its bones, poses, bone heat weights and results, and <out_dir>/results.csv.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=$1
[ -n "$OUT" ] || { echo "usage: $0 <out_dir>" >&2; exit 2; }
BENCH="$ROOT/build/apps/bodybench/bodybench"
LIST="$ROOT/experiments/2026-10-10-auto-weight-baselines/characters.csv"
mkdir -p "$OUT"

run_one() {  # name group body [artist.csv]
  name=$1; group=$2; body=$3; artist=$4
  dir="$OUT/$name"
  mkdir -p "$dir"
  set -- --body "$body"
  [ -n "$artist" ] && set -- "$@" --artist-weights "$artist"
  "$BENCH" "$@" --export-bones "$dir/bones.json" --export-mesh "$dir/body.obj" > /dev/null
  python3 "$ROOT/tools/make_body_poses.py" "$dir/bones.json" "$dir/poses.json"
  if ! blender -b --factory-startup --python "$ROOT/tools/blender_bone_heat.py" -- \
      "$dir/body.obj" "$dir/bones.json" "$dir/bone_heat.csv" > "$dir/blender.log" 2>&1; then
    echo "$name: Blender failed, see $dir/blender.log"
  fi
  grep "bone heat:" "$dir/blender.log" || true
  "$BENCH" "$@" --poses "$dir/poses.json" --pose-set test,named --character "$name" \
    --method "artist,floor,bbw,csv:bone_heat=$dir/bone_heat.csv" --save-weights "$dir" \
    --out "$dir/results.csv" > "$dir/bodybench.log" 2>&1 || echo "$name: bodybench failed, see $dir/bodybench.log"
  grep -E "^(artist|floor|bbw|bone_heat) " "$dir/bodybench.log" | sed "s/^/$name /" || true
  echo "$group" > "$dir/group.txt"
}

run_one makehuman humanoid "$ROOT/data/makehuman/body.gltf"
tail -n +2 "$LIST" | while IFS=, read -r group uuid rest; do
  c="$ROOT/data/articulation-xl2/characters/$uuid"
  run_one "$uuid" "$group" "$c/body.glb" "$c/artist.csv"
done

# One table: the group goes in front of every row.
first=1
for dir in "$OUT"/*/; do
  [ -f "$dir/results.csv" ] || continue
  g=$(cat "$dir/group.txt")
  if [ $first = 1 ]; then
    head -n 1 "$dir/results.csv" | sed 's/^/group,/' > "$OUT/results.csv"
    first=0
  fi
  tail -n +2 "$dir/results.csv" | sed "s/^/$g,/" >> "$OUT/results.csv"
done
echo "wrote $OUT/results.csv"
