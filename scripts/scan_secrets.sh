#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

PATTERN='(AKIA[0-9A-Z]{16}|AIza[0-9A-Za-z_-]{35}|xox[baprs]-[0-9A-Za-z-]{10,}|ghp_[0-9A-Za-z]{36}|sk-[A-Za-z0-9]{20,}|Bearer [A-Za-z0-9._-]{20,}|-----BEGIN (RSA|EC|OPENSSH|PRIVATE) KEY-----|api[_-]?key\s*[:=]\s*["\x27]?[A-Za-z0-9._-]{16,})'
IGNORE_PATTERN='(your_api_key_here|change_me_now|sk-xxx|YOUR_API_KEY|REPLACE_ME|example)'
SCAN_MODE="tracked"

if [[ "${1:-}" == "--all-files" ]]; then
  SCAN_MODE="all"
fi

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  cat <<'EOF'
Usage:
  bash scripts/scan_secrets.sh            # scan tracked files only (default)
  bash scripts/scan_secrets.sh --all-files # scan whole working tree including local untracked files
EOF
  exit 0
fi

EXCLUDES=(
  '--glob' '!*build/*'
  '--glob' '!*node_modules/*'
  '--glob' '!*docs/archive/*'
  '--glob' '!*frontend/node_modules/*'
  '--glob' '!*.venv/*'
)

echo "[scan] secret pattern scan mode=$SCAN_MODE"
set +e
if [[ "$SCAN_MODE" == "tracked" ]]; then
  mapfile -d '' tracked_files_raw < <(git ls-files -z -- . \
    ':(exclude)build/**' \
    ':(exclude)node_modules/**' \
    ':(exclude)frontend/node_modules/**' \
    ':(exclude)docs/archive/**')

  tracked_files=()
  for file in "${tracked_files_raw[@]}"; do
    if [[ -f "$file" ]]; then
      tracked_files+=("$file")
    fi
  done

  if [[ ${#tracked_files[@]} -eq 0 ]]; then
    echo "SCAN_STATUS=OK"
    echo "SCAN_SUMMARY=No tracked files to scan"
    exit 0
  fi

  scan_output="$(rg -n -i "$PATTERN" "${tracked_files[@]}")"
  scan_rc=$?
else
  scan_output="$(rg -n --hidden --no-ignore -i "${EXCLUDES[@]}" "$PATTERN" .)"
  scan_rc=$?
fi
set -e

if [[ $scan_rc -eq 1 ]]; then
  echo "SCAN_STATUS=OK"
  echo "SCAN_SUMMARY=No secret-like patterns found"
  exit 0
fi

if [[ $scan_rc -ne 0 ]]; then
  echo "SCAN_STATUS=FAILED"
  echo "SCAN_SUMMARY=Scanner execution failed"
  exit 2
fi

set +e
filtered_output="$(printf "%s\n" "$scan_output" | rg -vi "$IGNORE_PATTERN")"
filtered_rc=$?
set -e

if [[ $filtered_rc -eq 1 ]]; then
  echo "SCAN_STATUS=OK"
  echo "SCAN_SUMMARY=No secret-like patterns found (placeholders ignored)"
  exit 0
fi

if [[ $filtered_rc -ne 0 ]]; then
  echo "SCAN_STATUS=FAILED"
  echo "SCAN_SUMMARY=Failed to post-process scan output"
  exit 2
fi

printf "%s\n" "$filtered_output"
echo "SCAN_STATUS=WARN"
echo "SCAN_SUMMARY=Potential secret-like matches found; manual review required"
exit 1
