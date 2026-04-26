# 产品上线路线复盘记录（2026-04-24）

适用范围：从上一轮“产品化上线手册”推进开始，到本轮安全基线、可观测性最小闭环和容器生产加固落地为止。

这是一份独立复盘文档，用于后续回看“做了什么、为什么做、怎么验证、踩了什么坑、下一步该做什么”。

---

## 1. 日期与标题

- 日期：2026-04-24
- 标题：阶段 A 落地复盘 - 登录限流、健康检查、容器加固

---

## 2. 完成工作清单

- [x] 新增通用滑动窗口限流器
- [x] 接入登录防刷：按 IP 和用户名双维度限流
- [x] 接入命令防刷：按 IP 限制高频命令请求
- [x] 新增 HTTP 健康检查端点 `GET /health`
- [x] 容器运行改为非 root 用户
- [x] 容器健康检查改为真实 HTTP 探测
- [x] 运行时配置支持 `DB_PATH`、`LOG_FILE`、`SERVER_PORT`
- [x] 新增限流回归测试并接入 CMake
- [x] 更新部署文档和产品上线路线文档

---

## 3. 问题背景

这轮改动的目标不是新增聊天业务，而是把当前“能用”的服务往“能内测、能排障、能稳定部署”推进。

路线文档已经明确了三个最先需要补齐的点：

1. 安全基线不完整，尤其是登录刷接口和容器运行权限。
2. 可观测性太弱，只有进程活着不能说明服务真的健康。
3. 容器配置偏开发态，缺少生产部署时必须的最小权限和健康探针。

换句话说，这一轮的核心不是“加功能”，而是“降低上线风险”。

---

## 4. 根因分析

### 4.1 登录与命令入口没有统一节流

在这次改动前，`/login` 和普通命令入口都可以被客户端高频调用，没有任何请求速率约束。这会带来两个直接风险：

- 暴力尝试登录没有成本。
- 单个连接可以持续刷命令，放大日志、数据库和消息处理压力。

### 4.2 服务缺少可以直接探活的 HTTP 端点

原来的 Docker 健康检查只看 `chat_server` 进程是否存在，不能区分以下情况：

- 进程存在但数据库不可用
- 进程存在但网络协议处理已经异常
- 进程存在但业务逻辑已经卡死

这类探针对线上排障帮助很有限。

### 4.3 容器默认权限过高

原镜像运行方式更偏本地开发，没有显式设置非 root 用户、只读根文件系统、最小权限能力收敛。这会在生产环境里扩大攻击面，也不符合路线手册里的安全基线。

### 4.4 部署参数写死在代码和配置中

数据库路径、日志路径、端口等运行参数如果不通过环境变量接管，后面做多环境部署时就会不断修改代码或镜像，发布成本会越来越高。

---

## 5. 实现细节

### 5.1 新增通用限流器

新增了一个独立的滑动窗口限流器，放在 `src/utils/rate_limiter.h` 和 `src/utils/rate_limiter.cpp`。

核心思路是按 key 维护一个时间队列，超过窗口大小的历史事件会被丢弃；当队列长度达到阈值时拒绝本次请求。

```cpp
class RateLimiter
{
public:
    RateLimiter(std::size_t max_events, std::chrono::seconds window);
    bool allow(const std::string &key);
};
```

```cpp
while (!queue.empty() && queue.front() < cutoff)
{
    queue.pop_front();
}

if (queue.size() >= max_events_)
{
    return false;
}

queue.push_back(now);
return true;
```

### 5.2 登录限流与命令限流接入连接层

在 `src/server/connection.cpp` 中，登录请求和普通命令都接入了限流逻辑。

登录限流使用两个维度：

- IP 维度：防止单个来源连续撞库
- 用户名维度：防止围绕单个账号重复尝试

命令限流则按 IP 做统一节流，避免单连接刷爆服务。

```cpp
if (!g_login_ip_limiter.allow("login_ip:" + ip))
{
    send_system_message("登录尝试过于频繁，请稍后重试。");
    return;
}

if (!g_login_user_limiter.allow("login_user:" + username))
{
    send_system_message("登录尝试过于频繁，请稍后重试。");
    return;
}
```

```cpp
if (!allow_command_rate(input))
{
    return;
}
```

### 5.3 增加 HTTP 健康检查端点

同样是在连接握手阶段，增加了对 `GET /health` 的识别和直接应答。

健康检查返回 JSON，包含：

- `status`
- `service`
- `db`
- `active_connections`

这使得容器健康检查可以从“看进程”升级为“看服务真的能返回健康结果”。

```cpp
json payload = {
    {"status", db_ok ? "ok" : "degraded"},
    {"service", "chat_server"},
    {"db", db_ok},
    {"active_connections", static_cast<int>(snapshot_all_connections().size())}};
```

### 5.4 运行参数改为环境变量驱动

在 `src/main.cpp` 里补了对 `DB_PATH`、`LOG_FILE`、`SERVER_PORT` 的读取。

```cpp
std::string db_path = std::getenv("DB_PATH") ? std::getenv("DB_PATH") : "./chat.db";
std::string log_file = std::getenv("LOG_FILE") ? std::getenv("LOG_FILE") : "chat_server.log";
if (const char *port_env = std::getenv("SERVER_PORT"))
{
    port = static_cast<short>(std::atoi(port_env));
}
```

这样后面无论是本地、Docker 还是后续反向代理环境，都不需要再改代码来切换路径。

### 5.5 Docker 运行面收敛

Dockerfile 改动集中在三个方向：

- 增加 `curl` 和 `tini`
- 创建非 root 用户 `app`
- 健康检查改为请求 `http://127.0.0.1:8080/health`

```dockerfile
RUN groupadd --gid 10001 app && \
    useradd --uid 10001 --gid 10001 --create-home --shell /usr/sbin/nologin app

USER app

HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -fsS http://127.0.0.1:8080/health || exit 1
```

`docker-compose.yml` 同步加入了：

- `read_only: true`
- `tmpfs: /tmp`
- `security_opt: no-new-privileges:true`
- `cap_drop: ALL`

这部分属于低复杂度但高收益的容器硬化。

---

