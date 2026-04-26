# 服务器部署指南

## 环境要求

- 云服务器（Ubuntu 20.04+）
- 域名（可选，用于 HTTPS）
- 2GB+ 内存

---

## 步骤 1：准备服务器

### 1.1 购买云服务器

推荐：
- 阿里云 ECS
- 腾讯云 CVM
- AWS EC2

### 1.2 安装 Docker

```bash
# 安装 Docker
curl -fsSL https://get.docker.com | sh

# 启动 Docker
sudo systemctl enable docker
sudo systemctl start docker

# 安装 Docker Compose
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
```

### 1.3 配置防火墙

```bash
# 开放端口
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
```

生产部署时不建议将后端 8080 直接暴露到公网。后端应仅在容器/内网中可达，由前端 Nginx 统一通过 `/ws` 和 `/api` 反代。

如果你需要本地开发或直连调试后端，再临时开放 8080；生产环境默认不要开放该端口。

---

## 步骤 2：部署项目

### 2.1 克隆项目

```bash
git clone https://github.com/Tidehyc/cpp-ai-chat-server.git
cd cpp-ai-chat-server
```

### 2.2 配置环境变量

```bash
cp .env.example .env
nano .env
```

编辑 `.env` 文件：

```ini
# AI API 配置
AI_API_KEY=你的真实_API_Key
AI_API_URL=https://api.scnet.cn/api/llm/v1/chat/completions
AI_MODEL=MiniMax-M2.5

# 服务器配置
SERVER_PORT=8080

# 前端 WebSocket 配置（可选）
# 默认同源 /ws；如独立部署可设置为 wss://your-domain.com/ws
VITE_WS_URL=/ws
```

### 2.3 已提供的生产模板文件

仓库里已经补齐了可直接使用的生产模板，不需要手工抄写下面的配置片段：

- [docker-compose.prod.yml](../docker-compose.prod.yml) - 后端 + 前端 + HTTPS/WSS 入口
- [frontend/Dockerfile.prod](../frontend/Dockerfile.prod) - 前端生产镜像
- [frontend/nginx/nginx.conf](../frontend/nginx/nginx.conf) - 前端静态站点与反代配置

启动时建议直接使用：

```bash
docker-compose -f docker-compose.prod.yml up -d --build
```

### 2.4 前端 WebSocket 连接方式

前端现在默认连接同源的 `/ws`，浏览器在 HTTPS 页面下会自动使用 WSS。
如果前端和后端不是同源部署，可以在 `frontend/.env.production` 中显式指定：

```ini
VITE_WS_URL=wss://your-domain.com/ws
```

如需本地开发直连后端，也可以设置为：

```ini
VITE_WS_URL=ws://localhost:8080/ws
```

前端 WebSocket 服务会优先读取 `VITE_WS_URL`，否则使用当前页面协议与主机拼接 `/ws`。

创建 `frontend/.env.production`：

```ini
VITE_WS_URL=wss://your-domain.com/ws
```

如果使用 Vite 开发服务器，本地 `frontend/vite.config.ts` 的 `/ws` 代理会把连接转发到后端 `ws://localhost:8080`。

### 2.8 启动服务

```bash
# 构建并启动
docker-compose -f docker-compose.prod.yml up -d

# 查看日志
docker-compose -f docker-compose.prod.yml logs -f

# 查看状态
docker-compose -f docker-compose.prod.yml ps
```

---

## 步骤 3：配置域名和 HTTPS（推荐）

### 3.1 域名解析

在域名管理后台添加 A 记录：

```
类型: A
主机记录: @ 或 www
记录值: 你的服务器 IP
TTL: 600
```

### 3.2 申请 SSL 证书

使用 Let's Encrypt 免费证书：

```bash
# 安装 certbot
sudo apt install certbot

# 申请证书
sudo certbot certonly --standalone -d your-domain.com
```

证书位置：
- 证书：`/etc/letsencrypt/live/your-domain.com/cert.pem`
- 私钥：`/etc/letsencrypt/live/your-domain.com/privkey.pem`

### 3.3 配置 Nginx HTTPS

复制证书到 nginx 目录：

```bash
sudo mkdir -p nginx/ssl
sudo cp /etc/letsencrypt/live/your-domain.com/cert.pem nginx/ssl/
sudo cp /etc/letsencrypt/live/your-domain.com/privkey.pem nginx/ssl/
```

取消 `nginx/nginx.conf` 中 HTTPS 部分的注释。

### 3.4 自动续期证书

```bash
# 添加定时任务
sudo crontab -e

# 添加以下行（每月 1 号凌晨 3 点续期）
0 3 1 * * certbot renew --quiet && docker-compose -f docker-compose.prod.yml restart frontend
```

---

## 步骤 4：验证部署

### 4.1 测试后端

```bash
# 检查后端是否运行（生产中通常只在内网/容器网络中检查）
docker-compose -f docker-compose.prod.yml exec -T backend curl -fsS http://127.0.0.1:8080/health

# 如果你是本地直连调试，可临时访问 8080
# curl http://your-server-ip:8080
```

### 4.2 测试前端

浏览器访问：
- HTTP: `http://your-server-ip`
- HTTPS: `https://your-domain.com`

