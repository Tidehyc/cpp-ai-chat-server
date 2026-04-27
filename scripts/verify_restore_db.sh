#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPOSE_FILE="${COMPOSE_FILE:-docker-compose.prod.yml}"
BACKUP_FILE="${1:-}"
VOLUME_NAME_PATTERN='(^|_)chat_data$'

find_volume_name() {
  docker volume ls --format '{{.Name}}' | grep -E "$VOLUME_NAME_PATTERN" | head -n1
}

if [[ -n "$BACKUP_FILE" ]]; then
  bash "$ROOT_DIR/scripts/restore_db.sh" "$BACKUP_FILE"
fi

VOLUME_NAME="$(find_volume_name || true)"
if [[ -z "$VOLUME_NAME" ]]; then
  echo "错误: 未找到数据库卷（期望匹配 chat_data）。"
  exit 1
fi

echo "[1/2] 验证后端健康状态..."
docker-compose -f "$COMPOSE_FILE" exec -T backend curl -fsS http://127.0.0.1:8080/health >/dev/null

echo "[2/2] 抽样校验核心表..."
docker run -i --rm -v "$VOLUME_NAME":/data python:3.12-slim python - <<'PY'
import sqlite3
from pathlib import Path

db_path = Path('/data/chat.db')
if not db_path.exists():
    raise SystemExit('错误: /data/chat.db 不存在')

conn = sqlite3.connect(str(db_path))
cur = conn.cursor()
cur.execute("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")
tables = [row[0] for row in cur.fetchall()]
print('TABLES:', ','.join(tables))

for table_name in ('users', 'messages', 'rooms'):
    if table_name not in tables:
        raise SystemExit(f'错误: 缺少表 {table_name}')
    cur.execute(f'SELECT COUNT(*) FROM {table_name}')
    print(f'COUNT {table_name}:', cur.fetchone()[0])

conn.close()
PY

echo "恢复校验完成: ${BACKUP_FILE:-当前数据库}"