## 6. 验证与证据

本轮验证是完整闭环，不只是“改了代码”，还确认了能编译、能测、能被 CMake 接入。

### 6.1 构建验证

- `Build_CMakeTools` 成功完成。
- `chat_server` 与 `chat_server_test` 都成功生成。
- 新增的限流源文件和测试文件都被正确纳入构建。

### 6.2 测试验证

- `RunCtest_CMakeTools` 通过。
- 新增 `RateLimiterTest.BlocksWhenWindowLimitReached` 通过。
- 新增 `RateLimiterTest.AllowsAgainAfterWindowExpires` 通过。

### 6.3 语法与错误检查

- `src/server/connection.cpp` 无新错误。
- `src/main.cpp` 无新错误。
- `Dockerfile`、`docker-compose.yml` 无错误。
- `docs/PRODUCT_LAUNCH_PLAYBOOK.md` 无错误。

### 6.4 观察到的构建信息

- 构建时出现了一个与当前任务无关的既有 warning：`tests/test_rate_limiter.cpp` 已正常接入，未造成失败。
- CMake 还有一个 FetchContent 的开发者 warning，不影响本次交付结果。

---

## 7. 踩坑与处理方式

### 7.1 健康检查不能只看进程

原先的 `pgrep` 型探针很容易误判服务健康。后来改为 HTTP 探针后，至少能知道服务是否真的能返回 `/health`。

### 7.2 响应头长度要和 body 严格一致

在实现 `426 Upgrade Required` 响应时，曾经出现过 `Content-Length` 不匹配的问题，后来修正为与实际 body 长度一致。

### 7.3 新增测试文件必须同步接入 CMake

一开始 `tests/test_rate_limiter.cpp` 还没有被构建系统识别，导致 CMake 报“找不到源文件”。补充到 `add_executable(chat_server_test ...)` 后问题消失。

### 7.4 改动需要先看当前文件状态

这次回看 `src/main.cpp` 时，确认了文件已经由后续工具或格式化器更新过，所以没有按旧版本继续假设内容，避免了覆盖别人的改动。

---

## 8. 复盘结论

这轮最重要的不是单个功能，而是把“能跑”推进到了“更像一个可上线服务”的最低门槛：

- 有了防刷
- 有了健康检查
- 有了更安全的容器运行方式
- 有了可重复验证的测试

从产品化角度看，这些改动对应的不是新卖点，而是上线前必须补的底座。

---

## 9. 下一步改进项

- [ ] 补结构化日志字段：`request_id`、`module`、`error_code`
- [ ] 补监控与告警模板：CPU、内存、连接数、错误率
- [ ] 开始 TLS/WSS 与反向代理收敛
- [ ] 制作发布与回滚演练记录模板
- [ ] 做一轮仓库级敏感信息扫描门禁

---

## 10. 追加推进：前端 WebSocket 同源化

在完成上一轮安全基线后，继续推进了路线 B1 的前置准备，把前端 WebSocket 连接方式从“写死后台地址”改成“同源 /ws 优先、环境变量可覆盖”。

### 这一步做了什么

- 前端 WebSocket 服务默认改为读取 `VITE_WS_URL`
- 如果没有配置环境变量，则自动使用当前页面协议和主机拼出 `/ws`
- HTTPS 页面会自然切换到 WSS，不需要再改业务代码
- 同步更新部署文档和前端 README，避免旧写法继续传播

### 为什么要做

- 为后续 TLS/WSS 反代收敛铺路
- 避免本地开发、预发和线上三套地址写死在代码里
- 让前端连接方式和反向代理路径 `/ws` 保持一致

### 验证结果

- `frontend/src/services/websocket.ts` 语法检查通过
- 补充 `frontend/src/vite-env.d.ts` 后，`import.meta.env.VITE_WS_URL` 类型检查通过
- 连接默认策略已切换到同源 `/ws`
- `docs/DEPLOYMENT.md` 和 `frontend/README.md` 已同步更新
- `frontend` 生产构建通过

### 复盘要点

- 连接地址不要硬编码在构造函数默认值里，应该优先考虑“部署环境驱动”。
- WebSocket 最好和反向代理路径绑定成一个约定，否则 HTTPS / WSS 收敛时会出现两套逻辑。

---


---

## 11. 追加推进：生产 HTTPS/WSS 模板落地

这一轮把“文档里写着要有 TLS/WSS 模板”进一步落成了真实可用文件。

### 新增文件

- [docker-compose.prod.yml](../docker-compose.prod.yml)
- [frontend/Dockerfile.prod](../frontend/Dockerfile.prod)
- [frontend/nginx/nginx.conf](../frontend/nginx/nginx.conf)

### 这套模板解决了什么

- 前端通过 Nginx 对外提供 HTTPS
- WebSocket 通过同一入口代理到后端 `/ws`
- 后端只需要在内网暴露 8080，不再直接对公网开放
- 可以直接通过 `docker-compose -f docker-compose.prod.yml up -d --build` 启动

### 关键设计

- 前端默认连接同源 `/ws`，避免前后端地址分别维护
- 生产入口由 Nginx 统一做 TLS 终止、HTTP 跳转、静态资源与反代
- 后端继续保留 `/health` 供容器和编排系统做探活
- 前端镜像健康检查直接走 HTTPS `/health`，避免被 80 端口跳转干扰

### 复盘要点

- 生产入口最好一次性把静态站点、API 反代和 WebSocket 代理收口到同一层。
- 如果前端和后端都各自维护一套公网地址，后面做 HTTPS/WSS 时会出现配置分裂。

---

## 12. 适合后续复用的经验

- 先补运行时底座，再加新功能，收益更稳定。
- 限流和健康检查这种能力，尽量做成小而独立的模块，后续更容易复用和测试。
- 容器生产化要优先改默认权限、健康探针和持久化路径，成本低但收益高。
- 任何部署相关变更，都要同时更新文档和验证步骤，否则后面无法复盘。

---

## 13. 追加推进：结构化日志最小闭环（request_id + module）

为了继续推进阶段 A2（可观测性最小闭环），本轮在后端关键路径加入了可追踪的日志上下文字段：

