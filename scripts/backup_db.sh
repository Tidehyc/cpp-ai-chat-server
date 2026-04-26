#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKUP_DIR="${BACKUP_DIR:-$ROOT_DIR/backups/db}"
VOLUME_NAME_PATTERN='(^|_)chat_data$'
LOCAL_DB_CANDIDATES=(
  "$ROOT_DIR/build/chat.db"
  "$ROOT_DIR/chat.db"
  "$ROOT_DIR/data/chat.db"
)

find_volume_name() {
  docker volume ls --format '{{.Name}}' | grep -E "$VOLUME_NAME_PATTERN" | head -n1
}

VOLUME_NAME="$(find_volume_name || true)"
if [[ -z "$VOLUME_NAME" ]]; then
  echo "错误: 未找到数据库卷（期望匹配 chat_data）。"
  exit 1
fi

VOLUME_MOUNTPOINT="$(docker volume inspect -f '{{.Mountpoint}}' "$VOLUME_NAME")"
DB_PATH="$VOLUME_MOUNTPOINT/chat.db"

SOURCE_DB_PATH="$DB_PATH"
if [[ ! -f "$SOURCE_DB_PATH" ]]; then
  for candidate in "${LOCAL_DB_CANDIDATES[@]}"; do
    if [[ -f "$candidate" ]]; then
      SOURCE_DB_PATH="$candidate"
      break
    fi
  done
fi

if [[ ! -f "$SOURCE_DB_PATH" ]]; then
  echo "错误: 未找到数据库文件。已检查:"
  echo "- $DB_PATH"
  for candidate in "${LOCAL_DB_CANDIDATES[@]}"; do
    echo "- $candidate"
  done
  exit 1
fi

mkdir -p "$BACKUP_DIR"
STAMP="$(date +%Y%m%d_%H%M%S)"
BACKUP_FILE="$BACKUP_DIR/chat.db.${STAMP}.sqlite3"

python3 - "$SOURCE_DB_PATH" "$BACKUP_FILE" <<'PY'
import sqlite3
import sys

src_path, dst_path = sys.argv[1:3]
source = sqlite3.connect(src_path)
target = sqlite3.connect(dst_path)
with target:
    source.backup(target)
target.close()
source.close()
PY

echo "备份完成: $BACKUP_FILE"