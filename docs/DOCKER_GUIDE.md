# Docker 部署指南

_版本：v0.5.3_
_最后更新：2026-03-28_

---

## 📋 前置要求

- Docker 20.10+
- Docker Compose 2.0+
- 至少 1GB 可用内存
- 至少 500MB 磁盘空间

---

## 🚀 快速开始

### 1. 配置环境变量

```bash
cd /home/tide/workplace/cpp-ai-chat-server

# 复制配置模板
cp .env.example .env

# 编辑 .env 文件，填入你的 AI_API_KEY
nano .env
```

### 2. 构建镜像

**方式 1：使用 docker-compose**
```bash
docker-compose build
```

**方式 2：使用 Dockerfile**
```bash
docker build -t cpp-ai-chat-server:latest .
```

### 3. 启动服务器

**方式 1：使用启动脚本（推荐）**
```bash
./scripts/docker-start.sh
# 选择选项 1：启动服务器
```

**方式 2：使用 docker-compose**
```bash
docker-compose up -d
```

**方式 3：使用 Docker 命令**
```bash
docker run -d \
  --name cpp-ai-chat-server \
  -p 8080:8080 \
  -e AI_API_KEY="your_api_key" \
  -v chat_data:/app/data \
  cpp-ai-chat-server:latest
```

### 4. 验证运行

```bash
# 查看容器状态
docker ps

# 查看日志
docker-compose logs -f

# 测试连接
curl http://localhost:8080/health
```

---

## 🔧 常用命令

### 启动/停止

```bash
# 启动
docker-compose up -d

# 停止
docker-compose down

# 重启
docker-compose restart

# 查看状态
docker-compose ps
```

### 日志查看

```bash
# 实时日志
docker-compose logs -f chat-server

# 最近 100 行
docker-compose logs --tail=100 chat-server

# 导出日志
docker-compose logs chat-server > server.log
```

### 进入容器

```bash
# 进入容器 shell
docker-compose exec chat-server /bin/bash

# 查看容器信息
docker-compose exec chat-server env
```

### 数据管理

```bash
# 查看数据卷
docker volume ls

# 查看数据卷详情
docker volume inspect cpp_ai_chat_data

# 备份数据
docker run --rm -v cpp_ai_chat_data:/data -v $(pwd):/backup ubuntu tar czf /backup/chat_data.tar.gz /data

# 恢复数据
docker run --rm -v cpp_ai_chat_data:/data -v $(pwd):/backup ubuntu tar xzf /backup/chat_data.tar.gz -C /
```

---

## 📊 资源配置

### 内存限制

```yaml
# docker-compose.yml
services:
  chat-server:
    deploy:
      resources:
        limits:
          cpus: '1.0'
          memory: 512M
        reservations:
          cpus: '0.5'
          memory: 256M
```

### 端口映射

```yaml
# 默认端口
ports:
  - "8080:8080"

# 自定义端口
ports:
  - "9000:8080"  # 主机 9000 -> 容器 8080
```

---

## 🔍 故障排查

### 容器无法启动

```bash
# 查看详细日志
docker-compose logs chat-server

# 检查配置
docker-compose config

# 重新构建
docker-compose build --no-cache
```

### 端口被占用

```bash
# 查看占用端口的进程
lsof -i :8080

# 修改端口
# 编辑 docker-compose.yml，修改 ports 配置
ports:
  - "9080:8080"
```

### 数据丢失

```bash
# 检查数据卷
docker volume inspect cpp_ai_chat_data

# 恢复备份
docker run --rm -v cpp_ai_chat_data:/data -v $(pwd):/backup ubuntu \
  tar xzf /backup/chat_data.tar.gz -C /
```

---

## 📈 性能优化

### 多阶段构建

Dockerfile 已使用多阶段构建，减小镜像大小：
- 构建镜像：~2GB
- 运行镜像：~200MB

### 缓存优化

```bash
# 使用构建缓存
docker-compose build --build-arg BUILDKIT=1

# 清理未使用的镜像
docker image prune -f
```

---

## 🔐 安全建议

### 1. 环境变量

**不要**将 `.env` 文件提交到版本控制：

```bash
# 添加到 .gitignore
echo ".env" >> .gitignore
```

### 2. 网络隔离

使用独立网络：

```yaml
networks:
  chat_network:
    driver: bridge
    internal: false  # 设为 true 则完全隔离
```

### 3. 只读文件系统

```yaml
services:
  chat-server:
    read_only: true
    tmpfs:
      - /tmp
```

---

## 📝 生产环境部署

### 1. 使用 Docker Swarm

```bash
# 初始化 Swarm
docker swarm init

# 部署服务
docker service create \
  --name chat-server \
  --replicas 3 \
  -p 8080:8080 \
  --env-file .env \
  cpp-ai-chat-server:latest
```

### 2. 使用 Kubernetes

```yaml
# k8s/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: chat-server
spec:
  replicas: 3
  selector:
    matchLabels:
      app: chat-server
  template:
    spec:
      containers:
      - name: chat-server
        image: cpp-ai-chat-server:latest
        ports:
        - containerPort: 8080
        envFrom:
        - configMapRef:
            name: chat-config
```

---

## 📊 监控和告警

### 健康检查

```yaml
healthcheck:
  test: ["CMD", "pgrep", "-x", "chat_server"]
  interval: 30s
  timeout: 10s
  retries: 3
  start_period: 10s
```

### 资源监控

```bash
# 查看容器资源使用
docker stats chat-server

# 查看容器详情
docker inspect chat-server
```

---

_最后更新：2026-03-28_
_版本：v0.5.3_
