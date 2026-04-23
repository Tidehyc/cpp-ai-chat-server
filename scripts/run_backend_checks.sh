#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
REPORT_DIR="$ROOT_DIR/docs/perf_reports"
PERF_WARN_RATIO="${PERF_WARN_RATIO:-0.80}"
PERF_WARN_FAIL="${PERF_WARN_FAIL:-0}"

echo "[1/5] Configure project"
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
fi

echo "[2/5] Build"
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "[3/5] Run CTest"
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "[4/5] Run message history benchmark"
BENCH_OUTPUT="$(python3 "$ROOT_DIR/scripts/benchmark_message_history.py")"
echo "$BENCH_OUTPUT"

mkdir -p "$REPORT_DIR"
STAMP="$(date +%Y%m%d_%H%M%S)"
REPORT_FILE="$REPORT_DIR/message_history_benchmark_${STAMP}.txt"
printf "%s\n" "$BENCH_OUTPUT" > "$REPORT_FILE"

echo "Saved benchmark report: $REPORT_FILE"

echo "[5/5] Summarize benchmark history"
SUMMARY_ARGS=(--warn-ratio "$PERF_WARN_RATIO")
if [[ "$PERF_WARN_FAIL" == "1" || "$PERF_WARN_FAIL" == "true" || "$PERF_WARN_FAIL" == "TRUE" ]]; then
  SUMMARY_ARGS+=(--fail-on-warn)
fi
python3 "$ROOT_DIR/scripts/summarize_perf_reports.py" "${SUMMARY_ARGS[@]}"