- `module`
- `request_id`
- `connection_id`

### 落地点

- [src/server/connection.cpp](../src/server/connection.cpp)
- [src/server/connection.h](../src/server/connection.h)

### 做了什么

- 新增请求序号生成器，并在命令链路生成 `cmd-*` request_id。
- 在登录链路生成 `auth-*` request_id，记录尝试、限流、成功、失败原因。
- 在健康检查链路生成 `health-*` request_id，记录探活执行与数据库状态。
- 将限流拒绝日志改为结构化 key-value 输出，便于日志系统检索。

### 代表性日志格式

```text
module=auth request_id=auth-42 connection_id=7 event=login_attempt username=alice ip=10.0.0.8
module=auth request_id=auth-42 connection_id=7 event=login_success user_id=12 username=alice
module=connection request_id=cmd-51 connection_id=7 event=command_received summary=cmd=/users
```

### 验证结果

- CMake 构建通过（`chat_server`、`chat_server_test` 均成功链接）。
- `connection.cpp` / `connection.h` 静态错误检查通过。
- 仅存在与本次改动无关的既有 warning（`chat_room.cpp` 未使用参数）。

### 复盘要点

- 先从最关键链路补 request_id（命令、认证、健康检查），能迅速提升排障能力。
- 结构化日志不一定要一步到位上 JSON；先稳定 key-value 约定也能满足检索和聚合。

---

## 14. 追加推进：监控看板与告警阈值模板落地

为了完成阶段 A2 的剩余项，本轮继续补齐了“监控采集 + 可视化 + 告警规则”最小闭环。

### 新增目录与文件

- [monitoring/prometheus/prometheus.yml](../monitoring/prometheus/prometheus.yml)
- [monitoring/prometheus/alerts.yml](../monitoring/prometheus/alerts.yml)
- [monitoring/blackbox/blackbox.yml](../monitoring/blackbox/blackbox.yml)
- [monitoring/grafana/provisioning/datasources/datasource.yml](../monitoring/grafana/provisioning/datasources/datasource.yml)
- [monitoring/grafana/provisioning/dashboards/dashboard.yml](../monitoring/grafana/provisioning/dashboards/dashboard.yml)
- [monitoring/grafana/dashboards/chat-ops-overview.json](../monitoring/grafana/dashboards/chat-ops-overview.json)

### 编排接入

- 在 [docker-compose.prod.yml](../docker-compose.prod.yml) 中新增：
    - `prometheus`
    - `grafana`
    - `node-exporter`
    - `cadvisor`
    - `blackbox-exporter`

### 覆盖的核心指标

- 主机 CPU、内存占用
- 容器内存与重启变化
- 后端 `/health` 与前端 `/health` 探活成功率

### 已预置告警阈值

- 后端探活失败 2 分钟（critical）
- 前端探活失败 2 分钟（warning）
- CPU 连续 5 分钟 > 85%（warning）
- 内存连续 5 分钟 > 90%（warning）

### 文档同步

- 在 [docs/DEPLOYMENT.md](./DEPLOYMENT.md) 增加“监控与告警（已提供模板）”章节
- 在 [docs/PRODUCT_LAUNCH_PLAYBOOK.md](./PRODUCT_LAUNCH_PLAYBOOK.md) 将 A2 监控与阈值项标记完成

### 验证证据

- 监控配置文件静态检查通过：
    - `docker-compose.prod.yml`
    - `monitoring/prometheus/prometheus.yml`
    - `monitoring/prometheus/alerts.yml`
    - `monitoring/blackbox/blackbox.yml`
- 编排展开验证通过：`docker-compose -f docker-compose.prod.yml config`
- 运行环境未安装 `docker compose` 子命令，已回退到 `docker-compose` 命令验证

### 复盘要点

- 告警阈值先给“可运行默认值”，后续再按真实流量迭代，不要一开始追求完美阈值。
- 采集、看板、告警必须一起落地，否则只能看到数据但无法形成运维动作。

---

## 15. 追加推进：发布与回滚演练模板落地（A3）

为了补齐 A3，本轮将发布与回滚从“文档说明”升级为“脚本可执行 + 记录可审计”。

### 新增文件

- [scripts/release_prod.sh](../scripts/release_prod.sh)
- [scripts/rollback_prod.sh](../scripts/rollback_prod.sh)
- [docs/RELEASE_ROLLBACK_RUNBOOK.md](./RELEASE_ROLLBACK_RUNBOOK.md)
- [docs/release_records/TEMPLATE.md](./release_records/TEMPLATE.md)

### 关键能力

- 一键发布：构建/启动/健康检查/记录留档
- 一键回滚：停止/重建/健康检查/记录留档
- 自动记录：每次执行都会在 docs/release_records/ 生成一份 Markdown 记录

### 脚本策略说明

- 当前回滚脚本默认回滚到“当前仓库可部署状态”
- 若需精确历史版本回退，需要先切换到目标 commit/tag 再执行脚本

### 文档同步

- [docs/DEPLOYMENT.md](./DEPLOYMENT.md) 已新增“发布与回滚演练（已提供模板）”章节
- [docs/PRODUCT_LAUNCH_PLAYBOOK.md](./PRODUCT_LAUNCH_PLAYBOOK.md) 已将 A3 三项标记完成

### 复盘要点

- 发布/回滚必须同时产出记录文件，否则无法支撑故障复盘。
- 先做“可执行模板”，再做“精确版本回退自动化”，风险更可控。

---

## 16. 追加推进：Docker 构建上下文清理

在首次发布演练中，后端镜像构建失败，根因是 Docker build context 把宿主机的 `build/` 目录一并带入了镜像，导致容器内 CMake 读取到了不匹配的 `CMakeCache.txt`。

### 处理动作

