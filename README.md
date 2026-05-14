# C++ AI Chat Server v1.1.4

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Docker](https://img.shields.io/badge/docker-ready-green.svg)](https://www.docker.com/)

**高性能 C++ AI 聊天服务器**，集成 AI 助手，支持 AI 预设、多轮对话上下文、好友系统与私聊功能、离线消息推送。

---

## 🆕 最近更新（v1.1.4）

- 完成提交前隐私扫描：移除测试脚本明文 API Key，统一走环境变量注入
- 完成大规模改动 PR 合并收尾：功能分支合并回 `main`，临时分支已清理
- 前端视觉体系与图标规范统一落地：新增图标规范文档并全站收敛
- 后端协议/存储与已读状态链路增强：序列化、迁移、事务投递与测试覆盖完善

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
- [docs/INDEX.md](docs/INDEX.md) - docs 总索引（推荐从这里进入）
- [docs/devlog.md](docs/devlog.md) - 开发日志
- [docs/PRODUCT_LAUNCH_PLAYBOOK.md](docs/PRODUCT_LAUNCH_PLAYBOOK.md) - 产品化分阶段上线路线与验收清单
- [docs/BACKEND_TODO_CHECKLIST.md](docs/BACKEND_TODO_CHECKLIST.md) - 后端代办清单（按优先级执行）
- [docs/CONTRIBUTING_BACKEND.md](docs/CONTRIBUTING_BACKEND.md) - 后端标准提交流程与本地自检
- [docs/PERFORMANCE_REPORT_TEMPLATE.md](docs/PERFORMANCE_REPORT_TEMPLATE.md) - 性能报告模板
- [docs/perf_reports/PERFORMANCE_BASELINE_20260425.md](docs/perf_reports/PERFORMANCE_BASELINE_20260425.md) - 性能基线记录
- [docs/perf_reports/perf_gate_changes/](docs/perf_reports/perf_gate_changes/) - 性能门禁阈值调整记录目录
- [docs/perf_reports/concurrent_connections_20260425.md](docs/perf_reports/concurrent_connections_20260425.md) - 并发压测记录
- [docs/perf_reports/concurrency_test_matrix_20260425.md](docs/perf_reports/concurrency_test_matrix_20260425.md) - 并发参数矩阵（日常回归 200 / 300 / 600）
- [docs/PERF_GATE_CHANGE_POLICY.md](docs/PERF_GATE_CHANGE_POLICY.md) - 性能门禁阈值治理策略
- [docs/RELEASE_DOC_CHECKLIST.md](docs/RELEASE_DOC_CHECKLIST.md) - 发布文档核对清单
- [docs/DB_BACKUP_RECOVERY_RUNBOOK.md](docs/DB_BACKUP_RECOVERY_RUNBOOK.md) - 数据库备份与恢复演练手册
- [scripts/verify_restore_db.sh](scripts/verify_restore_db.sh) - 数据库恢复后一键验证脚本
- [scripts/ci_persistence_test.py](scripts/ci_persistence_test.py) - Docker 卷持久化验证脚本
- [docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md](docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md) - 分支保护与必过检查配置
- [docs/BRANCH_PROTECTION_RELEASE_TEMPLATE.md](docs/BRANCH_PROTECTION_RELEASE_TEMPLATE.md) - release 分支保护模板
- [docs/STUDY_NOTES_INDEX.md](docs/STUDY_NOTES_INDEX.md) - 学习笔记总览

---

## 🧪 运行测试

```bash
cd build

# 运行单元测试
./chat_server_test

# 查看测试覆盖率
gcovr -r .. --html --html-details -o coverage.html
```

### 一键后端检查（构建 + CTest + 性能基准）

```bash
bash scripts/run_backend_checks.sh
```

性能报告模板见 [docs/PERFORMANCE_REPORT_TEMPLATE.md](docs/PERFORMANCE_REPORT_TEMPLATE.md)。

### 治理巡检（发布前自检）

```bash
bash scripts/audit_backend_governance.sh
```

该脚本会检查工作流、门禁策略、文档链接和 PR 模板是否齐全，输出可直接贴到 PR 评论的机器可读结果。

### 数据库备份与恢复

```bash
bash scripts/backup_db.sh
bash scripts/restore_db.sh
```

备份与恢复演练手册见 [docs/DB_BACKUP_RECOVERY_RUNBOOK.md](docs/DB_BACKUP_RECOVERY_RUNBOOK.md)。

### 性能历史汇总（趋势）

```bash
python3 scripts/summarize_perf_reports.py
```

生成文件：

- `docs/perf_reports/message_history_benchmark_summary.md`
- `docs/perf_reports/message_history_benchmark_summary.csv`
- `docs/perf_reports/concurrency_test_matrix_20260425.md`

并发回归口径：日常执行建议固定为 `200 / 300 / 600` 三档；`400 / 500` 仅作为已验证的探索档位保留。

说明：`scripts/run_backend_checks.sh` 已在基准执行后自动调用该汇总脚本；当前规则会在最新 speedup 低于历史均值的 80% 时输出 warning。

汇总摘要会同时记录 `PERF_WARN_RATIO`、`PERF_WARN_FAIL`、`gate_mode` 和 `gate_source`，便于和 GitHub Actions 的 Job Summary 对齐。

可选门禁（默认关闭，不影响本地日常检查）：

```bash
PERF_WARN_FAIL=1 PERF_WARN_RATIO=0.80 bash scripts/run_backend_checks.sh
```

- `PERF_WARN_FAIL=1`：当触发 warning 时返回非 0 退出码。
- `PERF_WARN_RATIO`：阈值比例，取值范围 `(0, 1]`。

阈值与门禁治理说明见 [docs/PERF_GATE_CHANGE_POLICY.md](docs/PERF_GATE_CHANGE_POLICY.md) 和 [docs/PERFORMANCE_REPORT_TEMPLATE.md](docs/PERFORMANCE_REPORT_TEMPLATE.md)。

### CI 自动检查（GitHub Actions）

- 工作流文件：`.github/workflows/backend-checks.yml`
- 触发时机：`push`、`pull_request`
- 执行内容：与本地脚本 `scripts/run_backend_checks.sh` 一致（构建 + CTest + 性能基准）
- 产物：自动上传 `docs/perf_reports/*.txt` 以及 `message_history_benchmark_summary.md/csv` 作为 workflow artifact
- 可见性：同时将 summary 输出到 Actions log 与 Job Summary 页面
- Job Summary 还会显示 `PERF_WARN_RATIO`、`PERF_WARN_FAIL`、`gate_mode`、`gate_source`
- 告警门禁策略：
  - `main` 分支（push）与目标为 `main` 的 PR：启用 `PERF_WARN_FAIL=1`（触发 warning 即失败）
  - 其他分支：默认 `PERF_WARN_FAIL=0`（仅告警，不阻断）

分支保护与 required checks 具体配置步骤见 [docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md](docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md)。

失败分类与退出码约定（`scripts/run_backend_checks.sh`）：

- `11` / `21`：`build`（配置或编译失败）
- `31`：`test`（CTest 失败）
- `41`：`perf`（基准或汇总执行失败）
- `51`：`perf-gate`（性能门禁触发）

CI 会在 Job Summary 输出 `category` 和 `next_action`，用于快速定位处理路径。

阈值变更治理：

- 阈值调整必须走 PR 评审，不允许直接修改默认门禁参数。
- 每次调整都需新增记录文件，模板见 `docs/perf_reports/PERF_GATE_CHANGE_RECORD_TEMPLATE.md`。
- 策略详情见 `docs/PERF_GATE_CHANGE_POLICY.md`。

发布文档核对：

- 每次发布前请先执行 `docs/RELEASE_DOC_CHECKLIST.md` 中的核对步骤。
- README、Release Notes、devlog 的版本/日期信息必须同步。
- 新增治理文档后，README 文档索引也要同步更新。

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

## 🙏 致谢

- **[ex-skill](https://github.com/mrxiaoge/ex-skill)** - AI 预设系统参考了该项目的 Layer 设计理念，实现了多角色 AI 人格（老师/程序员/作家等）

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

_版本：v1.1.2_
_创建日期：2026-03-27_
_最后更新：2026-04-23_

**🎉 感谢使用 C++ AI Chat Server！**
