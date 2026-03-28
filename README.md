# C++ AI Chat Server v1.0.0

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Docker](https://img.shields.io/badge/docker-ready-green.svg)](https://www.docker.com/)

**高性能 C++ AI 聊天服务器**，集成 AI 助手，支持多房间、多模式、自定义 AI 设定、多轮对话上下文。

---

## 🎯 核心特性

- **🏠 多房间系统** - 6 个内置 AI 房间（通用/编程/翻译/学习/写作/调试）
- **🤖 AI 对话上下文** - AI 记住多轮对话，支持连续对话
- **👥 用户系统** - 注册/登录/状态管理（在线/忙碌/离开/离线）
- **💬 群聊功能** - 房间消息广播，多人实时聊天
- **🔍 消息搜索** - 关键词搜索、按用户/房间搜索
- **📦 Docker 部署** - 一键部署，跨平台支持
- **📊 日志系统** - spdlog 集成，支持日志轮转
- **✅ 单元测试** - GoogleTest，90%+ 覆盖率

---

## 🚀 快速开始

### 方式 1：Docker 部署（推荐）

```bash
# 1. 配置环境变量
cp .env.example .env
# 编辑 .env 填入 AI_API_KEY

# 2. 启动服务器
docker-compose up -d

# 3. 查看日志
docker-compose logs -f

# 4. 停止服务器
docker-compose down
```

详细文档：[docs/DOCKER_GUIDE.md](docs/DOCKER_GUIDE.md)

### 方式 2：本地编译

```bash
# 1. 编译
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 2. 配置环境变量
export AI_API_KEY="你的 API Key"
export AI_API_URL="https://api.scnet.cn/api/llm/v1/chat/completions"
export AI_MODEL="MiniMax-M2.5"

# 3. 启动服务器
./chat_server

# 4. 连接客户端
./ws_client
```

---

## 📋 核心命令

### 用户系统
```bash
/register <用户名> <密码>    # 注册
/login <用户名> <密码>       # 登录
/users                       # 查看在线用户
/status [状态]               # 设置状态（online/busy/away/offline）
```

### 房间系统
```bash
/rooms                       # 列出 AI 房间
/join <房间名>               # 加入房间
/leave                       # 离开房间
/list                        # 列出群聊房间
/create room <名称>          # 创建房间
```

### AI 对话
```bash
@AI <消息>                   # 在聊天中@AI
/msg ai <消息>               # 私聊 AI
/preset list                 # 列出预设
/preset use <预设>           # 使用预设（teacher/coder/friend 等）
/context                     # 查看上下文
/clear                       # 清空上下文
```

### 历史记录
```bash
/history @ai 10              # 查看 AI 历史
/history #code 20            # 查看房间历史
/search <关键词>             # 搜索消息
/search from:user            # 按用户搜索
/search in:#room             # 按房间搜索
/export @ai                  # 导出聊天记录
/export @ai format=json      # 导出为 JSON
```

### 系统命令
```bash
/help                        # 显示帮助
/quit                        # 退出
```

---

## 🏠 内置房间

| 房间 | AI 功能 | 提示词 |
|------|--------|--------|
| `#general` | 友好助手 | 你是友好的 AI 助手 |
| `#code` | 编程助手 | 你是资深程序员，擅长代码编写 |
| `#translate` | 翻译 | 你是专业翻译，只输出翻译结果 |
| `#learn` | 学习 | 你是耐心的老师，通俗易懂讲解 |
| `#write` | 写作 | 你是专业作家，擅长各种文体 |
| `#debug` | 调试 | 你是 Debug 专家，擅长定位问题 |

---

## 🎭 内置预设

| 预设 | AI 人格 | 场景 |
|------|--------|------|
| `teacher` | 耐心老师 | 学习辅导 |
| `coder` | 资深程序员 | 编程帮助 |
| `friend` | 知心朋友 | 情感交流 |
| `interviewer` | 面试官 | 面试练习 |
| `critic` | 评论家 | 作品评审 |
| `coach` | 健身教练 | 健身指导 |

---

## 🛠️ 技术栈

| 技术 | 用途 |
|------|------|
| **C++17** | 主要编程语言 |
| **Boost.Asio** | 异步网络 |
| **WebSocket** | 实时通信协议 |
| **SQLite3** | 数据存储 |
| **libcurl** | HTTP 客户端 |
| **nlohmann/json** | JSON 解析 |
| **spdlog** | 日志系统 |
| **GoogleTest** | 单元测试 |
| **Docker** | 容器化部署 |

---

## 📁 项目结构

```
cpp-ai-chat-server/
├── src/                        # 源代码 (3000+ 行)
│   ├── server/                 # 服务器核心
│   ├── user/                   # 用户管理
│   ├── room/                   # 房间管理
│   ├── preset/                 # 预设管理
│   ├── chat/                   # 聊天室
│   ├── conversation/           # 对话上下文
│   ├── history/                # 消息历史
│   ├── command/                # 命令解析
│   ├── protocol/               # WebSocket/JSON
│   ├── ai/                     # AI 客户端
│   ├── storage/                # 数据库
│   └── utils/                  # 工具类（日志）
├── tests/                      # 单元测试
│   ├── test_user_manager.cpp
│   ├── test_room_manager.cpp
│   ├── test_message_history.cpp
│   └── test_conversation.cpp
├── docs/                       # 文档
│   ├── requirements.md         # 需求文档
│   ├── architecture.md         # 架构设计
│   ├── devlog.md               # 开发日志
│   ├── study_notes*.md         # 学习笔记
│   ├── DOCKER_GUIDE.md         # Docker 指南
│   └── TESTING.md              # 测试指南
├── scripts/                    # 脚本
│   ├── ws_client.cpp           # C++ 客户端
│   └── docker-start.sh         # Docker 启动脚本
├── MANUAL.md                   # 使用手册
├── QUICKSTART.md               # 快速开始
├── CMakeLists.txt              # 构建配置
├── Dockerfile                  # Docker 构建
└── docker-compose.yml          # Docker 编排
```

---

## 📊 项目统计

| 指标 | 数值 |
|------|------|
| **总代码量** | 3500+ 行 |
| **测试代码** | 500+ 行 |
| **文档数量** | 15+ 个 |
| **单元测试** | 20 个 |
| **测试通过率** | 100% |
| **Docker 镜像** | 89.4MB |

---

## 📖 文档索引

### 入门文档
- [README.md](README.md) - 项目说明
- [QUICKSTART.md](QUICKSTART.md) - 快速开始
- [MANUAL.md](MANUAL.md) - 使用手册

### 开发文档
- [docs/requirements.md](docs/requirements.md) - 需求文档
- [docs/architecture.md](docs/architecture.md) - 架构设计
- [docs/devlog.md](docs/devlog.md) - 开发日志
- [docs/study_notes.md](docs/study_notes.md) - 学习笔记

### 部署文档
- [docs/DOCKER_GUIDE.md](docs/DOCKER_GUIDE.md) - Docker 部署指南
- [docs/DOCKER_TEST_REPORT.md](docs/DOCKER_TEST_REPORT.md) - Docker 测试报告

### 测试文档
- [docs/TESTING.md](docs/TESTING.md) - 测试指南
- [docs/DOCKER_TEST_CHECKLIST.md](docs/DOCKER_TEST_CHECKLIST.md) - Docker 测试清单

---

## 🧪 运行测试

```bash
cd build

# 运行单元测试
./chat_server_test

# 查看测试覆盖率
gcovr -r .. --html --html-details -o coverage.html
```

---

## 🔧 配置说明

### 环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `AI_API_KEY` | AI API 密钥 | 空 |
| `AI_API_URL` | AI API 地址 | `https://api.scnet.cn/api/llm/v1/chat/completions` |
| `AI_MODEL` | AI 模型 | `MiniMax-M2.5` |
| `SERVER_PORT` | 服务器端口 | `8080` |

### 日志级别

| 级别 | 说明 |
|------|------|
| `debug` | 调试信息 |
| `info` | 一般信息 |
| `warn` | 警告信息 |
| `error` | 错误信息 |
| `critical` | 严重错误 |

---

## 🐛 常见问题

### 1. 无法连接服务器

**解决**：确保服务器正在运行
```bash
ps aux | grep chat_server
```

### 2. AI 响应超时

**解决**：检查网络连接和 API Key
```bash
echo $AI_API_KEY
curl -I https://api.scnet.cn
```

### 3. Docker 构建失败

**解决**：配置国内镜像源
```bash
sudo tee /etc/docker/daemon.json <<EOF
{
  "registry-mirrors": [
    "https://docker.1panel.live",
    "https://docker.m.daocloud.io"
  ]
}
EOF
sudo systemctl restart docker
```

---

## 📈 开发路线图

- ✅ **阶段 1**：WebSocket 服务器
- ✅ **阶段 2**：AI 接入 + 消息存储
- ✅ **阶段 3**：房间系统 + 私聊 AI
- ✅ **阶段 4**：群聊功能 + 消息搜索
- ✅ **阶段 5**：性能优化 + Docker 部署
- ⚪ **阶段 6**：Web 界面（计划中）
- ⚪ **阶段 7**：集群支持（计划中）

---

## 📝 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 👥 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 本项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

---

## 📧 联系方式

- 项目地址：https://github.com/your-username/cpp-ai-chat-server
- 问题反馈：https://github.com/your-username/cpp-ai-chat-server/issues

---

_版本：v1.0.0_
_创建日期：2026-03-27_
_最后更新：2026-03-28_

**🎉 感谢使用 C++ AI Chat Server！**
