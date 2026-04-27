#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

COMPOSE_FILE="docker-compose.prod.yml"
RELEASE_VERSION="${1:-$(date +%Y%m%d-%H%M%S)}"
RECORD_FILE="docs/release_records/release-${RELEASE_VERSION}.md"
SSL_DIR="nginx/ssl"
FULLCHAIN_FILE="${SSL_DIR}/fullchain.pem"
PRIVKEY_FILE="${SSL_DIR}/privkey.pem"
FRONTEND_HTTP_PORT="${FRONTEND_HTTP_PORT:-80}"
FRONTEND_HTTPS_PORT="${FRONTEND_HTTPS_PORT:-443}"

if [[ ! -f "$COMPOSE_FILE" ]]; then
  echo "错误: 未找到 $COMPOSE_FILE"
  exit 1
fi

mkdir -p docs/release_records

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
  echo "[准备] 未检测到 TLS 证书，生成自签名证书用于演练..."
  mkdir -p "$SSL_DIR"
  openssl req -x509 -nodes -newkey rsa:2048 \
    -keyout "$PRIVKEY_FILE" \
    -out "$FULLCHAIN_FILE" \
    -days 3650 \
    -subj "/CN=localhost" >/dev/null 2>&1
  echo "[准备] 自签名证书已生成: ${FULLCHAIN_FILE}, ${PRIVKEY_FILE}"
fi

echo "[发布] 版本: $RELEASE_VERSION"
echo "[发布] 使用编排: $COMPOSE_FILE"
echo "[发布] 前端端口: HTTP=${FRONTEND_HTTP_PORT}, HTTPS=${FRONTEND_HTTPS_PORT}"

echo "[1/5] 清理旧容器(兼容 docker-compose v1)..."
docker-compose -f "$COMPOSE_FILE" down --remove-orphans || true

echo "[2/5] 构建与部署..."
docker-compose -f "$COMPOSE_FILE" up -d --build

echo "[3/5] 健康检查(后端)..."
wait_for_healthcheck "后端" "docker-compose -f ${COMPOSE_FILE} exec -T backend curl -fsS http://127.0.0.1:8080/health" 40 2

echo "[4/5] 健康检查(前端 HTTPS)..."
wait_for_healthcheck "前端" "curl -kfsS https://127.0.0.1:${FRONTEND_HTTPS_PORT}/health" 40 2

echo "[5/5] 记录发布信息..."
cat > "$RECORD_FILE" <<EOF
# 发布记录 ${RELEASE_VERSION}

- 时间: $(date '+%Y-%m-%d %H:%M:%S')
- 执行人: ${USER:-unknown}
- 编排文件: ${COMPOSE_FILE}
- 前端端口: HTTP=${FRONTEND_HTTP_PORT}, HTTPS=${FRONTEND_HTTPS_PORT}
- 发布状态: success

## 执行命令

1. docker-compose -f ${COMPOSE_FILE} down --remove-orphans
2. docker-compose -f ${COMPOSE_FILE} up -d --build
3. docker-compose -f ${COMPOSE_FILE} exec -T backend curl -fsS http://127.0.0.1:8080/health
4. curl -kfsS https://127.0.0.1:${FRONTEND_HTTPS_PORT}/health

## 容器状态摘要

\
$(docker-compose -f "$COMPOSE_FILE" ps)

## 备注

- 如需回滚，执行: ./scripts/rollback_prod.sh ${RELEASE_VERSION}
EOF

echo "发布完成，记录已生成: $RECORD_FILE"