### 4.3 测试 WebSocket

打开浏览器控制台，输入：

```javascript
const ws = new WebSocket('wss://your-domain.com/ws');
ws.onopen = () => console.log('连接成功');
ws.onmessage = (e) => console.log('收到消息:', e.data);
```

生产环境建议只通过 HTTPS/WSS 访问，避免明文 WebSocket 直接暴露。

---

## 数据库备份与恢复（B2）

数据库采用 SQLite 持久化到 Docker volume。建议每天执行一次全量备份，并保留最近 7 天的备份文件。

### 备份命令

```bash
bash scripts/backup_db.sh
```

### 恢复命令

```bash
bash scripts/restore_db.sh
```

如需指定某个备份文件，也可以传入路径：

```bash
bash scripts/restore_db.sh backups/db/chat.db.20260425_030000.sqlite3
```

### 恢复建议

1. 恢复前先确认当前后端可短暂停机。
2. 恢复后执行 `docker-compose -f docker-compose.prod.yml exec -T backend curl -fsS http://127.0.0.1:8080/health`。
3. 至少抽查用户、消息、房间三类数据是否可正常读取。
4. 如需一键完成恢复 + 健康检查 + 抽样校验，可执行 `bash scripts/verify_restore_db.sh [备份文件]`。

### 运维注意事项

- 备份文件应保留在独立目录，不要与源码混放。
- 恢复前不要覆盖原备份。
- 如果恢复失败，先保留现场，再切回上一个可用备份。

---

## 常见问题

### Q1: WebSocket 连接失败

**解决**：
1. 检查防火墙是否开放 8080 端口
2. 检查 Nginx 配置是否正确代理 WebSocket
3. 检查前端 WebSocket URL 是否正确

### Q2: 前端页面空白

**解决**：
1. 检查 Nginx 日志：`docker logs chat-frontend`
2. 确保 SPA 路由配置正确（try_files）
3. 检查浏览器控制台是否有错误

### Q3: 数据库持久化

数据已保存在 Docker Volume `chat_data` 中：

```bash
# 查看数据
docker volume inspect chat_data

# 备份数据
docker run --rm -v chat_data:/data -v $(pwd):/backup alpine tar czf /backup/chat_data.tar.gz -C /data .
```

---

## 性能优化（可选）

### 启用 Gzip 压缩

在 Nginx 配置中添加：

```nginx
gzip on;
gzip_types text/plain text/css application/json application/javascript text/xml application/xml;
gzip_min_length 1000;
```

### 启用 HTTP/2

```nginx
listen 443 ssl http2;
```

### 添加 Rate Limiting

```nginx
limit_req_zone $binary_remote_addr zone=api:10m rate=10r/s;

location /ws {
    limit_req zone=api burst=20;
    ...
}
```

---

## 监控与告警（已提供模板）

生产编排已内置监控栈：Prometheus + Grafana + node-exporter + blackbox-exporter。

### 启动方式

```bash
docker-compose -f docker-compose.prod.yml up -d --build
```

### 访问入口

- Grafana: `http://127.0.0.1:3000`
- Prometheus: `http://127.0.0.1:9090`

默认 Grafana 账号从环境变量读取：

- `GRAFANA_ADMIN_USER`（默认 `admin`）
- `GRAFANA_ADMIN_PASSWORD`（默认 `change_me_now`，上线请务必修改）

### 已预置采集与探测

- 主机资源：CPU / 内存（node-exporter）
- 服务可用性：
    - `http://backend:8080/health`
    - `https://frontend:443/health`
    （blackbox-exporter 主动探测）

### 已预置告警阈值

- `BackendHealthcheckFailed`：后端健康检查连续 2 分钟失败（critical）
- `FrontendHealthcheckFailed`：前端健康检查连续 2 分钟失败（warning）
- `HostCpuUsageHigh`：主机 CPU 连续 5 分钟 > 85%（warning）
- `HostMemoryUsageHigh`：主机内存连续 5 分钟 > 90%（warning）

### 看板文件位置

- [monitoring/grafana/dashboards/chat-ops-overview.json](../monitoring/grafana/dashboards/chat-ops-overview.json)

### 配置文件位置

- [monitoring/prometheus/prometheus.yml](../monitoring/prometheus/prometheus.yml)
- [monitoring/prometheus/alerts.yml](../monitoring/prometheus/alerts.yml)
- [monitoring/blackbox/blackbox.yml](../monitoring/blackbox/blackbox.yml)

---

## 发布与回滚演练（已提供模板）

已提供可执行脚本与演练手册：

- [scripts/release_prod.sh](../scripts/release_prod.sh)
- [scripts/rollback_prod.sh](../scripts/rollback_prod.sh)
- [docs/RELEASE_ROLLBACK_RUNBOOK.md](./RELEASE_ROLLBACK_RUNBOOK.md)

首次使用请赋予执行权限：

```bash
chmod +x scripts/release_prod.sh scripts/rollback_prod.sh
```

发布演练：

```bash
./scripts/release_prod.sh v2026.04.24-01
```

回滚演练：

```bash
./scripts/rollback_prod.sh v2026.04.24-01
```

每次执行都会自动在 [docs/release_records/](./release_records/) 生成记录文件。