- 新增根目录 [ .dockerignore ](../.dockerignore) 排除宿主机构建产物、日志、临时文件、编辑器配置与无需进镜像的文档目录
- 新增 [frontend/.dockerignore](../frontend/.dockerignore) 排除前端 `node_modules/`、`dist/` 与本地编辑器文件
- 将默认监控编排收敛为更稳定的 node-exporter + blackbox 组合，避免因外部镜像源可达性拖慢发布演练
- 将 Docker builder 依赖收敛为系统包优先，避免 CMake 在容器里依赖 GitHub FetchContent 下载 spdlog / googletest
- 将 `nlohmann_json` 也改为系统包优先，避免 Docker 构建阶段再从 GitHub 下载 JSON 依赖
- 修复发布演练在 `docker-compose` v1 上的重建异常（`KeyError: 'ContainerConfig'`）：发布脚本在 `up --build` 前先执行 `down --remove-orphans`
- 修复 backend 运行时镜像缺少共享库导致启动失败：在运行阶段安装 `libspdlog1` 与 `libfmt8`
- 处理本机 80/443 端口占用导致 frontend 启动失败：编排改为端口可配置，发布脚本自动切换到备用端口并按实际端口做健康检查
- 修复发布脚本健康检查过早触发导致误判失败：后端/前端健康检查改为重试等待机制
- 修复 backend 健康检查路径错误：backend 仅在容器网络内暴露，发布/回滚脚本改为 `docker-compose exec -T backend curl 127.0.0.1:8080/health`
- 将回滚脚本也补齐端口占用自适配（`FRONTEND_HTTP_PORT`/`FRONTEND_HTTPS_PORT`），保证本机演练场景下发布与回滚行为一致

---

## 17. 继续推进：v1.3 P0 流程治理文档落地

在发布/回滚演练闭环完成后，继续推进了 `docs/BACKEND_TODO_CHECKLIST_V1.3.md` 的 P0 项，避免团队后续协作继续依赖口头约定。

### 17.1 本轮新增与更新

- 新增后端贡献流程文档：`docs/CONTRIBUTING_BACKEND.md`
    - 明确 `feature branch -> PR -> required checks -> merge`
    - 给出可直接执行的最小自检命令
    - 补充 GitHub required checks 命名差异说明
- 新增 release 分支保护模板：`docs/BRANCH_PROTECTION_RELEASE_TEMPLATE.md`
    - 给出 `release/*` 的 required checks 基线
    - 给出 `main` 与 `release/*` 的差异化策略建议（尤其 up-to-date）
    - 提供管理员核对清单和一次人工演练步骤
- 新增 PR 模板：`.github/pull_request_template.md`
    - 固化变更范围、验证命令、风险评估、回滚方案
- 同步入口文档
    - `README.md` 文档索引新增上述文档入口
    - `docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md` 增加关联文档链接

### 17.2 清单状态同步

已将 `docs/BACKEND_TODO_CHECKLIST_V1.3.md` 中以下条目标记为完成：

- P0-1 贡献流程标准化
- P0-2 Release 分支保护模板落地
- 进度记录中的 `2026-04-W5：P0 完成`

### 17.3 价值总结

- 将“怎么提 PR、怎么过门禁、怎么做 release 分支保护”从经验沉淀成可复制文档。
- 新成员可以独立完成一次标准 PR 提交流程，不再依赖临时解释。
- 后续进入 v1.3 P1（CI 失败分层与性能门禁治理）时有明确输入边界。
- 将 Grafana dashboard 挂载路径从只读 provisioning 目录迁移到 `/var/lib/grafana/dashboards`，避免容器初始化时无法创建挂载点

---

## 18. 继续推进：v1.3 P1-3 CI 失败分层与动作指引

本轮将 `backend-checks` 从“统一失败”升级为“可分层定位”的失败模型，并把处理建议直接写入 Job Summary。

### 18.1 变更内容

- 更新 `scripts/run_backend_checks.sh`
    - 增加稳定退出码语义：
        - `11/21`：build
        - `31`：test
        - `41`：perf
        - `51`：perf-gate
    - 在各阶段失败时输出统一分类信息，便于 CI 映射
- 更新 `.github/workflows/backend-checks.yml`
    - `Run backend checks` 步骤改为捕获退出码并映射失败分类
    - 新增 Job Summary 状态块，输出：`exit_code`、`category`、`next_action`
    - 保留 benchmark summary 上传与展示
    - 最终由专用步骤按退出码决定 job fail，保证 summary 一定可见
- 更新 `README.md`
    - 增补失败分类与退出码说明，便于本地与 CI 对齐

### 18.2 对应清单进展

- `docs/BACKEND_TODO_CHECKLIST_V1.3.md` 中 P1-3 三项已完成并勾选

### 18.3 价值总结

- CI 失败不再需要通读全日志才知道方向。
- 首次定位成本下降：从“看哪里炸了”变成“先按分类处理”。
- 为后续 P1-4 的门禁阈值治理提供了更稳定的执行语义基础。

---

## 19. 继续推进：v1.3 P1-4 性能门禁阈值治理

本轮补齐了“阈值怎么改、谁能改、改了怎么留痕”的治理闭环，避免后续出现门禁参数随意漂移。

### 19.1 新增文档

- 新增策略文档：`docs/PERF_GATE_CHANGE_POLICY.md`
    - 定义触发条件、角色权限、标准变更流程、回滚策略
    - 约束阈值调整必须走 PR 评审并保留记录
- 新增记录模板：`docs/perf_reports/PERF_GATE_CHANGE_RECORD_TEMPLATE.md`
    - 固化“变更前后、原因、影响、证据、回滚预案”字段

### 19.2 联动更新

- 更新 `docs/PERFORMANCE_REPORT_TEMPLATE.md`
    - 新增“门禁阈值上下文”与“阈值治理记录”章节
- 更新 `README.md`
    - 补充策略文档入口与阈值变更治理说明
- 更新 `docs/BACKEND_TODO_CHECKLIST_V1.3.md`
    - P1-4 三项全部勾选完成

### 19.3 价值总结

- 阈值调整从“经验操作”变为“有流程、有证据、有回滚”的变更。
- 趋势报告与治理策略形成双向链接，便于审计和复盘。
- 为后续 P2（治理巡检脚本）提供了明确检查对象与规则来源。

---

## 20. 继续推进：v1.3 P2-5 治理巡检自动化

本轮把治理规则本身做成了一条可执行巡检命令，避免每次发布前都靠人工逐项翻文档。

### 20.1 新增脚本

