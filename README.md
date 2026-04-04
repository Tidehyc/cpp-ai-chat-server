# C++ AI Chat Server v1.1.1

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Docker](https://img.shields.io/badge/docker-ready-green.svg)](https://www.docker.com/)

**高性能 C++ AI 聊天服务器**，集成 AI 助手，支持 AI 预设、多轮对话上下文、好友系统与私聊功能、离线消息推送。

---

## 🎯 核心特性

- **🎭 AI 预设系统** - 多种内置 AI 人格（老师/程序员/作家/专家等），随时切换
- **🤖 AI 对话上下文** - AI 记住多轮对话，支持连续对话
- **👥 用户系统** - 注册/登录/状态管理（在线/忙碌/离开/离线）
- **👫 好友系统** - 好友请求/添加/删除，私聊功能
- **💬 私聊功能** - 好友间一对一私聊，支持离线消息推送
- **🏠 群聊房间** - 多人聊天室，支持创建自定义房间
- **🔍 消息搜索** - 关键词搜索、按用户/房间搜索
- **📦 Docker 部署** - 一键部署，跨平台支持
- **📊 日志系统** - spdlog 集成，支持日志轮转
- **✅ 单元测试** - GoogleTest，100% 通过率
- **🌐 Web 前端** - React + TypeScript 现代化界面

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

### 好友系统
```bash
/friend add <用户名>         # 添加好友
/friend remove <用户名>      # 删除好友
/friend list                 # 查看好友列表
/friend request <用户名>     # 发送好友请求
/friend requests             # 查看收到的请求
/friend accept <用户名>      # 接受好友请求
/friend reject <用户名>      # 拒绝好友请求
/msg <用户名> <消息>         # 发送私聊消息
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

## 🏠 群聊房间

| 房间 | 用途 |
|------|------|
| `#general` | 综合聊天 |
| `#code` | 编程交流 |
| `#translate` | 翻译练习 |
| `#learn` | 学习交流 |
| `#write` | 写作交流 |
| `#debug` | 调试讨论 |

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
│   ├── friend/                 # 好友管理
│   └── utils/                  # 工具类（日志）
├── tests/                      # 单元测试
│   ├── test_user_manager.cpp
│   ├── test_room_manager.cpp
│   ├── test_message_history.cpp
│   └── test_conversation.cpp
├── docs/                       # 文档
│   ├── devlog.md               # 开发日志
│   ├── study_notes*.md         # 学习笔记
│   └── DOCKER_GUIDE.md         # Docker 指南
├── frontend/                   # Web 前端
│   ├── src/                    # React 源代码
│   └── package.json            # 依赖配置
├── scripts/                    # 脚本
│   ├── ws_client.cpp           # C++ 客户端
│   └── docker-start.sh         # Docker 启动脚本
├── CMakeLists.txt              # 构建配置
├── Dockerfile                  # Docker 构建
└── docker-compose.yml          # Docker 编排
```

---

## 📊 项目统计

| 指标 | 数值 |
|------|------|
| **总代码量** | 4000+ 行 |
| **测试代码** | 500+ 行 |
| **文档数量** | 10+ 个 |
| **单元测试** | 20 个 |
| **测试通过率** | 100% |
| **Docker 镜像** | 89.4MB |

---

## 📖 文档索引

### 入门文档
- [README.md](README.md) - 项目说明
- [docs/DOCKER_GUIDE.md](docs/DOCKER_GUIDE.md) - Docker 部署指南

### 开发文档
- [docs/devlog.md](docs/devlog.md) - 开发日志
- [docs/study_notes.md](docs/study_notes.md) - 学习笔记
- [docs/study_notes_phase5.2.md](docs/study_notes_phase5.2.md) - 阶段 5.2 学习笔记
- [docs/study_notes_phase5.3.md](docs/study_notes_phase5.3.md) - 阶段 5.3 学习笔记
- [docs/study_notes_phase5.4.md](docs/study_notes_phase5.4.md) - 阶段 5.4 学习笔记

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
- ✅ **阶段 6**：Web 界面（已完成）
  - ✅ 阶段 6.1：基础聊天功能
  - ✅ 阶段 6.2：前端功能完善
  - ✅ 阶段 6.3：好友系统与私聊界面
- ⚪ **阶段 7**：集群支持（计划中）

---

## 🌐 Web 前端

### 快速启动

```bash
# 前端
cd frontend
npm install
npm run dev

# 访问 http://localhost:5173
```

### 功能特性

- 💬 实时聊天 - WebSocket 双向通信
- 🏠 多房间支持 - 6 个 AI 房间隔离
- 🤖 AI 对话 - @AI 触发 AI 回复
- 👤 用户系统 - 登录/注册/自动登录
- 📱 响应式设计 - 类似微信/QQ 的界面
- 🎨 深色主题 - 浅色/深色/系统三种模式
- 👥 联系人 - 在线用户列表
- ⚙️ 设置 - 个人信息/主题切换

### 技术栈

| 技术 | 用途 |
|------|------|
| **React 18** | UI 框架 |
| **TypeScript** | 类型安全 |
| **Vite 5** | 构建工具 |
| **Tailwind CSS** | 样式 |
| **Zustand** | 状态管理 |
| **React Router** | 路由 |

### 已完成页面

| 页面 | 功能 | 状态 |
|------|------|------|
| 登录页 | 注册/登录/自动登录 | ✅ |
| 聊天页 | 消息收发/@AI/房间切换 | ✅ |
| 联系人 | 在线用户列表/状态分组 | ✅ |
| AI 助手 | AI 房间/AI 预设 | ✅ |
| 设置 | 个人信息/主题切换/退出 | ✅ |

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

_版本：v1.1.1_
_创建日期：2026-03-27_
_最后更新：2026-04-04_

**🎉 感谢使用 C++ AI Chat Server！**
