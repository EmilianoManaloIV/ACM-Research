#!/usr/bin/env sh
# Runs experiment 2026-10-10-auto-weight-baselines-v2 (RQ-002): every method on
# every character of both sets, once, under the amended protocol.
# Usage: tools/run_auto_weight_v2.sh <out_dir>
# Needs build/apps/bodybench, Blender 4.0.2 on PATH, data/makehuman/body.gltf
# (tools/make_makehuman.py) and the welded characters in
# data/articulation-xl2/characters_welded/ (tools/fetch_articulation.py
# convert ... --weld). Every bodybench call ignores contact in the
# self-intersection metric and clips weights above -1e-6; BBW runs at
# libigl's default of 100 iterations. Writes <out_dir>/<set>/<character>/
# and <out_dir>/results.csv with the set and group in front of every row.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
[ -n "$1" ] || { echo "usage: $0 <out_dir>" >&2; exit 2; }
mkdir -p "$1"
OUT=$(cd "$1" && pwd)
BENCH="$ROOT/build/apps/bodybench/bodybench"
LIST="$ROOT/experiments/2026-10-10-auto-weight-baselines-v2/characters.csv"
CHARS="$ROOT/data/articulation-xl2/characters_welded"
FLAGS="--ignore-contact --negative-tolerance 1e-6"
BBW_LIMIT=600
# TetGen writes temporary files into the working directory; keep them out
# of the repository.
mkdir -p "$OUT/tmp"
cd "$OUT/tmp"

run_one() {  # set name group body [artist.csv]
  set_=$1; name=$2; group=$3; body=$4; artist=$5
  dir="$OUT/$set_/$name"
  mkdir -p "$dir"
  set -- --body "$body"
  [ -n "$artist" ] && set -- "$@" --artist-weights "$artist"
  "$BENCH" "$@" --export-bones "$dir/bones.json" --export-mesh "$dir/body.obj" > /dev/null
  python3 "$ROOT/tools/make_body_poses.py" "$dir/bones.json" "$dir/poses.json" > /dev/null
  if ! blender -b --factory-startup --python "$ROOT/tools/blender_bone_heat.py" -- \
      "$dir/body.obj" "$dir/bones.json" "$dir/bone_heat.csv" > "$dir/blender.log" 2>&1; then
    echo "$name: Blender failed, see $dir/blender.log"
  fi
  heat=$(sed -n 's/^bone heat:.* \([0-9.]*\) seconds;.*/\1/p' "$dir/blender.log")
  common="--poses $dir/poses.json --pose-set test,named --character $name --save-weights $dir $FLAGS"
  "$BENCH" "$@" $common --method "artist,floor,csv:bone_heat=$dir/bone_heat.csv" \
    ${heat:+--method-seconds bone_heat=$heat} \
    --out "$dir/results.csv" > "$dir/bodybench.log" 2>&1 || echo "$name: bodybench failed, see $dir/bodybench.log"
  # BBW in its own process, with a time limit: TetGen can crash or loop
  # forever on a bad surface, and either is a BBW failure, not a reason to
  # lose the other methods' rows or stall the run. Healthy solves here take
  # seconds.
  if timeout "$BBW_LIMIT" "$BENCH" "$@" $common --bbw-iterations 100 --method bbw --out "$dir/bbw.csv" > "$dir/bbw.log" 2>&1; then
    tail -n +2 "$dir/bbw.csv" >> "$dir/results.csv"
  else
    code=$?
    if [ "$code" = 124 ]; then why="timed out after $BBW_LIMIT s (TetGen did not finish)"; else why="bodybench exited with status $code (TetGen crashed)"; fi
    echo "$name,bbw,weights,lbs,0,,,,,,,,,,,,\"$why\"" >> "$dir/results.csv"
    echo "bbw        valid 0  $why" >> "$dir/bbw.log"
  fi
  grep -hE "^(artist|floor|bbw|bone_heat) " "$dir/bodybench.log" "$dir/bbw.log" | sed "s/^/$set_ $name /" || true
  printf '%s,%s' "$set_" "$group" > "$dir/tag.txt"
}

run_one same makehuman biped "$ROOT/data/makehuman/body.gltf"
tail -n +2 "$LIST" | while IFS=, read -r set_ group uuid rest; do
  run_one "$set_" "$uuid" "$group" "$CHARS/$uuid/body.glb" "$CHARS/$uuid/artist.csv"
done

echo "set,group,character,method,pose,skinning,valid,unweighted,stretch_p95,stretch_mean,stretch_max,torn,self_intersection,l1,precision,recall,artist_pose_distance,seconds,note" > "$OUT/results.csv"
for dir in "$OUT"/same/*/ "$OUT"/fresh/*/; do
  [ -f "$dir/results.csv" ] || continue
  tag=$(cat "$dir/tag.txt")
  tail -n +2 "$dir/results.csv" | sed "s/^/$tag,/" >> "$OUT/results.csv"
done
cd "$OUT" && rm -rf "$OUT/tmp"
echo "wrote $OUT/results.csv"