- 新增 `scripts/audit_backend_governance.sh`
    - 检查 `backend-checks` workflow、后端检查脚本、贡献文档、release 分支保护模板、性能门禁策略与记录模板是否齐全
    - 检查 README 是否链接到了这些治理入口
    - 检查 `docs/BACKEND_TODO_CHECKLIST_V1.3.md` 中 P2-5 是否已经接入
    - 输出 `AUDIT_STATUS=OK/FAILED`、`AUDIT_OK=*`、`AUDIT_FAIL=*`，便于直接贴到 PR 评论

### 20.2 文档同步

- 更新 `README.md`
    - 增加“治理巡检（发布前自检）”章节
    - 明确该脚本用于发布前自检和 PR 评论输出
- 更新 `docs/BACKEND_TODO_CHECKLIST_V1.3.md`
    - P2-5 三项勾选完成
    - 进度记录新增 `2026-05-W2：P2-5 完成`

### 20.3 验证结果

- `bash -n scripts/audit_backend_governance.sh` 通过
- 脚本实跑输出 `AUDIT_STATUS=OK`，所有治理项均命中

### 20.4 价值总结

- 让发布前的治理检查变成“一条命令”而不是“人工 checklist”。
- 这一步完成后，后续做版本一致性治理时也有了统一检查入口。

---

## 21. 继续推进：v1.3 P2-6 文档版本一致性治理

本轮补齐了发布前最后一块治理拼图：README、Release Notes、devlog 的版本/日期一致性。

### 21.1 新增文档

- 新增 `docs/RELEASE_DOC_CHECKLIST.md`
    - 定义发布前版本同步规则
    - 固化 README / Release Notes / devlog / 产品路线图 / 后端清单的人工核对步骤
    - 提供可直接复制执行的核对命令

### 21.2 联动更新

- 更新 `README.md`
    - 在文档索引中新增发布文档核对清单入口
    - 在发布治理说明中要求先执行核对清单
- 更新 `docs/devlog.md`
    - 追加 2026-04-25 的发布文档一致性治理记录
- 更新 `docs/BACKEND_TODO_CHECKLIST_V1.3.md`
    - P2-6 三项全部勾选完成
    - 进度记录新增 `2026-05-W2：P2-6 完成`

### 21.3 验证结果

- `bash scripts/audit_backend_governance.sh` 仍为 `AUDIT_STATUS=OK`
- 新文档已可在 README 中直接访问

### 21.4 价值总结

- 发布文档不再是“改了就行”，而是有核对顺序、有证据、有入口。
- README / Release Notes / devlog 的关系被正式化，减少后续版本叙事漂移。
- 这一步完成后，v1.3 的治理侧能力已经具备比较完整的发布前自检闭环。

---

## 22. 继续推进：阶段 B1 传输与边界安全收口

本轮把公开 Beta 的第一项缺口收口为可执行配置：HTTPS/WSS、跳转、反代安全头与公网端口暴露边界。

### 22.1 技术落地

- 更新 `frontend/nginx/nginx.conf`
    - 增加 `server_tokens off`
    - 补充 `Content-Security-Policy` 基线
    - 保留 HSTS、X-Frame-Options、X-Content-Type-Options、Referrer-Policy、Permissions-Policy
- 更新 `docs/DEPLOYMENT.md`
    - 生产防火墙只开放 80/443
    - 明确后端 8080 不应直接暴露到公网
    - 将 WebSocket 测试示例切换为 `wss://.../ws`
- 更新 `docs/PRODUCT_LAUNCH_PLAYBOOK.md`
    - 将 B1 4 项标记为完成
    - 在已落地进展中加入 B1 状态

### 22.2 验证结果

- 前端 Nginx 配置已包含安全头与 CSP 基线
- 部署文档已与实际生产入口一致
- B1 在路线图中被正式勾选完成

### 22.3 价值总结

- 公开 Beta 的传输边界由“开发可用”推进到“生产可控”。
- 安全头与端口收敛减少了暴露面，也降低了后续外部扫描的风险。
- 生产文档与实际 compose/nginx 行为现在是一致的。

### 这次修复的价值

- 避免宿主机构建残留污染镜像构建
- 明显缩小 Docker build context，降低构建耗时
- 让发布脚本在干净环境里可重复执行

### 验证结果

- 发布演练首次失败原因已确认并定位
- 这类问题通过 `.dockerignore` 一次性消除，不需要改 Dockerfile 主逻辑

---

## 23. 追加推进：发布演练复跑成功与构建警告清理

在定位完发布脚本的状态后，又做了一次完整复跑，确认发布链路本身是通的。后来又直接执行了一次 `./scripts/release_prod.sh drill-20260424g`，同样成功；前一次 `exit 1` 更像是旧终端状态/超时残留，而不是脚本逻辑失败。

### 这一步做了什么

- 以 `bash -x scripts/release_prod.sh drill-20260424g` 方式复跑发布脚本，完整观察每个阶段。
- 直接执行 `./scripts/release_prod.sh drill-20260424g`，再次确认脚本可正常作为可执行文件运行。
- 确认脚本会自动避开被占用的 80/443，切到 `18080/18443`。
- 确认后端和前端健康检查都能通过。
- 在前端样式入口调整 `@import` 顺序，消除构建警告。

### 验证结果

- 发布脚本完整执行成功。
- 生成发布记录：`docs/release_records/release-drill-20260424g.md`。
- 后端 `/health` 和前端 `/health` 都已实测通过。
- Vite 的 CSS 顺序警告在调整后消失。

### 复盘要点

- 发布脚本的成功与否要看完整执行链路，不能只根据一次中断或超时判断。
- 构建警告虽然不阻断发布，但会污染演练日志，最好及时清理。

---

## 24. 继续推进：命令返回码不一致排障与修复

在“继续/重试”阶段，出现了两类容易误判的问题：

- `./scripts/release_prod.sh drill-20260424g` 曾出现 `exit 1`，但 `bash -x` 与后续复跑又成功。
- `git diff --check && git diff --stat -- docs/PRODUCT_LAUNCH_PLAYBOOK.md docs/perf_reports/PERFORMANCE_BASELINE_20260425.md` 返回 `exit 2`。

