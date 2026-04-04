# Release Notes - v1.1.1

_发布日期：2026-04-04_

---

## 🐛 Bug 修复

### 离线消息推送修复

- **修复离线消息无法推送到前端的问题** - 在 Login 页面提前监听私聊消息，解决消息推送时序问题
- **修复前端消息类型判断错误** - `determineMessageType()` 优先使用服务器下发的 `type` 字段
- **修复用户名大小写不一致导致离线消息丢失** - SQL 查询添加 `COLLATE NOCASE`
- **修复私聊消息存储使用非标准用户名** - 发送私聊时使用数据库中的标准用户名

### 编译问题修复

- **修复 Logger 方法调用错误** - `connection.cpp` 中 `warning()` 改为 `warn()`
- **修复测试程序缺少 spdlog 链接** - CMakeLists.txt 添加 `spdlog::spdlog`

---

## 📊 版本对比

| 版本 | 发布日期 | 主要更新 |
|------|----------|----------|
| v1.1.1 | 2026-04-04 | 离线消息推送修复、编译问题修复 |
| v1.1.0 | 2026-04-03 | 好友系统与 Web 前端 |
| v1.0.0 | 2026-03-28 | 初始版本 |

---

## 📝 完整更新日志

### v1.1.1 (2026-04-04)

**修复**：
- 🐛 离线消息无法推送到前端（Login 页面提前监听私聊消息）
- 🐛 前端消息类型判断错误（优先使用服务器 `type` 字段）
- 🐛 用户名大小写不一致导致离线消息丢失（`COLLATE NOCASE`）
- 🐛 私聊消息存储使用非标准用户名（使用数据库标准用户名）
- 🐛 Logger 方法调用错误（`warning()` → `warn()`）
- 🐛 测试程序缺少 spdlog 链接

**新增**：
- ✨ `FriendManager::get_username_by_id()` 方法获取标准用户名

### v1.1.0 (2026-04-03)

**新增**：
- ✅ 好友系统（添加/删除/请求/接受/拒绝）
- ✅ 私聊功能（好友间一对一聊天）
- ✅ Web 前端（React + TypeScript）
- ✅ 会话管理（token 生成与验证）
- ✅ 用户邮箱字段支持
- ✅ Markdown 清理（AI 响应格式化）
- ✅ 预设 Prompt 增强（系统消息插入）
- ✅ 登录结果枚举（SUCCESS/USER_NOT_FOUND/WRONG_PASSWORD）

**优化**：
- ✅ 数据库表结构优化（索引优化）
- ✅ 好友关系双向查询
- ✅ 事务处理（好友请求接受）
- ✅ 日志记录增强

**删除**：
- ❌ 移除冗余文档文件

### v1.0.0 (2026-03-28)

**新增**：
- ✅ WebSocket 服务器
- ✅ AI 接入（支持国家超算互联网）
- ✅ 用户系统（注册/登录/状态）
- ✅ 房间系统（6 个内置房间）
- ✅ 预设系统（6 个内置人格）
- ✅ 对话上下文（多轮对话）
- ✅ 消息搜索（关键词/用户/房间）
- ✅ 历史记录（查看/导出）
- ✅ 群聊功能（消息广播）
- ✅ 日志系统（spdlog + 轮转）
- ✅ Docker 部署（多阶段构建）
- ✅ 单元测试（20 个测试）

---

# Release Notes - v1.1.0

_发布日期：2026-04-03_

---

## 🎉 好友系统与 Web 前端

我们很高兴地宣布 **C++ AI Chat Server v1.1.0** 正式发布！

此版本新增了完整的好友管理系统，包括好友请求、添加/删除好友、私聊功能，以及全新的 React Web 前端界面。

---

## ✨ 主要功能

### 新增功能

- **👫 好友系统** - 完整的好友管理功能（添加/删除/请求/接受/拒绝）
- **💬 私聊功能** - 好友间一对一私聊，支持离线消息
- **🌐 Web 前端** - React + TypeScript 现代化界面，类似微信/QQ 的交互体验
- **🔐 会话管理** - 用户会话 token 生成与验证，支持自动登录
- **📧 邮箱支持** - 用户表新增邮箱字段
- **✨ Markdown 清理** - AI 响应自动清理 Markdown 格式，提升可读性
- **📋 预设 Prompt 增强** - 预设 prompt 作为系统消息插入对话上下文

### 核心功能（v1.0.0 保留）

- **🏠 多房间系统** - 6 个内置 AI 房间（通用/编程/翻译/学习/写作/调试）
- **🤖 AI 对话上下文** - AI 记住多轮对话，支持连续对话
- **👥 用户系统** - 注册/登录/状态管理（在线/忙碌/离开/离线）
- **💬 群聊功能** - 房间消息广播，多人实时聊天
- **🔍 消息搜索** - 关键词搜索、按用户/房间搜索
- **📦 Docker 部署** - 一键部署，跨平台支持
- **📊 日志系统** - spdlog 集成，支持日志轮转
- **✅ 单元测试** - GoogleTest，90%+ 覆盖率

### 技术特性

