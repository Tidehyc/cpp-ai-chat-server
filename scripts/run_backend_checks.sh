#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
REPORT_DIR="$ROOT_DIR/docs/perf_reports"
PERF_WARN_RATIO="${PERF_WARN_RATIO:-0.80}"
PERF_WARN_FAIL="${PERF_WARN_FAIL:-0}"

# Stable non-zero exit codes for CI failure classification.
EXIT_CONFIGURE_FAILED=11
EXIT_BUILD_FAILED=21
EXIT_TEST_FAILED=31
EXIT_BENCHMARK_FAILED=41
EXIT_PERF_GATE_FAILED=51

fail_with_category() {
  local category="$1"
  local exit_code="$2"
  local hint="$3"

  echo "CHECK_STATUS=FAILED"
  echo "CHECK_FAILURE_CATEGORY=${category}"
  echo "CHECK_FAILURE_CODE=${exit_code}"
  echo "CHECK_FAILURE_HINT=${hint}"
  exit "$exit_code"
}

echo "[1/5] Configure project"
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  if ! cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug; then
    fail_with_category "build" "$EXIT_CONFIGURE_FAILED" "检查 CMake 配置与依赖安装，确认本地/CI 构建环境一致。"
  fi
fi

echo "[2/5] Build"
if ! cmake --build "$BUILD_DIR" -j"$(nproc)"; then
  fail_with_category "build" "$EXIT_BUILD_FAILED" "优先定位首个编译错误，修复后重新运行 bash scripts/run_backend_checks.sh。"
fi

echo "[3/5] Run CTest"
if ! ctest --test-dir "$BUILD_DIR" --output-on-failure; then
  fail_with_category "test" "$EXIT_TEST_FAILED" "查看失败用例日志并先复现单测，再回归全量 CTest。"
fi

echo "[4/5] Run message history benchmark"
if ! BENCH_OUTPUT="$(python3 "$ROOT_DIR/scripts/benchmark_message_history.py")"; then
  fail_with_category "perf" "$EXIT_BENCHMARK_FAILED" "检查 benchmark 运行环境和历史数据文件是否完整。"
fi
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
set +e
python3 "$ROOT_DIR/scripts/summarize_perf_reports.py" "${SUMMARY_ARGS[@]}"
summary_rc=$?
set -e
if [[ "$summary_rc" -ne 0 ]]; then
  if [[ "$summary_rc" -eq 2 ]]; then
    fail_with_category "perf-gate" "$EXIT_PERF_GATE_FAILED" "性能门禁触发，检查最新 speedup 与历史均值偏差并评估是否需要优化或调整阈值。"
  fi
  fail_with_category "perf" "$EXIT_BENCHMARK_FAILED" "性能汇总失败，检查 perf report 生成与解析。"
fi

echo "CHECK_STATUS=OK"