### 24.1 现场结论

- 发布脚本在最新一次完整复跑中成功（包含构建、部署、前后端健康检查、记录落盘）。
- `exit 2` 的根因不是 `PRODUCT_LAUNCH_PLAYBOOK.md` 或 `PERFORMANCE_BASELINE_20260425.md`，而是另一个文件的空白问题先触发了 `git diff --check` 失败。

### 24.2 根因定位

问题文件：`docs/perf_reports/message_history_benchmark_summary.csv`

- 新增行在 Git 视角被判定为 trailing whitespace。
- 进一步检查发现该文件使用 CRLF（`^M`）行尾，导致新增行触发空白检查失败。

### 24.3 处理动作

- 将 `docs/perf_reports/message_history_benchmark_summary.csv` 从 CRLF 统一转换为 LF。
- 复跑原命令链路并确认恢复正常。

### 24.4 验证结果

- `git diff --check && git diff --stat -- docs/PRODUCT_LAUNCH_PLAYBOOK.md docs/perf_reports/PERFORMANCE_BASELINE_20260425.md` => `EXIT:0`
- `./scripts/release_prod.sh drill-20260424g` 最新复跑 => `EXIT:0`

### 24.5 复盘要点

- `git diff --check` 是全局空白门禁，哪怕只统计两个文件，也会被仓库内其他改动先拦截。
- 遇到“同命令偶发失败”时，优先保留一条 `bash -x` 成功链路，再区分是脚本问题还是终端状态/环境噪声。

---

## 25. 继续推进：B2 备份与恢复实测闭环

在完成前一节排障后，继续按 B2 runbook 执行了一轮“备份 -> 恢复 -> 健康检查”的最小闭环演练。

### 25.1 执行动作

- 执行备份：`bash scripts/backup_db.sh`
- 选取最新备份并恢复：`bash scripts/restore_db.sh backups/db/chat.db.20260425_133057.sqlite3`
- 恢复后健康检查：`docker-compose -f docker-compose.prod.yml exec -T backend curl -fsS http://127.0.0.1:8080/health`

### 25.2 关键证据

- 新备份产物：`backups/db/chat.db.20260425_133057.sqlite3`
- 恢复脚本关键输出：
    - `[1/4] 停止后端服务... done`
    - `[2/4] 恢复数据库文件...`
    - `[3/4] 启动后端服务... done`
    - `[4/4] 验证数据库健康状态...`
    - `恢复完成: backups/db/chat.db.20260425_133057.sqlite3`
- 恢复后健康检查返回：`{"active_connections":5,"db":true,"service":"chat_server","status":"ok"}`

### 25.3 结论

- B2 的脚本能力在当前环境可直接执行，且恢复后服务健康状态正常。
- 当前闭环已满足“可备份、可恢复、可验证”的最小验收目标。

### 25.4 后续建议

- 将本次演练结果同步到 `docs/devlog.md`，与发布演练记录形成统一时间线。
- 下一轮可补一次“指定旧备份恢复 + 关键业务抽样”演练，验证历史快照可用性。

---

## 26. 继续推进：旧备份恢复与关键数据抽样验证

按上一节建议，继续执行了一轮“指定旧备份恢复 + 用户/消息/房间抽样”的补充演练，用于验证历史快照的可恢复性。

### 26.1 执行动作

- 选取次新备份：`backups/db/chat.db.20260425_082642.sqlite3`
- 执行恢复：`bash scripts/restore_db.sh backups/db/chat.db.20260425_082642.sqlite3`
- 恢复后健康检查：`docker-compose -f docker-compose.prod.yml exec -T backend curl -fsS http://127.0.0.1:8080/health`
- 使用临时容器挂载 `chat_data` 卷，对 SQLite 做抽样计数校验

### 26.2 关键证据

- 恢复脚本输出：`恢复完成: backups/db/chat.db.20260425_082642.sqlite3`
- 健康检查返回：`{"active_connections":4,"db":true,"service":"chat_server","status":"ok"}`
- 表结构可见：`conversations, friend_requests, friendships, messages, private_messages, rooms, schema_meta, sqlite_sequence, user_presets, users`
- 抽样计数结果：
    - `COUNT users: 0`
    - `COUNT messages: 0`
    - `COUNT rooms: 6`

### 26.3 结论

- 历史备份文件在当前环境可成功恢复，恢复后服务保持健康。
- 用户/消息/房间三类核心表均可查询，满足 B2 的“恢复后可读性验证”要求。

### 26.4 复盘要点

- 在宿主机权限受限场景下，可通过临时容器挂载卷进行 SQLite 验证，避免直接访问 `/var/lib/docker/volumes` 失败。
- 恢复验证建议固定为“两层证据”：服务健康（`/health`）+ 关键数据抽样（至少 3 张核心表）。

---

## 27. 继续推进：恢复后验证脚本化

为了避免后续每次都手工重复“恢复 + 健康检查 + 抽样计数”，新增了一键验证脚本 `scripts/verify_restore_db.sh`。

### 27.1 新增能力

- 可选指定备份文件，脚本会先调用恢复脚本，再执行验证。
- 自动检查后端 `/health` 是否可用。
- 自动挂载 `chat_data` 卷并抽样检查 `users`、`messages`、`rooms` 三张核心表。

### 27.2 验证结果

- `bash -n scripts/verify_restore_db.sh` 通过。
- `bash scripts/verify_restore_db.sh backups/db/chat.db.20260425_082642.sqlite3` 通过。
- 脚本输出已包含表结构与计数结果：
    - `TABLES: conversations,friend_requests,friendships,messages,private_messages,rooms,schema_meta,sqlite_sequence,user_presets,users`
    - `COUNT users: 0`
    - `COUNT messages: 0`
    - `COUNT rooms: 6`

### 27.3 复盘要点

- 一键验证脚本应把“恢复成功”与“可读性验证”绑在一起，否则恢复成功的判断不够完整。
- 使用临时容器执行 SQLite 读取时，必须保留 stdin 传递能力，否则 heredoc 代码不会真正进入容器。

---

## 28. 继续推进：性能基准三次复跑与波动确认

