# 快速参考卡片

_版本：v1.0.0_

---

## 🚀 快速启动

### Docker
```bash
docker-compose up -d
```

### 本地
```bash
mkdir -p build && cd build && cmake .. && make
export AI_API_KEY="your_key"
./chat_server
```

---

## 📋 常用命令

### 用户
```
/register <user> <pass>    # 注册
/login <user> <pass>       # 登录
/users                     # 查看用户
/status <status>           # 设置状态
```

### 房间
```
/rooms                     # AI 房间
/list                      # 群聊房间
/join <room>               # 加入
/leave                     # 离开
```

### AI
```
@AI <msg>                  # @AI
/msg ai <msg>              # 私聊
/preset use <name>         # 切换预设
/context                   # 查看上下文
```

### 历史
```
/history @ai 10            # AI 历史
/history #room 20          # 房间历史
/search <keyword>          # 搜索
/export @ai                # 导出
```

---

## 🔧 配置

### 环境变量
```bash
AI_API_KEY=sk-xxx
AI_API_URL=https://api.scnet.cn/api/llm/v1/chat/completions
AI_MODEL=MiniMax-M2.5
SERVER_PORT=8080
```

### Docker
```bash
docker-compose up -d      # 启动
docker-compose down       # 停止
docker-compose logs -f    # 日志
docker-compose ps         # 状态
```

---

## 📁 文件结构
```
cpp-ai-chat-server/
├── src/           # 源代码
├── tests/         # 测试
├── docs/          # 文档
├── scripts/       # 脚本
├── build/         # 编译输出
├── Dockerfile     # Docker 构建
└── docker-compose.yml
```

---

## 🧪 测试
```bash
cd build
./chat_server_test           # 单元测试
./ws_client                  # 客户端测试
```

---

## 📖 文档
- README.md - 项目说明
- MANUAL.md - 使用手册
- QUICKSTART.md - 快速开始
- docs/DOCKER_GUIDE.md - Docker 指南
- docs/PROJECT_SUMMARY.md - 项目总结

---

## 🐛 故障排除

### 无法连接
```bash
ps aux | grep chat_server
```

### AI 超时
```bash
echo $AI_API_KEY
curl -I https://api.scnet.cn
```

### Docker 问题
```bash
docker-compose logs
docker system prune
```

---

_最后更新：2026-03-28_
