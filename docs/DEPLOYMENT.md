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
sudo ufw allow 8080/tcp  # 后端 WebSocket
```

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
```

### 2.3 修改 docker-compose.yml 用于公网部署

创建 `docker-compose.prod.yml`：

```yaml
version: '3.8'

services:
  # 后端服务器
  backend:
    build: .
    container_name: chat-backend
    ports:
      - "8080:8080"
    environment:
      - AI_API_KEY=${AI_API_KEY}
      - AI_API_URL=${AI_API_URL}
      - AI_MODEL=${AI_MODEL}
      - SERVER_PORT=8080
    volumes:
      - chat_data:/app/data
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "nc", "-z", "localhost", "8080"]
      interval: 30s
      timeout: 10s
      retries: 3

  # 前端 Nginx
  frontend:
    build:
      context: ./frontend
      dockerfile: Dockerfile.prod
    container_name: chat-frontend
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx/nginx.conf:/etc/nginx/nginx.conf:ro
      - ./nginx/ssl:/etc/nginx/ssl:ro  # SSL 证书
    depends_on:
      - backend
    restart: unless-stopped

volumes:
  chat_data:
```

### 2.4 创建前端 Dockerfile

创建 `frontend/Dockerfile.prod`：

```dockerfile
# 构建阶段
FROM node:20-alpine AS builder

WORKDIR /app
COPY package*.json ./
RUN npm ci --production

COPY . .
RUN npm run build

# 生产阶段
FROM nginx:alpine

COPY --from=builder /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
```

### 2.5 创建前端 Nginx 配置

创建 `frontend/nginx.conf`：

```nginx
server {
    listen 80;
    server_name localhost;

    root /usr/share/nginx/html;
    index index.html;

    # SPA 路由支持
    location / {
        try_files $uri $uri/ /index.html;
    }

    # 静态资源缓存
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
}
```

### 2.6 创建后端 Nginx 配置（反向代理）

创建 `nginx/nginx.conf`：

```nginx
events {
    worker_connections 1024;
}

http {
    upstream backend {
        server backend:8080;
    }

    server {
        listen 80;
        server_name your-domain.com;  # 替换为你的域名

        # 前端静态文件
        location / {
            root /usr/share/nginx/html;
            index index.html;
            try_files $uri $uri/ /index.html;
        }

        # WebSocket 代理
        location /ws {
            proxy_pass http://backend;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection "upgrade";
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_read_timeout 86400;
        }

        # API 代理
        location /api {
            proxy_pass http://backend;
            proxy_http_version 1.1;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
        }
    }

    # HTTPS 配置（可选）
    # server {
    #     listen 443 ssl;
    #     server_name your-domain.com;
    #
    #     ssl_certificate /etc/nginx/ssl/cert.pem;
    #     ssl_certificate_key /etc/nginx/ssl/key.pem;
    #
    #     location / {
    #         root /usr/share/nginx/html;
    #         try_files $uri $uri/ /index.html;
    #     }
    #
    #     location /ws {
    #         proxy_pass http://backend;
    #         proxy_http_version 1.1;
    #         proxy_set_header Upgrade $http_upgrade;
    #         proxy_set_header Connection "upgrade";
    #         proxy_set_header Host $host;
    #     }
    # }
}
```

### 2.7 修改前端 WebSocket 配置

编辑 `frontend/src/services/websocket.ts`，将 WebSocket URL 改为环境变量：

```typescript
// 修改构造函数
constructor(url?: string) {
    this.config = {
        url: url || process.env.VITE_WS_URL || 'ws://localhost:8080',
        maxReconnectAttempts: 10,
        reconnectDelay: 3000,
        heartbeatInterval: 30000,
    }
}
```

创建 `frontend/.env.production`：

```ini
VITE_WS_URL=wss://your-domain.com/ws
```

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
# 检查后端是否运行
curl http://your-server-ip:8080

# 应该返回 WebSocket 升级响应
```

### 4.2 测试前端

浏览器访问：
- HTTP: `http://your-server-ip`
- HTTPS: `https://your-domain.com`

### 4.3 测试 WebSocket

打开浏览器控制台，输入：

```javascript
const ws = new WebSocket('ws://your-server-ip:8080');
ws.onopen = () => console.log('连接成功');
ws.onmessage = (e) => console.log('收到消息:', e.data);
```

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