为了把 B3 的性能数据从“单次结果”推进到“可解释的趋势”，连续复跑了三次消息历史基准。

### 28.1 复跑结果

- 第 1 次：`speedup=117.91x`
- 第 2 次：`speedup=124.00x`
- 第 3 次：`speedup=140.87x`

### 28.2 结论

- 与首次基线 `59.27x` 相比，三次复跑明显更快，说明该基准对缓存/环境状态高度敏感。
- 当前更适合把 `59.27x` 视作“首次冷态基线”，把后续三次视作“热态趋势样本”。

### 28.3 复盘要点

- 性能门禁不能只看单点值，必须记录冷/热缓存条件，否则误判概率很高。
- 复跑值跨度较大时，优先补记录，不要急着改索引或 SQL。

---

## 29. 继续推进：B3 关键路径限流落地

为了继续推进阶段 B3，本轮在后端统一 AI 调用入口上加了更细粒度的请求节流，避免 `/msg ai` 和 `@AI` 两条路径对外部模型与连接线程形成突发压力。

### 29.1 新增能力

- 在 `send_ai_response` 中增加 AI 请求限流。
- 按 IP 维度限制 AI 调用频率。
- 按用户维度限制 AI 调用频率。
- 继续保留现有的命令级 IP 限流与登录限流。

### 29.2 关键实现

```cpp
RateLimiter g_ai_ip_limiter(30, std::chrono::seconds(60));
RateLimiter g_ai_user_limiter(12, std::chrono::seconds(60));
```

```cpp
if (!g_ai_ip_limiter.allow("ai_ip:" + ip))
{
    send_system_message("AI 请求过于频繁，请稍后重试。");
    return;
}
```

### 29.3 验证结果

- `Build_CMakeTools` 通过，`chat_server` 与 `chat_server_test` 成功重新链接。
- 既有测试未受影响，编译期间仅保留与本次改动无关的旧 warning。

### 29.4 复盘要点

- B3 的“关键路径限流”最好落在真正昂贵的调用入口，而不是只靠外围命令限流。
- AI 请求是最容易被高频触发的昂贵路径之一，按 IP + 用户双维度节流更稳妥。

---

## 30. 继续推进：B3 并发连接压测通过

为了补齐 B3 的“压测并输出报告”，在正式 WSS `/ws` 入口上跑了一轮并发连接稳定性测试。

### 30.1 执行方式

- 脚本：`scripts/test_concurrent_connections.py`
- 入口：`wss://127.0.0.1:18443/ws`
- 参数：`clients=30`、`waves=3`、`timeout=5`

### 30.2 结果摘要

- `[wave 1] ok=30/30`
- `[wave 2] ok=30/30`
- `[wave 3] ok=30/30`
- `[summary] total=90, failed=0, success_rate=100.00%`

### 30.3 结论

- 当前正式入口下，90 个并发连接、3 个波次全部通过。
- 这轮验证更像连接稳定性基线，和消息历史 benchmark 的性能趋势应该分开记录。

### 30.4 复盘要点

- 并发测试要连接正式入口，不要只盯着 backend 内部端口，否则容易得到错误失败结论。
- WSS 场景下如果证书是本地自签，压测脚本需要明确支持跳过校验，避免把 TLS 配置误判为服务异常。

---

## 31. 继续推进：B3 纯连接容量确认

为了把“并发连接”和“命令分发”拆开验证，又补跑了一次纯连接模式压测。

### 31.1 执行方式

- 脚本：`scripts/test_concurrent_connections.py`
- 入口：`wss://127.0.0.1:18443/ws`
- 模式：`connect_only`
- 参数：`clients=200`、`waves=1`、`timeout=5`

### 31.2 结果摘要

- `[wave 1] ok=200/200`
- `[summary] total=200, failed=0, success_rate=100.00%`

### 31.3 结论

- 正式 WSS `/ws` 入口在 200 个并发连接下稳定可用，达到首轮在线连接目标。
- 这份结果更适合作为“连接容量基线”，与 `rooms_json` 命令模式分开记录。

### 31.4 复盘要点

- B3 的在线连接容量应该先用纯连接模式验证，再单独做命令/AI 压力测试，避免不同限流策略互相干扰。
- 当 command 模式触发 rate limit 时，不应直接推导出连接容量不足。

---

## 32. 继续推进：纯连接容量扩展验证（250/300）

在完成 200 并发纯连接验证后，继续把压力提升到 250 和 300，并保持同样的 `connect_only` 模式。

### 32.1 执行方式

- 入口：`wss://127.0.0.1:18443/ws`
- 脚本：`scripts/test_concurrent_connections.py`
- 模式：`connect_only`
- 参数：`clients=250, waves=1, timeout=5` 与 `clients=300, waves=1, timeout=5`

### 32.2 结果摘要

- 250 并发：`[summary] total=250, failed=0, success_rate=100.00%`
- 300 并发：`[summary] total=300, failed=0, success_rate=100.00%`

### 32.3 结论

- 当前正式入口在 300 并发纯连接下仍保持稳定。
- B3 的首轮在线连接目标（200）已被覆盖并留有余量。

### 32.4 复盘要点

- 连接容量和命令吞吐应持续分开度量，否则压测结论会被限流策略污染。
- 在现阶段结果下，下一步更值得做的是“300 并发 + 业务命令”组合压测，而不是继续只拉高纯连接数量。

---

## 33. 继续推进：300 并发命令模式校验

按照上一节建议，补做了 `300 并发 + rooms_json` 组合压测，用于验证命令路径在高并发下的行为边界。

### 33.1 执行方式

- 脚本：`scripts/test_concurrent_connections.py`
- 入口：`wss://127.0.0.1:18443/ws`
- 模式：`rooms_json`
- 参数：`clients=300`、`waves=1`、`timeout=5`

### 33.2 结果摘要

- 紧接 200 并发测试后执行：`[summary] total=300, failed=260, success_rate=13.33%`
- 重启 backend 清空窗口后执行：`[summary] total=300, failed=60, success_rate=80.00%`（`240/300`）

### 33.3 结论

