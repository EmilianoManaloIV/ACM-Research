#!/usr/bin/env sh
# Runs every method on every synthetic garment over the held-out test poses and
# collects one CSV. Usage: tools/run_benchmark.sh [results_dir] [methods] [lbs|dqs] [synthetic|makehuman]
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/data/results"}
METHODS=${2:-all}
SKINNING=${3:-lbs}
DATASET=${4:-synthetic}
BENCH="$ROOT/build/apps/weightbench/weightbench"
DATA="$ROOT/data/$DATASET"
if [ "$DATASET" = makehuman ]; then
  CASES="makehuman/skirt makehuman/tights makehuman/sleeve_tight makehuman/sleeve_loose makehuman/trousers makehuman/shirt"
  DATA="$ROOT/data"
else
  CASES="arm/sleeve_tight arm/sleeve_loose arm/cuff legs/trousers legs/skirt legs/dress"
fi
mkdir -p "$OUT/weights"
rm -f "$OUT/results.csv"
for case in $CASES; do
  body=$(dirname "$case")
  garment=$(basename "$case")
  echo "== $case"
  HEM=""
  case "$garment" in skirt|dress) [ -f "$DATA/$body/hem.json" ] && HEM="--hem-rig $DATA/$body/hem.json" ;; esac
  "$BENCH" --body "$DATA/$body/body.gltf" --garment "$DATA/$case.obj" --poses "$DATA/$body/poses.json" \
    --pose-set test --method "$METHODS" --skinning "$SKINNING" $HEM --out "$OUT/$garment.csv" --save-weights "$OUT/weights" | grep -v '^source'
  if [ -f "$OUT/results.csv" ]; then
    tail -n +2 "$OUT/$garment.csv" >> "$OUT/results.csv"
  else
    cp "$OUT/$garment.csv" "$OUT/results.csv"
  fi
done
echo "wrote $OUT/results.csv"
