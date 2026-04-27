#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPOSE_FILE="${COMPOSE_FILE:-docker-compose.prod.yml}"
BACKUP_DIR="${BACKUP_DIR:-$ROOT_DIR/backups/db}"
BACKUP_FILE="${1:-}"
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
if [[ -z "$BACKUP_FILE" ]]; then
  BACKUP_FILE="$(ls -1t "$BACKUP_DIR"/chat.db.*.sqlite3 2>/dev/null | head -n1 || true)"
fi

if [[ -z "$BACKUP_FILE" || ! -f "$BACKUP_FILE" ]]; then
  echo "错误: 未找到可用备份文件。"
  exit 1
fi

DB_PATH=""
if [[ -n "$VOLUME_NAME" ]]; then
  VOLUME_MOUNTPOINT="$(docker volume inspect -f '{{.Mountpoint}}' "$VOLUME_NAME")"
  DB_PATH="$VOLUME_MOUNTPOINT/chat.db"
fi

if [[ -z "$DB_PATH" || ! -d "$(dirname "$DB_PATH")" ]]; then
  for candidate in "${LOCAL_DB_CANDIDATES[@]}"; do
    mkdir -p "$(dirname "$candidate")"
    DB_PATH="$candidate"
    break
  done
fi

if [[ -z "$DB_PATH" ]]; then
  echo "错误: 无法确定数据库恢复目标。"
  exit 1
fi

echo "[1/4] 停止后端服务..."
docker-compose -f "$COMPOSE_FILE" stop backend || true

echo "[2/4] 恢复数据库文件..."
python3 - "$BACKUP_FILE" "$DB_PATH" <<'PY'
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

echo "[3/4] 启动后端服务..."
docker-compose -f "$COMPOSE_FILE" up -d backend

echo "[4/4] 验证数据库健康状态..."
if docker-compose -f "$COMPOSE_FILE" ps -q backend >/dev/null 2>&1; then
  docker-compose -f "$COMPOSE_FILE" up -d backend >/dev/null
  docker-compose -f "$COMPOSE_FILE" exec -T backend curl -fsS http://127.0.0.1:8080/health >/dev/null
fi

echo "恢复完成: $BACKUP_FILE"