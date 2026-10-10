#!/usr/bin/env sh
# Runs every method on every synthetic garment over the held-out test poses and
# collects one CSV. Usage: tools/run_benchmark.sh [results_dir] [methods]
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/data/results"}
METHODS=${2:-all}
BENCH="$ROOT/build/apps/weightbench/weightbench"
DATA="$ROOT/data/synthetic"
mkdir -p "$OUT/weights"
rm -f "$OUT/results.csv"
for case in arm/sleeve_tight arm/sleeve_loose arm/cuff legs/trousers legs/skirt legs/dress; do
  body=$(dirname "$case")
  garment=$(basename "$case")
  echo "== $case"
  "$BENCH" --body "$DATA/$body/body.gltf" --garment "$DATA/$case.obj" --poses "$DATA/$body/poses.json" \
    --pose-set test --method "$METHODS" --out "$OUT/$garment.csv" --save-weights "$OUT/weights" | grep -v '^source'
  if [ -f "$OUT/results.csv" ]; then
    tail -n +2 "$OUT/$garment.csv" >> "$OUT/results.csv"
  else
    cp "$OUT/$garment.csv" "$OUT/results.csv"
  fi
done
echo "wrote $OUT/results.csv"
