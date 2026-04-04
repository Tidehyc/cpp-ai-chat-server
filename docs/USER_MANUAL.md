# C++ AI Chat Server 操作说明书

_版本：v1.1.0_
_更新日期：2026-04-03_

---

## 📖 目录

1. [系统简介](#系统简介)
2. [快速开始](#快速开始)
3. [命令行客户端使用指南](#命令行客户端使用指南)
4. [Web 前端使用指南](#web-前端使用指南)
5. [管理员操作](#管理员操作)
6. [常见问题与故障排除](#常见问题与故障排除)
7. [附录](#附录)

---

## 系统简介

### 什么是 C++ AI Chat Server？

C++ AI Chat Server 是一个高性能的 AI 聊天服务器，采用 C++17 开发，集成 AI 助手，支持多房间、多模式、自定义 AI 设定、多轮对话上下文、好友系统与私聊功能。

### 核心功能

| 功能 | 说明 |
|------|------|
| 🏠 多房间系统 | 6 个内置 AI 房间（通用/编程/翻译/学习/写作/调试） |
| 🤖 AI 对话 | 支持多轮对话上下文，AI 能记住之前的对话 |
| 👥 用户系统 | 注册/登录/状态管理（在线/忙碌/离开/离线） |
| 💬 群聊功能 | 房间消息广播，多人实时聊天 |
| 👫 好友系统 | 好友请求/添加/删除，私聊功能 |
| 🔍 消息搜索 | 关键词搜索、按用户/房间搜索 |
| 🌐 Web 前端 | React + TypeScript 现代化界面 |

### 技术架构

```
┌─────────────────────────────────────────────────┐
│                  Web 前端 (React)                │
│              或 命令行客户端 (C++)               │
└─────────────────────┬───────────────────────────┘
                      │ WebSocket
┌─────────────────────▼───────────────────────────┐
│              C++ AI Chat Server                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
│  │ 用户管理 │  │ 房间管理 │  │  好友管理    │  │
│  └──────────┘  └──────────┘  └──────────────┘  │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
│  │ AI 客户端│  │ 消息历史 │  │  对话上下文  │  │
│  └──────────┘  └──────────┘  └──────────────┘  │
└─────────────────────┬───────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────┐
│              SQLite3 数据库                      │
│         (用户/消息/好友/上下文存储)              │
└─────────────────────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────┐
│              AI API (MiniMax)                    │
│    https://api.scnet.cn/api/llm/v1/...          │
└─────────────────────────────────────────────────┘
```

---

## 快速开始

### 方式 1：Docker 部署（推荐）

#### 步骤 1：配置环境变量

```bash
# 复制环境变量模板
cp .env.example .env

# 编辑 .env 文件，填入 AI API Key
nano .env
```

`.env` 文件内容：
```bash
AI_API_KEY=your_api_key_here
AI_API_URL=https://api.scnet.cn/api/llm/v1/chat/completions
AI_MODEL=MiniMax-M2.5
SERVER_PORT=8080
```

#### 步骤 2：启动服务器

```bash
# 启动服务
docker-compose up -d

# 查看运行状态
docker-compose ps

# 查看日志
docker-compose logs -f
```

#### 步骤 3：启动 Web 前端

```bash
cd frontend
npm install
npm run dev

# 访问 http://localhost:5173
```

### 方式 2：本地编译

#### 步骤 1：安装依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake libboost-all-dev libsqlite3-dev libcurl4-openssl-dev libssl-dev

# 安装 vcpkg（可选，用于依赖管理）
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
```

#### 步骤 2：编译项目

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

#### 步骤 3：配置并启动

```bash
# 设置环境变量
export AI_API_KEY="your_api_key_here"
export AI_API_URL="https://api.scnet.cn/api/llm/v1/chat/completions"
export AI_MODEL="MiniMax-M2.5"

# 启动服务器
./chat_server

# 启动客户端（另一个终端）
./ws_client
```

---

## 命令行客户端使用指南

### 连接服务器

```bash
# 启动客户端
./ws_client

# 连接到指定服务器
./ws_client localhost 8080
```

### 用户操作

#### 注册账号

```bash
# 格式：/register <用户名> <密码>
/register alice 123456

# 成功提示：
# 注册成功！欢迎 alice
```

**注意事项**：
- 用户名不能重复
- 密码长度 ≥ 6 位
- 注册后自动 登录

#### 登录账号

```bash
# 格式：/login <用户名> <密码>
/login alice 123456

# 成功提示：
# 登录成功！欢迎回来，alice
```

**登录结果说明**：
- `SUCCESS`：登录成功
- `USER_NOT_FOUND`：用户不存在
- `WRONG_PASSWORD`：密码错误

#### 设置状态

```bash
# 查看当前状态
/status

# 设置状态
/status online    # 在线（绿色）
/status busy      # 忙碌（红色）
/status away      # 离开（黄色）
/status offline   # 离线（灰色）
```

#### 查看在线用户

```bash
/users

# 输出示例：
# 在线用户：
# 🟢 alice (在线)
# 🔴 bob (忙碌)
# 🟡 charlie (离开)
```

### 房间操作

#### 列出 AI 房间

```bash
/rooms

# 输出示例：
# AI 房间：
# #general - 友好助手
# #code - 编程助手
# #translate - 翻译
# #learn - 学习
# #write - 写作
# #debug - 调试
```

#### 加入房间

```bash
# 格式：/join <房间名>
/join #code

# 成功提示：
# 已加入房间：#code
```

#### 离开房间

```bash
/leave

# 成功提示：
# 已离开房间
```

#### 列出群聊房间

```bash
/list

# 输出示例：
# 群聊房间：
# #general
# #code
# #gaming
# #music
# #movie
```

#### 创建房间

```bash
# 格式：/create room <房间名>
/create room my_room

# 成功提示：
# 房间创建成功：my_room
```

### AI 对话

#### @AI 触发

在房间聊天时，使用 `@AI` 前缀触发 AI 回复：

```bash
# 在房间发送消息
@AI 你好，请介绍一下自己

# AI 回复：
# AI: 你好！我是 AI 助手，很高兴为你服务...
```

#### 私聊 AI

```bash
# 格式：/msg ai <消息>
/msg ai 请帮我解释一下什么是递归

# AI 回复：
# AI: 递归是一种编程技巧...
```

#### 多轮对话

AI 会记住对话上下文，支持连续对话：

```bash
# 第一轮
@AI 什么是 C++？

# AI 回复后，继续提问（无需再次 @AI）
那它和 C 语言有什么区别？

# AI 会根据上下文回答
```

#### 查看上下文

```bash
# 查看当前上下文大小
/context

# 输出示例：
# 当前上下文：10 条消息
```

#### 清空上下文

```bash
/clear

# 输出示例：
# 对话上下文已清空
```

#### 预设管理

```bash
# 列出所有预设
/preset list

# 输出示例：
# 可用预设：
# teacher - 耐心老师
# coder - 资深程序员
# friend - 知心朋友
# interviewer - 面试官
# critic - 评论家
# coach - 健身教练

# 使用预设
/preset use coder

# 输出示例：
# 已切换预设：coder
```

### 好友系统

#### 发送好友请求

```bash
# 格式：/friend request <用户名>
/friend request bob

# 成功提示：
# 好友请求已发送给 bob
```

#### 查看收到的请求

```bash
/friend requests

# 输出示例：
# 待处理的好友请求：
# - bob
# - charlie
```

#### 接受/拒绝请求

```bash
# 接受好友请求
/friend accept bob

# 拒绝好友请求
/friend reject charlie

# 成功提示：
# 已接受 bob 的好友请求
# 已拒绝 charlie 的好友请求
```

#### 查看好友列表

```bash
/friend list

# 输出示例：
# 你的好友：
# 👤 bob (在线)
# 👤 charlie (离线)
```

#### 添加/删除好友

```bash
# 直接添加好友（如果对方也添加你）
/friend add bob

# 删除好友
/friend remove bob
```

#### 发送私聊消息

```bash
# 格式：/msg <用户名> <消息>
/msg bob 你好，最近怎么样？

# 成功提示：
# 私聊消息已发送给 bob
```

**注意事项**：
- 只能给好友发送私聊消息
- 非好友会收到拒绝提示

### 消息搜索与历史

#### 搜索消息

```bash
# 按关键词搜索
/search 递归

# 按用户搜索
/search from:alice

# 按房间搜索
/search in:#code

# 组合搜索
/search 递归 from:alice in:#code
```

#### 查看历史记录

```bash
# 查看 AI 对话历史
/history @ai 10

# 查看房间历史
/history #code 20

# 输出示例：
# [2026-04-03 10:30:15] alice: 什么是递归？
# [2026-04-03 10:30:16] AI: 递归是一种...
```

#### 导出聊天记录

```bash
# 导出 AI 对话记录
/export @ai

# 导出为 JSON 格式
/export @ai format=json

# 导出房间历史
/export #code

# 成功提示：
# 聊天记录已导出
```

### 系统命令

#### 查看帮助

```bash
/help

# 输出所有可用命令
```

#### 退出系统

```bash
/quit

# 或按 Ctrl+C
```

---

## Web 前端使用指南

### 访问前端

打开浏览器访问：`http://localhost:5173`

### 登录页面

#### 注册账号

1. 点击"注册"标签
2. 输入用户名（至少 3 个字符）
3. 输入密码（至少 6 个字符）
4. 点击"注册"按钮
5. 注册成功后自动跳转到聊天页面

#### 登录账号

1. 输入用户名
2. 输入密码
3. （可选）勾选"记住我"自动登录
4. 点击"登录"按钮

### 聊天页面

#### 界面布局

```
┌─────────────────────────────────────────────────┐
│ 💬 好友聊天 [➕]    |  💬 Tide [已连接] 👤 你  │
├─────────────────────┤  退出                    │
│ 📬 待处理 (1)       ├──────────────────────────┤
│  ├ bob             │  [聊天区域]              │
│  ├ ✓ 同意 ✕ 拒绝    │  消息列表...             │
├─────────────────────┤                          │
│ 🔍 搜索...          ├──────────────────────────┤
│                     │  [输入框] [发送]         │
│ 👤 Alice (在线)     └──────────────────────────┘
│ 👤 Bob (离线)       │
└─────────────────────┘
```

#### 好友聊天

1. **查看好友列表**
   - 左侧显示所有好友
   - 在线好友显示绿色圆点
   - 离线好友显示灰色圆点

2. **发送消息**
   - 点击好友名称打开聊天窗口
   - 在底部输入框输入消息
   - 点击"发送"按钮或按 Enter 键发送

3. **好友请求**
   - 有待处理请求时显示黄色提示区域
   - 点击"✓"接受请求
   - 点击"✕"拒绝请求

4. **添加好友**
   - 点击"➕"按钮
   - 输入要添加的用户名
   - 点击"发送请求"

#### 群聊房间

1. **查看房间列表**
   - 点击"群聊"标签
   - 左侧显示所有可用房间
   - 房间图标区分不同类型

2. **加入房间**
   - 点击房间名称自动加入
   - 聊天区域显示房间消息

3. **发送消息**
   - 在输入框输入消息
   - 消息会广播给房间内所有用户

4. **创建房间**
   - 点击"➕"按钮
   - 输入房间名称
   - 点击"创建"

#### 切换聊天模式

- **从群聊切换到好友**：自动离开当前房间
- **从好友切换到群聊**：自动加入所选房间

#### 登出操作

1. 点击右上角"退出"按钮
2. 确认退出对话框
3. 清除登录状态并返回登录页

### 设置页面

#### 个人信息

- 查看当前用户名
- 修改在线状态
- 查看会话信息

#### 主题设置

- 浅色模式
- 深色模式
- 跟随系统

#### 退出登录

- 点击"退出登录"按钮
- 确认后清除状态

---

## 管理员操作

### 服务器管理

#### 启动服务器

```bash
# 本地启动
./chat_server

# Docker 启动
docker-compose up -d
```

#### 停止服务器

```bash
# 本地停止
# 按 Ctrl+C 或发送 SIGTERM

# Docker 停止
docker-compose down
```

#### 查看日志

```bash
# 本地日志
tail -f logs/chat_server.log

# Docker 日志
docker-compose logs -f
```

### 数据库管理

#### 查看数据库

```bash
# 使用 sqlite3 命令行
sqlite3 data/chat.db

# 查看表
.tables

# 查询用户
SELECT * FROM users;

# 查询消息
SELECT * FROM messages LIMIT 10;
```

#### 清理数据

```bash
# 使用清理脚本
./scripts/clear_data.sh

# 或清理数据库
./scripts/clear_db.sh
```

### 性能监控

#### 查看进程状态

```bash
# 查看服务器进程
ps aux | grep chat_server

# 查看内存占用
ps -o pid,rss,vsz,comm -p $(pgrep chat_server)
```

#### 查看连接数

```bash
# 查看 WebSocket 连接数
netstat -an | grep 8080 | grep ESTABLISHED | wc -l
```

---

## 常见问题与故障排除

### 1. 无法连接服务器

**症状**：客户端连接失败

**解决方案**：
```bash
# 检查服务器是否运行
ps aux | grep chat_server

# 检查端口是否监听
netstat -tlnp | grep 8080

# 重启服务器
docker-compose restart
```

### 2. AI 响应超时

**症状**：发送 @AI 后长时间无响应

**解决方案**：
```bash
# 检查 API Key 配置
echo $AI_API_KEY

# 测试 API 连接
curl -I https://api.scnet.cn

# 检查网络连接
ping api.scnet.cn

# 查看服务器日志
docker-compose logs -f | grep -i ai
```

### 3. Docker 构建失败

**症状**：`docker-compose build` 失败

**解决方案**：
```bash
# 配置国内镜像源
sudo tee /etc/docker/daemon.json <<EOF
{
  "registry-mirrors": [
    "https://docker.1panel.live",
    "https://docker.m.daocloud.io"
  ]
}
EOF

# 重启 Docker
sudo systemctl restart docker

# 清理并重新构建
docker-compose down
docker system prune -a
docker-compose build
```

### 4. Web 前端无法连接后端

**症状**：前端显示连接失败

**解决方案**：
```bash
# 检查后端是否运行
curl http://localhost:8080

# 检查 CORS 配置
# 确保服务器允许前端域名

# 重启前端
cd frontend
npm run dev
```

### 5. 数据库锁定

**症状**：数据库操作失败，提示 "database is locked"

**解决方案**：
```bash
# 停止服务器
docker-compose down

# 检查数据库文件权限
ls -l data/chat.db

# 修复权限
chmod 666 data/chat.db

# 重启服务器
docker-compose up -d
```

### 6. 好友请求无法发送

**症状**：发送好友请求失败

**解决方案**：
```bash
# 检查用户是否存在
/users

# 检查是否已经是好友
/friend list

# 检查是否有待处理请求
/friend requests

# 查看服务器日志
docker-compose logs -f | grep -i friend
```

---

## 附录

### A. 命令速查表

| 命令 | 说明 | 示例 |
|------|------|------|
| `/register <用户> <密码>` | 注册账号 | `/register alice 123456` |
| `/login <用户> <密码>` | 登录账号 | `/login alice 123456` |
| `/status [状态]` | 设置状态 | `/status busy` |
| `/users` | 查看在线用户 | `/users` |
| `/rooms` | 列出 AI 房间 | `/rooms` |
| `/join <房间>` | 加入房间 | `/join #code` |
| `/leave` | 离开房间 | `/leave` |
| `/list` | 列出群聊房间 | `/list` |
| `/create room <名称>` | 创建房间 | `/create room my_room` |
| `@AI <消息>` | 触发 AI 回复 | `@AI 你好` |
| `/msg ai <消息>` | 私聊 AI | `/msg ai 解释递归` |
| `/msg <用户> <消息>` | 私聊用户 | `/msg bob 你好` |
| `/preset list` | 列出预设 | `/preset list` |
| `/preset use <预设>` | 使用预设 | `/preset use coder` |
| `/context` | 查看上下文 | `/context` |
| `/clear` | 清空上下文 | `/clear` |
| `/friend request <用户>` | 发送好友请求 | `/friend request bob` |
| `/friend requests` | 查看收到的请求 | `/friend requests` |
| `/friend accept <用户>` | 接受好友请求 | `/friend accept bob` |
| `/friend reject <用户>` | 拒绝好友请求 | `/friend reject bob` |
| `/friend list` | 查看好友列表 | `/friend list` |
| `/friend add <用户>` | 添加好友 | `/friend add bob` |
| `/friend remove <用户>` | 删除好友 | `/friend remove bob` |
| `/search <关键词>` | 搜索消息 | `/search 递归` |
| `/search from:<用户>` | 按用户搜索 | `/search from:alice` |
| `/search in:<房间>` | 按房间搜索 | `/search in:#code` |
| `/history @ai <数量>` | 查看 AI 历史 | `/history @ai 10` |
| `/history #<房间> <数量>` | 查看房间历史 | `/history #code 20` |
| `/export @ai` | 导出聊天记录 | `/export @ai` |
| `/export @ai format=json` | 导出为 JSON | `/export @ai format=json` |
| `/help` | 显示帮助 | `/help` |
| `/quit` | 退出系统 | `/quit` |

### B. 内置房间列表

| 房间名 | AI 功能 | 提示词 |
|--------|--------|--------|
| `#general` | 友好助手 | 你是友好的 AI 助手 |
| `#code` | 编程助手 | 你是资深程序员，擅长代码编写 |
| `#translate` | 翻译 | 你是专业翻译，只输出翻译结果 |
| `#learn` | 学习 | 你是耐心的老师，通俗易懂讲解 |
| `#write` | 写作 | 你是专业作家，擅长各种文体 |
| `#debug` | 调试 | 你是 Debug 专家，擅长定位问题 |

### C. 内置预设列表

| 预设名 | AI 人格 | 场景 |
|--------|--------|------|
| `teacher` | 耐心老师 | 学习辅导 |
| `coder` | 资深程序员 | 编程帮助 |
| `friend` | 知心朋友 | 情感交流 |
| `interviewer` | 面试官 | 面试练习 |
| `critic` | 评论家 | 作品评审 |
| `coach` | 健身教练 | 健身指导 |

### D. 环境变量说明

| 变量 | 说明 | 默认值 | 必填 |
|------|------|--------|------|
| `AI_API_KEY` | AI API 密钥 | 空 | ✅ |
| `AI_API_URL` | AI API 地址 | `https://api.scnet.cn/api/llm/v1/chat/completions` | ❌ |
| `AI_MODEL` | AI 模型 | `MiniMax-M2.5` | ❌ |
| `SERVER_PORT` | 服务器端口 | `8080` | ❌ |

### E. 日志级别说明

| 级别 | 说明 | 使用场景 |
|------|------|----------|
| `debug` | 调试信息 | 开发调试 |
| `info` | 一般信息 | 正常运行 |
| `warn` | 警告信息 | 潜在问题 |
| `error` | 错误信息 | 操作失败 |
| `critical` | 严重错误 | 系统故障 |

### F. 项目结构

```
cpp-ai-chat-server/
├── src/                        # 后端源代码
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
├── frontend/                   # Web 前端
│   ├── src/                    # React 源代码
│   └── package.json            # 依赖配置
├── docs/                       # 文档
├── scripts/                    # 脚本工具
├── CMakeLists.txt              # 构建配置
├── Dockerfile                  # Docker 构建
└── docker-compose.yml          # Docker 编排
```

---

## 📞 联系方式

- **项目地址**：https://github.com/tidehyc/cpp-ai-chat-server
- **问题反馈**：https://github.com/tidehyc/cpp-ai-chat-server/issues
- **文档**：查看 `docs/` 目录下的详细文档

---

## 📜 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

_版本：v1.1.0_
_更新日期：2026-04-03_

**🎉 感谢使用 C++ AI Chat Server！**
