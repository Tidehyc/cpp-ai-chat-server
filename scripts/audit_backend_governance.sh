#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

STATUS="OK"
FAILURES=()

report_ok() {
  echo "AUDIT_OK=$1"
}

report_fail() {
  local key="$1"
  local message="$2"
  STATUS="FAILED"
  FAILURES+=("$key")
  echo "AUDIT_FAIL=$key|$message"
}

check_file() {
  local rel_path="$1"
  local key="$2"
  local message="$3"

  if [[ -f "$ROOT_DIR/$rel_path" ]]; then
    report_ok "$key"
  else
    report_fail "$key" "$message"
  fi
}

check_path() {
  local rel_path="$1"
  local key="$2"
  local message="$3"

  if [[ -e "$ROOT_DIR/$rel_path" ]]; then
    report_ok "$key"
  else
    report_fail "$key" "$message"
  fi
}

check_contains() {
  local rel_path="$1"
  local key="$2"
  local pattern="$3"
  local message="$4"

  if grep -Fq "$pattern" "$ROOT_DIR/$rel_path"; then
    report_ok "$key"
  else
    report_fail "$key" "$message"
  fi
}

echo "AUDIT_STATUS=START"

check_file ".github/workflows/backend-checks.yml" "workflow_file" "缺少 backend-checks workflow 文件。"
check_file "scripts/run_backend_checks.sh" "backend_checks_script" "缺少后端检查脚本。"
check_file "docs/CONTRIBUTING_BACKEND.md" "contributing_doc" "缺少后端贡献流程文档。"
check_file "docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md" "required_checks_doc" "缺少 required checks 配置文档。"
check_file "docs/BRANCH_PROTECTION_RELEASE_TEMPLATE.md" "release_template_doc" "缺少 release 分支保护模板。"
check_file "docs/PERF_GATE_CHANGE_POLICY.md" "perf_policy_doc" "缺少性能门禁阈值治理策略文档。"
check_file "docs/PERFORMANCE_REPORT_TEMPLATE.md" "perf_report_template" "缺少性能报告模板。"
check_file "docs/perf_reports/PERF_GATE_CHANGE_RECORD_TEMPLATE.md" "perf_gate_record_template" "缺少性能门禁阈值记录模板。"
check_path "docs/perf_reports/perf_gate_changes" "perf_gate_changes_dir" "缺少性能门禁变更记录目录。"
check_file "docs/INDEX.md" "docs_index" "缺少 docs 总索引文档。"
check_file "docs/STUDY_NOTES_INDEX.md" "study_notes_index" "缺少学习笔记总览文档。"
check_file ".github/pull_request_template.md" "pr_template" "缺少 PR 模板。"

check_contains "README.md" "readme_contributing_link" "docs/CONTRIBUTING_BACKEND.md" "README 未链接后端贡献流程文档。"
check_contains "README.md" "readme_release_template_link" "docs/BRANCH_PROTECTION_RELEASE_TEMPLATE.md" "README 未链接 release 分支保护模板。"
check_contains "README.md" "readme_perf_policy_link" "docs/PERF_GATE_CHANGE_POLICY.md" "README 未链接性能门禁阈值治理策略。"
check_contains "README.md" "readme_perf_gate_changes_link" "docs/perf_reports/perf_gate_changes/" "README 未链接性能门禁变更记录目录。"
check_contains "README.md" "readme_governance_audit_link" "scripts/audit_backend_governance.sh" "README 未提供治理巡检脚本入口。"
check_contains "README.md" "readme_docs_index_link" "docs/INDEX.md" "README 未链接 docs 总索引。"

check_contains "docs/INDEX.md" "index_playbook_link" "PRODUCT_LAUNCH_PLAYBOOK.md" "docs 索引缺少上线手册入口。"
check_contains "docs/INDEX.md" "index_roadmap_link" "ROADMAP.md" "docs 索引缺少路线图入口。"
check_contains "docs/INDEX.md" "index_perf_gate_changes_link" "perf_reports/perf_gate_changes/" "docs 索引缺少性能门禁变更记录目录入口。"
check_contains "docs/STUDY_NOTES_INDEX.md" "study_index_main_note" "study_notes.md" "学习笔记索引缺少主笔记入口。"

check_contains "docs/BACKEND_TODO_CHECKLIST.md" "checklist_p2_5" "### 5. 例行巡检自动化脚本" "清单中缺少 P2-5 巡检脚本条目。"

if [[ "$STATUS" == "OK" ]]; then
  echo "AUDIT_STATUS=OK"
  echo "AUDIT_SUMMARY=All governance checks passed."
  exit 0
fi

echo "AUDIT_STATUS=FAILED"
echo "AUDIT_SUMMARY=${#FAILURES[@]} check(s) failed: ${FAILURES[*]}"
exit 1