- **高性能** - C++17 + Boost.Asio 异步网络
- **智能化** - AI 支持多轮对话上下文
- **易用性** - Docker 一键部署 + Web 界面
- **可靠性** - 100% 单元测试通过率
- **可维护** - 完整文档 + 日志系统
- **可扩展** - 模块化设计

---

## 📊 统计数据

| 指标 | 数值 |
|------|------|
| 总代码量 | 4000+ 行 |
| 测试代码 | 500+ 行 |
| 文档数量 | 10+ 个 |
| 单元测试 | 20 个 |
| 测试通过率 | 100% |
| Docker 镜像 | 89.4MB |

---

## 🛠️ 技术栈

### 后端
- **C++17** - 主要编程语言
- **Boost.Asio** - 异步网络
- **WebSocket** - 实时通信
- **SQLite3** - 数据存储
- **libcurl** - HTTP 客户端
- **nlohmann/json** - JSON 解析
- **spdlog** - 日志系统
- **GoogleTest** - 单元测试
- **Docker** - 容器化部署

### 前端
- **React 18** - UI 框架
- **TypeScript** - 类型安全
- **Vite 5** - 构建工具
- **Tailwind CSS** - 样式
- **Zustand** - 状态管理
- **React Router** - 路由

---

## 📦 安装

### Docker（推荐）

```bash
# 1. 配置环境变量
cp .env.example .env

# 2. 启动
docker-compose up -d

# 3. 查看状态
docker-compose ps
```

### 本地编译

```bash
# 1. 编译
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 2. 配置环境
export AI_API_KEY="your_key"

# 3. 启动
./chat_server
```

### Web 前端

```bash
cd frontend
npm install
npm run dev

# 访问 http://localhost:3000
```

---

## 📖 文档

- [README.md](README.md) - 项目说明
- [docs/DOCKER_GUIDE.md](docs/DOCKER_GUIDE.md) - Docker 部署指南
- [docs/devlog.md](docs/devlog.md) - 开发日志
- [docs/study_notes_phase5.2.md](docs/study_notes_phase5.2.md) - 阶段 5.2 学习笔记
- [docs/study_notes_phase5.3.md](docs/study_notes_phase5.3.md) - 阶段 5.3 学习笔记
- [docs/study_notes_phase5.4.md](docs/study_notes_phase5.4.md) - 阶段 5.4 学习笔记

---

## 🧪 测试

```bash
cd build

# 运行单元测试
./chat_server_test

# 客户端测试
./ws_client
```

---

## 🐛 已知问题

无已知问题。

---

## 📝 更新日志

### v1.1.0 (2026-04-03)

**新增**：
- ✅ 好友系统（添加/删除/请求/接受/拒绝）
- ✅ 私聊功能（好友间一对一聊天）
- ✅ Web 前端（React + TypeScript）
- ✅ 会话管理（token 生成与验证）
- ✅ 用户邮箱字段支持
- ✅ Markdown 清理（AI 响应格式化）
- ✅ 预设 Prompt 增强（系统消息插入）
- ✅ 登录结果枚举（SUCCESS/USER_NOT_FOUND/WRONG_PASSWORD）

**优化**：
- ✅ 数据库表结构优化（索引优化）
- ✅ 好友关系双向查询
- ✅ 事务处理（好友请求接受）
- ✅ 日志记录增强

**删除**：
- ❌ 移除冗余文档文件（CHANGELOG.md, MANUAL.md, QUICKSTART.md 等）

### v1.0.0 (2026-03-28)

**新增**：
- ✅ WebSocket 服务器
- ✅ AI 接入（支持国家超算互联网）
- ✅ 用户系统（注册/登录/状态）
- ✅ 房间系统（6 个内置房间）
- ✅ 预设系统（6 个内置人格）
- ✅ 对话上下文（多轮对话）
- ✅ 消息搜索（关键词/用户/房间）
- ✅ 历史记录（查看/导出）
- ✅ 群聊功能（消息广播）
- ✅ 日志系统（spdlog + 轮转）
- ✅ Docker 部署（多阶段构建）
- ✅ 单元测试（20 个测试）

**优化**：
- ✅ 异步 AI 调用（不阻塞）
- ✅ 内存优化（缓冲区预分配）
- ✅ 镜像优化（89.4MB）

---

## 🎯 路线图

### 已完成
- ✅ 阶段 1：WebSocket 服务器
- ✅ 阶段 2：AI 接入 + 消息存储
- ✅ 阶段 3：房间系统 + 私聊 AI
- ✅ 阶段 4：群聊功能 + 消息搜索
- ✅ 阶段 5：性能优化 + Docker 部署 + 日志系统
- ✅ 阶段 6：Web 界面 + 好友系统

### 计划中
- ⚪ 阶段 7：集群支持
- ⚪ 消息已读未读状态
- ⚪ 离线消息同步
- ⚪ 好友分组功能

---

## 📧 联系方式

- 项目地址：https://github.com/your-username/cpp-ai-chat-server
- 问题反馈：https://github.com/your-username/cpp-ai-chat-server/issues

---

## 📜 许可证

MIT License

---

**🎉 感谢使用 C++ AI Chat Server v1.1.0！**
