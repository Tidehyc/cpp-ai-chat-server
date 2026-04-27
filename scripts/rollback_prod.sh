#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

COMPOSE_FILE="docker-compose.prod.yml"
TARGET_VERSION="${1:-manual}"
ROLLBACK_ID="rollback-$(date +%Y%m%d-%H%M%S)"
RECORD_FILE="docs/release_records/${ROLLBACK_ID}.md"
SSL_DIR="nginx/ssl"
FULLCHAIN_FILE="${SSL_DIR}/fullchain.pem"
PRIVKEY_FILE="${SSL_DIR}/privkey.pem"
FRONTEND_HTTP_PORT="${FRONTEND_HTTP_PORT:-80}"
FRONTEND_HTTPS_PORT="${FRONTEND_HTTPS_PORT:-443}"

is_port_busy() {
  local port="$1"
  ss -H -ltn "sport = :${port}" | grep -q .
}

wait_for_healthcheck() {
  local name="$1"
  local cmd="$2"
  local max_attempts="${3:-30}"
  local sleep_seconds="${4:-2}"

  local i
  for ((i=1; i<=max_attempts; i++)); do
    if eval "$cmd" >/dev/null 2>&1; then
      return 0
    fi
    sleep "$sleep_seconds"
  done

  echo "错误: ${name} 健康检查超时"
  return 1
}

if [[ ! -f "$COMPOSE_FILE" ]]; then
  echo "错误: 未找到 $COMPOSE_FILE"
  exit 1
fi

mkdir -p docs/release_records

if is_port_busy "$FRONTEND_HTTP_PORT"; then
  if [[ "$FRONTEND_HTTP_PORT" == "80" ]]; then
    FRONTEND_HTTP_PORT="18080"
  fi
fi

if is_port_busy "$FRONTEND_HTTPS_PORT"; then
  if [[ "$FRONTEND_HTTPS_PORT" == "443" ]]; then
    FRONTEND_HTTPS_PORT="18443"
  fi
fi

export FRONTEND_HTTP_PORT FRONTEND_HTTPS_PORT

if [[ ! -f "$FULLCHAIN_FILE" || ! -f "$PRIVKEY_FILE" ]]; then
  echo "[准备] 未检测到 TLS 证书，生成自签名证书用于回滚演练..."
  mkdir -p "$SSL_DIR"
  openssl req -x509 -nodes -newkey rsa:2048 \
    -keyout "$PRIVKEY_FILE" \
    -out "$FULLCHAIN_FILE" \
    -days 3650 \
    -subj "/CN=localhost" >/dev/null 2>&1
  echo "[准备] 自签名证书已生成: ${FULLCHAIN_FILE}, ${PRIVKEY_FILE}"
fi

echo "[回滚] 目标版本: $TARGET_VERSION"
echo "[回滚] 执行回滚动作(容器重建与重启)..."
echo "[回滚] 前端端口: HTTP=${FRONTEND_HTTP_PORT}, HTTPS=${FRONTEND_HTTPS_PORT}"

docker-compose -f "$COMPOSE_FILE" down
# 默认回滚到当前仓库对应可部署状态；如需精确版本请先切换 git tag/commit

docker-compose -f "$COMPOSE_FILE" up -d --build

echo "[验证] 健康检查..."
wait_for_healthcheck "后端" "docker-compose -f ${COMPOSE_FILE} exec -T backend curl -fsS http://127.0.0.1:8080/health" 40 2
wait_for_healthcheck "前端" "curl -kfsS https://127.0.0.1:${FRONTEND_HTTPS_PORT}/health" 40 2

cat > "$RECORD_FILE" <<EOF
# 回滚记录 ${ROLLBACK_ID}

- 时间: $(date '+%Y-%m-%d %H:%M:%S')
- 执行人: ${USER:-unknown}
- 目标版本: ${TARGET_VERSION}
- 编排文件: ${COMPOSE_FILE}
- 前端 HTTPS 端口: ${FRONTEND_HTTPS_PORT}
- 回滚状态: success

## 执行命令

1. docker-compose -f ${COMPOSE_FILE} down
2. docker-compose -f ${COMPOSE_FILE} up -d --build
3. docker-compose -f ${COMPOSE_FILE} exec -T backend curl -fsS http://127.0.0.1:8080/health
4. curl -kfsS https://127.0.0.1:${FRONTEND_HTTPS_PORT}/health

## 容器状态摘要

\
$(docker-compose -f "$COMPOSE_FILE" ps)

## 备注

- 如需精确回退历史版本，请先切换到目标 commit/tag 再执行本脚本。
EOF

echo "回滚完成，记录已生成: $RECORD_FILE"
