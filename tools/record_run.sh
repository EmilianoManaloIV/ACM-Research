#!/usr/bin/env sh
# Writes a reproducibility stamp for one run, then runs the command.
# Usage: tools/record_run.sh <results_dir> <command...>
# Example: tools/record_run.sh data/results/2026-10-10-my-exp tools/run_benchmark.sh data/results/2026-10-10-my-exp all lbs
# The stamp (results_dir/run_record.txt) holds the commit, whether the tree had
# uncommitted changes, the OS, compiler, CMake and library versions, the Python
# package versions, and the exact command. Copy it into the experiment README.
set -e
if [ $# -lt 2 ]; then
  echo "usage: $0 <results_dir> <command...>" >&2
  exit 2
fi
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=$1
shift
mkdir -p "$OUT"
REC="$OUT/run_record.txt"
{
  echo "date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "commit: $(git -C "$ROOT" rev-parse HEAD)"
  if [ -n "$(git -C "$ROOT" status --porcelain)" ]; then
    echo "tree: uncommitted changes present"
  else
    echo "tree: clean"
  fi
  echo "os: $( (. /etc/os-release 2>/dev/null && echo "$PRETTY_NAME") || uname -sr)"
  echo "arch: $(uname -m)"
  echo "compiler: $( (c++ --version 2>/dev/null || cc --version 2>/dev/null) | head -n 1)"
  echo "cmake: $(cmake --version 2>/dev/null | head -n 1)"
  echo "cmake deps: $(grep -E 'GIT_TAG' "$ROOT/CMakeLists.txt" | awk '{print $2}' | tr '\n' ' ')"
  echo "python: $(python3 --version 2>&1)"
  echo "python packages: $(python3 -m pip freeze 2>/dev/null | grep -iE '^(numpy|scipy|libigl|pygltflib)=' | tr '\n' ' ')"
  echo "command: $*"
} > "$REC"
cat "$REC"
"$@"
echo "exit: $?" >> "$REC"
