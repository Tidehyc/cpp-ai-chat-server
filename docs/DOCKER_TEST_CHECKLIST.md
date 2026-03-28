# Docker 部署测试清单

_版本：v0.5.3_
_测试日期：_____________
_测试人员：_____________

---

## ✅ 前置条件

- [ ] Docker 已安装（`docker --version`）
- [ ] Docker Compose 已安装（`docker-compose --version`）
- [ ] Docker 服务正在运行（`systemctl status docker`）
- [ ] 当前用户有 Docker 权限（`docker ps`）

---

## 📋 测试 1：配置文件验证

### 1.1 检查文件完整性

```bash
cd /home/tide/workplace/cpp-ai-chat-server

# 检查必要文件
ls -la Dockerfile
ls -la docker-compose.yml
ls -la .env.example
ls -la .env  # 应该已创建
ls -la scripts/docker-start.sh
```

**预期**：所有文件存在

### 1.2 验证 docker-compose 配置

```bash
docker-compose config
```

**预期**：输出有效的 YAML 配置，无错误

---

## 📋 测试 2：Docker 镜像构建

### 2.1 构建镜像

```bash
docker-compose build
```

**预期**：
- ✅ 构建成功
- ✅ 无错误信息
- ✅ 镜像创建成功

**预计时间**：5-10 分钟（首次构建）

### 2.2 查看镜像

```bash
docker images | grep cpp-ai-chat-server
```

**预期**：
```
cpp-ai-chat-server_chat-server   latest   <IMAGE_ID>   <SIZE>
```

**镜像大小**：~200MB（运行镜像）

---

## 📋 测试 3：容器启动

### 3.1 启动容器

```bash
docker-compose up -d
```

**预期**：
- ✅ 容器创建成功
- ✅ 容器启动成功
- ✅ 端口映射正常

### 3.2 查看容器状态

```bash
docker-compose ps
```

**预期**：
```
NAME                   STATUS         PORTS
cpp-ai-chat-server     Up (healthy)   0.0.0.0:8080->8080/tcp
```

### 3.3 查看日志

```bash
docker-compose logs chat-server
```

**预期日志**：
```
========================================
  C++ AI Chat Server v0.3.0
========================================
AI Provider: 国家超算互联网
AI API URL: ...
✅ AI API Key 已配置
[Database] Initialized at ./chat.db
[Server] Listening on port 8080
Server is running on port 8080
```

---

## 📋 测试 4：功能验证

### 4.1 测试端口连通性

```bash
curl -v http://localhost:8080
```

**预期**：连接成功（WebSocket 握手）

### 4.2 进入容器测试

```bash
docker-compose exec chat-server /bin/bash

# 在容器内
ps aux | grep chat_server  # 应该看到进程
ls -la /app/data/          # 应该看到数据库文件
```

### 4.3 测试 WebSocket 连接

**方式 1：使用 ws_client（如果有）**
```bash
docker-compose exec chat-server ./ws_client
```

**方式 2：使用外部客户端**
```bash
# 在宿主机
./build/ws_client
# 连接到 localhost:8080
```

---

## 📋 测试 5：数据持久化

### 5.1 检查数据卷

```bash
docker volume ls | grep chat_data
```

**预期**：数据卷存在

### 5.2 查看数据卷详情

```bash
docker volume inspect cpp_ai_chat_data
```

**预期**：
```json
{
    "Driver": "local",
    "Mountpoint": "/var/lib/docker/volumes/cpp_ai_chat_data/_data",
    ...
}
```

### 5.3 验证数据持久化

```bash
# 1. 停止容器
docker-compose down

# 2. 重新启动
docker-compose up -d

# 3. 检查数据是否保留
docker-compose exec chat-server ls -la /app/data/
```

**预期**：数据库文件仍然存在

---

## 📋 测试 6：环境变量

### 6.1 检查环境变量

```bash
docker-compose exec chat-server env | grep AI_
```

**预期**：
```
AI_API_KEY=sk-...
AI_API_URL=https://...
AI_MODEL=MiniMax-M2.5
```

### 6.2 修改环境变量

```bash
# 编辑 .env 文件
nano .env

# 重启容器
docker-compose restart

# 验证
docker-compose exec chat-server env | grep AI_
```

**预期**：环境变量已更新

---

## 📋 测试 7：健康检查

### 7.1 查看健康状态

```bash
docker inspect --format='{{json .State.Health}}' cpp-ai-chat-server | jq
```

**预期**：
```json
{
    "Status": "healthy",
    "FailingStreak": 0,
    ...
}
```

### 7.2 查看健康检查日志

```bash
docker inspect --format='{{range .State.Health.Log}}{{.Output}}{{end}}' cpp-ai-chat-server
```

**预期**：健康检查通过

---

## 📋 测试 8：日志管理

### 8.1 查看实时日志

```bash
docker-compose logs -f chat-server
```

**预期**：实时显示日志

### 8.2 查看最近日志

```bash
docker-compose logs --tail=100 chat-server
```

**预期**：显示最近 100 行日志

### 8.3 验证日志轮转

```bash
# 查看日志文件
ls -la /var/lib/docker/containers/*/container.log
```

**预期**：日志文件大小合理（< 10MB）

---

## 📋 测试 9：网络隔离

### 9.1 查看网络配置

```bash
docker network inspect cpp_ai_chat_network
```

**预期**：
```json
{
    "Name": "cpp_ai_chat_network",
    "Driver": "bridge",
    ...
}
```

### 9.2 测试网络连通性

```bash
# 在容器内 ping 外部
docker-compose exec chat-server ping -c 3 api.scnet.cn
```

**预期**：可以访问外部网络

---

## 📋 测试 10：容器管理

### 10.1 停止容器

```bash
docker-compose down
```

**预期**：容器停止

### 10.2 重启容器

```bash
docker-compose restart
```

**预期**：容器重启成功

### 10.3 清理资源

```bash
# 停止并删除容器
docker-compose down

# 删除数据卷（谨慎使用）
docker-compose down -v
```

**预期**：资源清理成功

---

## 📊 测试结果汇总

### 通过的测试

- [ ] 测试 1：配置文件验证
- [ ] 测试 2：Docker 镜像构建
- [ ] 测试 3：容器启动
- [ ] 测试 4：功能验证
- [ ] 测试 5：数据持久化
- [ ] 测试 6：环境变量
- [ ] 测试 7：健康检查
- [ ] 测试 8：日志管理
- [ ] 测试 9：网络隔离
- [ ] 测试 10：容器管理

### 失败的项目

| 测试 | 问题描述 | 解决方案 |
|------|---------|---------|
|      |         |         |

### 总体评价

- [ ] 优秀（全部通过）
- [ ] 良好（1-2 个失败）
- [ ] 一般（3-5 个失败）
- [ ] 需改进（5 个以上失败）

---

_测试完成日期：_____________
_测试人员签名：_____________