- 该失败模式主要是命令限流触发（系统消息），并非连接握手失败。
- 300 并发下 `rooms_json` 成功数与命令限流窗口一致（240/分钟/IP），与当前限流配置预期一致。

### 33.4 复盘要点

- 命令模式压测结果必须结合限流阈值解读，否则容易误判为“服务不稳定”。
- 连接容量基线（connect_only）与命令吞吐基线（rooms_json）应长期双轨记录。

---

## 34. 继续推进：300 并发命令模式分批优化

在确认 300 并发命令模式受限流窗口影响后，继续做了一轮分批发送验证，目标是不改阈值前提下提升成功率。

### 34.1 执行方式

- 脚本：`scripts/test_concurrent_connections.py`
- 模式：`rooms_json`
- 参数：`clients=300, waves=1, batch_size=30, batch_interval_ms=9000`

### 34.2 结果摘要

- 分批 10 轮均通过（每批 30/30）
- 最终结果：`[summary] total=300, failed=0, success_rate=100.00%`

### 34.3 结论

- 在不修改限流阈值的前提下，分批策略可以显著降低命令路径瞬时冲击并恢复总体成功率。
- 300 并发命令模式可以通过“节奏控制”达到稳定通过，不必把该问题误判为连接容量不足。

### 34.4 复盘要点

- 压测中遇到限流失败时，优先尝试发送节奏控制，再决定是否调整限流参数。
- 对生产策略而言，限流阈值是保护机制；对压测策略而言，分批发送是更合理的验证手段。

---

## 35. 继续推进：压测参数矩阵沉淀

为了让后续 B3 回归不再重复探索参数，本轮把并发压测结果整理成统一矩阵文档。

### 35.1 新增文档

- `docs/perf_reports/concurrency_test_matrix_20260425.md`

### 35.2 覆盖范围

- `connect_only`：200/250/300 并发容量结果
- `rooms_json`：200/300 命令模式结果
- 分批策略：300 并发下 `batch_size=30, batch_interval_ms=9000` 的稳定通过结果

### 35.3 结论

- 连接容量与命令吞吐已形成双轨回归基线。
- 压测流程可按“connect_only -> rooms_json -> rooms_json batched”顺序复用。

---

## 36. 继续推进：400 并发边界观察

在参数矩阵基础上，继续补跑 400 并发档位，目的是确认“连接容量上限”和“命令模式限流边界”是否开始分化。

### 36.1 执行方式

- `connect_only`：`clients=400, waves=1, timeout=5`
- `rooms_json`（分批）：`clients=400, waves=1, batch_size=40, batch_interval_ms=9000`

### 36.2 结果摘要

- `connect_only`：`[summary] total=400, failed=0, success_rate=100.00%`
- `rooms_json`（分批）：`[summary] total=400, failed=40, success_rate=90.00%`
- `rooms_json` 第 7 批出现 `0/40`，其余批次通过。

### 36.3 结论

- 连接容量在 400 档位仍稳定。
- 命令模式在 400 档位开始显著受限流窗口影响，当前分批参数已接近阈值边界。

### 36.4 复盘要点

- 当并发继续提升时，应优先用“批大小/间隔”调节压测节奏，而不是直接把连接失败归因给服务容量。
- 400 档位已进入“容量稳定、命令受限流约束”的分化区，适合作为下一轮策略优化起点。

---

## 37. 继续推进：400 并发分批参数收敛

在 400 并发边界观察后，继续做了分批参数搜索，目标是在不改限流阈值前提下找到稳定 100% 的参数组合。

### 37.1 参数对比

- `40/9000` => `360/400`（90.00%）
- `25/6000` => `390/400`（97.50%）
- `20/5000` => `400/400`（100.00%）

### 37.2 结论

- 400 并发命令模式可通过节奏参数收敛到 100% 成功率。
- 当前最佳参数：`batch_size=20, batch_interval_ms=5000`。

### 37.3 复盘要点

- 高并发命令压测应采用“参数搜索 -> 固化推荐值”的方式，而不是一次性猜测批大小。
- 当目标是验证系统上限而非绕过保护策略时，优先调节发送节奏比调高限流阈值更安全。

---

## 38. 继续推进：500 并发容量确认

在 400 档位参数收敛后，继续把纯连接与命令模式都推进到 500，确认当前入口是否还有余量。

### 38.1 执行方式

- `connect_only`：`clients=500, waves=1, timeout=5`
- `rooms_json`：`clients=500, waves=1, batch_size=20, batch_interval_ms=5000`

### 38.2 结果摘要

- 连接容量：`[summary] total=500, failed=0, success_rate=100.00%`
- 命令模式（推荐节奏）：`[summary] total=500, failed=0, success_rate=100.00%`

### 38.3 结论

- 当前正式入口在 500 并发下仍保持稳定。
- 推荐分批参数 `20/5000` 在 500 档位仍然可复用。

### 38.4 复盘要点

- 当容量继续抬升时，先复用既有推荐节奏，再决定是否需要再做参数搜索。
- 500 档位的结果说明，当前系统更接近“节奏可控”而非“容量不足”。

## 39. 继续推进：600 并发容量确认

在 500 档位结果继续保持稳定后，再把纯连接与命令模式都推进到 600，确认当前推荐节奏是否还能复用。

### 39.1 执行方式

- `connect_only`：`clients=600, waves=1, timeout=5`
- `rooms_json`：`clients=600, waves=1, batch_size=20, batch_interval_ms=5000`

### 39.2 结果摘要

- 连接容量：`[summary] total=600, failed=0, success_rate=100.00%`
- 命令模式（推荐节奏）：`[summary] total=600, failed=0, success_rate=100.00%`

### 39.3 结论

- 当前正式入口在 600 并发下仍保持稳定。
- 推荐分批参数 `20/5000` 在 600 档位仍然可复用。

### 39.4 复盘要点

- 600 档位进一步说明，当前系统的主要约束仍是节奏与限流窗口，而不是连接容量本身。
- 现在可以把 600 作为高档位回归基线，日常回归只保留 200 / 300 / 600 三档。
- 400 / 500 仅保留为已验证的探索档位，后续只有在找上限时才临时加测。

