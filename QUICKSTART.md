# 快速开始

## 🚀 5 分钟快速测试

### 1. 编译项目

```bash
cd /home/tide/workplace/cpp-ai-chat-server
mkdir -p build && cd build
cmake ..
make
```

### 2. 启动服务器

```bash
./chat_server
```

### 3. 启动测试客户端（新终端）

```bash
cd /home/tide/workplace/cpp-ai-chat-server/build
./ws_client
```

### 4. 发送消息

```
你好
@AI 你好，请介绍一下你自己
quit
```

---

## 🔑 配置 AI API Key

### 国家超算互联网（推荐，有免费额度）

1. **获取 API Key**
   - 访问：https://www.scnet.cn/
   - 注册并登录
   - 进入「Chat」→「模型 API」
   - 创建/查看 API Key

2. **配置环境变量**
   ```bash
   export AI_API_KEY="你的 API Key"
   export AI_API_URL="https://api.scnet.cn/api/llm/v1/chat/completions"
   export AI_MODEL="MiniMax-M2.5"  # 或其他可用模型
   ```

3. **重启服务器**
   ```bash
   ./chat_server
   ```

4. **测试 AI 对话**
   ```bash
   ./ws_client
   # 输入：@AI 你好
   ```

### 可用模型

| 模型 | 说明 |
|------|------|
| MiniMax-M2.5 | MiniMax 最新模型 |
| DeepSeek-V3.2 | 深度求索 |
| Qwen3-30B-A3B | 通义千问 |
| Qwen3-235B-A22B | 通义千问大模型 |
| DeepSeek-R1-0528 | 深度求索推理 |

---

## 📋 测试命令

| 命令 | 说明 |
|------|------|
| `你好` | 发送普通消息 |
| `@AI xxx` | 发送消息给 AI |
| `ping` | 测试连接 |
| `quit` | 退出客户端 |

---

## 🛠️ 常见问题

### 1. 端口被占用

```bash
# 使用其他端口
./chat_server 9000
```

### 2. 找不到 ws_client

```bash
# 确认编译成功
ls -la build/ws_client
```

### 3. AI 回复模拟消息

检查 API Key 是否正确配置：
```bash
echo $AI_API_KEY
```

### 4. Model Not Exist 错误

检查模型名称是否正确，可用模型：
- `MiniMax-M2.5`
- `DeepSeek-V3.2`
- `Qwen3-30B-A3B`

---

_版本：v0.2.0_
_最后更新：2026-03-27_
