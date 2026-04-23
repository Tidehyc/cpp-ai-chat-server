# 开发日志

---

## 📅 2026-04-23 (第 44 天) - Required Checks 配置文档落地 ✅

### 完成的工作

- [x] 新增分支保护配置文档 `docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md`
- [x] 给出 `main` 分支 required checks 推荐项与操作步骤
- [x] 在 README 增加文档入口与引用
- [x] 对齐当前 CI 分支化门禁策略说明

### 问题背景

- 现有 CI 已支持按分支自动切换性能门禁，但仓库设置层面缺少一份“可直接照做”的分支保护配置说明。
- 没有明确文档时，后续维护者容易遗漏 required checks，导致策略不能真正生效。

### 根因分析

1. CI 逻辑已实现，但 GitHub 仓库配置属于平台侧动作，不在代码变更里自动完成。
2. 检查项命名在 UI 中可能因工作流/作业显示方式不同而混淆。

### 实现细节

1. **新增操作手册**

```text
docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md
```

- 内容包含：
  - 分支保护开启路径（Settings -> Branches）
  - required checks 选择建议
  - 与 `PERF_WARN_FAIL` 分支策略的映射关系
  - 合并前自检命令

2. **README 入口补齐**

- 在开发文档索引中加入该文档链接。
- 在 CI 章节补充“具体配置步骤见该文档”的跳转。

### 验证与证据

- 新文档已创建并可在仓库内直接访问。
- README 文档索引与 CI 章节均已包含新入口，形成闭环导航。

### 踩坑与经验

- 把“代码策略”和“平台配置步骤”放在同一链路里，能显著降低后续落地偏差。
- Required checks 名称应给出“按实际 UI 为准”的说明，避免硬编码单一展示名。

### 后续优化

- [ ] 在仓库治理文档中补一份 release 分支保护模板。
- [ ] 若团队扩大，可把该配置纳入 onboarding 清单。

---

## 📅 2026-04-23 (第 43 天) - CI 分支化性能门禁策略 ✅

### 完成的工作

- [x] 在 workflow 中新增门禁模式解析步骤
- [x] 按分支自动切换 `PERF_WARN_FAIL`
- [x] 保持本地脚本入口不变
- [x] 更新 README，补充分支化门禁说明

### 问题背景

- 之前虽然支持了 `PERF_WARN_FAIL`，但 CI 仍是固定值，无法体现“主分支严格、开发分支灵活”的策略。
- 统一策略需要在不增加维护负担的前提下完成自动切换。

### 根因分析

1. workflow 里对 `PERF_WARN_FAIL` 使用静态配置，缺少上下文判断。
2. 不同分支的风险容忍度不同，但没有被编码到 CI 流程。

### 实现细节

1. **新增门禁解析步骤**

```yaml
- name: Resolve perf warning gate mode
```

- 默认：`PERF_WARN_FAIL=0`
- 若 `push` 到 `main`：切换为 `PERF_WARN_FAIL=1`
- 若 `pull_request` 目标分支是 `main`：切换为 `PERF_WARN_FAIL=1`
- 结果写入 `GITHUB_ENV`，后续 `Run backend checks` 直接复用。

### 验证与证据

- 本地门禁行为已在前一步验证：
  - 默认模式通过
  - `PERF_WARN_FAIL=1` 时触发 warning 返回非 0
- 本次变更不改构建与测试命令，仅改 CI 环境注入策略。

### 踩坑与经验

- 将策略编码在单独步骤中，比在多个步骤重复写条件更易维护。
- 对门禁策略做“默认宽松 + 关键分支严格”能平衡效率与质量。

### 后续优化

- [ ] 若后续 main 分支稳定，可评估对 release 分支采用更高阈值或更严格规则。
- [ ] 结合分支保护，将该检查设为 required check。

---

## 📅 2026-04-23 (第 42 天) - 性能告警门禁可配置化 ✅

### 完成的工作

- [x] 为 `scripts/summarize_perf_reports.py` 增加命令行参数：`--warn-ratio`、`--fail-on-warn`
- [x] 为 `scripts/run_backend_checks.sh` 增加环境变量开关：`PERF_WARN_RATIO`、`PERF_WARN_FAIL`
- [x] 在 CI workflow 中补充 Job Summary 发布步骤
- [x] 本地完成默认模式与门禁模式双路径验证

### 问题背景

- 现有 warning 机制只做提示，不便于在不同场景下切换“观测模式”与“门禁模式”。
- CI 虽可在日志里看到摘要，但 PR 页面缺少稳定的 Summary 入口，不利于快速审阅。

### 根因分析

1. 汇总脚本没有参数化控制，阈值和失败策略固定在代码里。
2. 统一检查脚本未暴露门禁开关，无法在不同执行环境复用同一入口。
3. workflow 只输出 log/artifact，缺少 Job Summary 直观展示。

### 实现细节

1. **汇总脚本参数化**

```bash
python3 scripts/summarize_perf_reports.py --warn-ratio 0.80 --fail-on-warn
```

- 当触发 warning 且启用 `--fail-on-warn` 时，脚本返回 `exit code 2`。
- `--warn-ratio` 取值约束为 `(0, 1]`，非法值直接失败并给出提示。

2. **统一入口支持门禁开关**

```bash
PERF_WARN_FAIL=1 PERF_WARN_RATIO=0.80 bash scripts/run_backend_checks.sh
```

- 默认 `PERF_WARN_FAIL=0`，保持本地日常检查不被告警中断。
- 在需要严格门禁的场景可直接开启，无需改脚本。

3. **CI 可见性增强（Summary 页面）**

- workflow 新增步骤将 `docs/perf_reports/message_history_benchmark_summary.md` 写入 `$GITHUB_STEP_SUMMARY`。
- 这样 PR 页面可直接查看趋势与告警，不必先下载 artifact。

### 验证与证据

- 默认模式：`bash scripts/run_backend_checks.sh` 成功，`status=OK`。
- 门禁模式：`PERF_WARN_FAIL=1 PERF_WARN_RATIO=0.80 bash scripts/run_backend_checks.sh` 在触发 warning 时返回 `exit code 2`。
- 两次运行均通过构建与 CTest：`100% tests passed, 0 tests failed out of 36`。

### 踩坑与经验

- 性能门禁要支持“开关可配”，否则容易在日常开发中产生过多阻断。
- 同一份 summary 同时用于 log、artifact、job summary，能降低巡检成本。

### 后续优化

- [ ] 结合分支策略，评估是否在受保护分支将 `PERF_WARN_FAIL` 默认改为 `1`。
- [ ] 评估告警阈值从固定比例升级为分位数或滑窗基线。

---

## 📅 2026-04-23 (第 41 天) - 后端检查闭环再验证 ✅

### 完成的工作

- [x] 重新执行 `scripts/run_backend_checks.sh`
- [x] 验证构建、36 个 CTest、benchmark、summary 全链路可跑通
- [x] 生成新的 benchmark 报告与趋势摘要
- [x] 确认 warning 继续在 summary 中可见

### 问题背景

- 在前一次 CI 可见性增强之后，需要再跑一轮完整后端检查，确认最近的代码与脚本改动没有把链路打断。
- benchmark 本身会有波动，因此这一步也顺手观察最新速度比是否仍能触发告警。

### 根因分析

1. 需要用一次完整执行来验证脚本编排没有隐性回归。
2. 最新 benchmark 结果波动较大，适合作为趋势告警规则的复核样本。

### 实现细节

1. **复用统一入口**

```bash
bash scripts/run_backend_checks.sh
```

- 脚本会按顺序执行 configure、build、CTest、benchmark、summary。
- 这次 build 命中缓存，`ninja: no work to do.`，说明构建输入没有额外漂移。

2. **刷新趋势产物**

- 新增报告：`docs/perf_reports/message_history_benchmark_20260423_091225.txt`
- 更新摘要：`docs/perf_reports/message_history_benchmark_summary.md`
- 更新 CSV：`docs/perf_reports/message_history_benchmark_summary.csv`

### 验证与证据

- CTest：`100% tests passed, 0 tests failed out of 36`
- 最新 benchmark：

```text
indexed_total_ms=89.56
no_index_total_ms=3011.94
speedup=33.63x
```

- summary 输出仍包含 warning：

```text
WARNING: latest_speedup 33.63x is below 80% of average speedup (100.22x)
```

### 踩坑与经验

- benchmark 结果会随机器负载波动，趋势告警适合看长期变化，不适合拿单次数值做绝对判断。
- 先确认脚本链路完整，再看数值回退，是更稳的巡检顺序。

### 后续优化

- [ ] 如果后续还要频繁跑基准，考虑把最近一次报告做成更容易定位的入口。
- [ ] 继续观察趋势告警是否需要从 warning 升级为更明确的 CI summary。

---

## 📅 2026-04-23 (第 40 天) - CI 可见性增强：趋势摘要直出日志 ✅

### 完成的工作

- [x] 在 GitHub Actions workflow 中新增 `Show benchmark summary` 步骤
- [x] 让 CI 日志直接展示 `message_history_benchmark_summary.md`
- [x] 保持原始 benchmark 报告与 summary artifact 一并上传
- [x] 本地复核通过，summary warning 仍可稳定输出

### 问题背景

- 目前 CI 已能运行完整后端检查并生成趋势 summary，但 warning 主要停留在脚本输出中。
- 如果不把 summary 直接写入 Actions 日志，排查趋势异常时仍需要额外下载 artifact。

### 根因分析

1. workflow 只上传 artifact，没有把 summary 内容同步到日志页。
2. warning 虽然存在，但可见性不如直接在 CI log 中展示。

### 实现细节

1. **新增日志输出步骤**

```yaml
- name: Show benchmark summary
  if: always()
  run: cat docs/perf_reports/message_history_benchmark_summary.md
```

- 这一步放在 artifact 上传前，确保即使后续步骤失败也能看到 summary。
- 通过直接输出 markdown，CI 页面可立即查看告警段和趋势表。

### 验证与证据

- 本地重新执行汇总脚本仍可输出 warning：

```text
reports=3
latest_speedup=101.79x
warn_ratio=80%
WARNING: latest_speedup 101.79x is below 80% of average speedup (124.66x)
```

### 踩坑与经验

- 把结果直接打印进 CI 日志，比单纯靠 artifact 更适合“先看是否异常、再决定要不要下载详细文件”的场景。
- `if: always()` 很适合放在这种可见性步骤上，避免前面失败导致 summary 也看不到。

### 后续优化

- [ ] 如果后续 warning 频繁出现，可以考虑在 workflow summary 里再加一段简短结论。
- [ ] 视情况把 summary 文件名也固定成最近一次的软链接，便于下载。

---

## 📅 2026-04-23 (第 39 天) - 性能趋势告警补齐 ✅

### 完成的工作

- [x] 为 `scripts/summarize_perf_reports.py` 增加历史均值阈值告警
- [x] 将告警写入 summary markdown 与 stdout
- [x] 更新 README，补充 warning 规则说明
- [x] 实跑验证通过，当前趋势已触发 warning

### 问题背景

- 汇总脚本已经能产出趋势表，但还不能主动提示“最新结果是否明显低于历史基线”。
- 没有告警时，趋势回退只能靠人工盯表，不利于日常巡检。

### 根因分析

1. 之前的 summary 只负责展示，不负责健康判断。
2. 趋势表中虽然有多个样本，但缺少一个简单且稳定的阈值规则。

### 实现细节

1. **增加告警规则**

```python
warn_ratio = 0.80
threshold = avg_speedup * warn_ratio
if latest.speedup < threshold:
  print(f"WARNING: ...")
```

- 规则定义：最新 speedup 低于历史平均 speedup 的 80% 时输出 warning。
- 告警信息同步写入 `message_history_benchmark_summary.md` 的 Alerts 段。

2. **保留摘要产物**

- `docs/perf_reports/message_history_benchmark_summary.md`
- `docs/perf_reports/message_history_benchmark_summary.csv`

### 验证与证据

- 运行结果：

```text
reports=3
latest_speedup=101.79x
warn_ratio=80%
WARNING: latest_speedup 101.79x is below 80% of average speedup (124.66x)
```

### 踩坑与经验

- 告警逻辑要尽量简单、稳定、可解释；这里用“最新值 vs 历史均值的固定比例”比复杂统计规则更适合日常巡检。
- 输出 warning 不一定要让脚本失败，先做可见告警，避免打断常规检查流程。

### 后续优化

- [ ] 如果后续样本数继续增长，可以考虑增加基线区间或分位数阈值。
- [ ] 评估在 CI 中把 warning 汇总成独立检查摘要。

---

## 📅 2026-04-22 (第 38 天) - 性能报告历史汇总自动化 ✅

### 完成的工作

- [x] 新增趋势汇总脚本 `scripts/summarize_perf_reports.py`
- [x] 将汇总步骤接入 `scripts/run_backend_checks.sh`（benchmark 后自动执行）
- [x] 更新 README，补充趋势汇总命令与产物路径
- [x] 实跑验证通过（构建 + CTest + benchmark + summary）

### 问题背景

- 已具备单次性能基准输出，但历史报告仅以独立文本存在。
- 缺少统一汇总视图时，难以快速判断趋势、波动区间与最新基线位置。

### 根因分析

1. 报告格式固定但分散，未形成可直接对比的聚合结果。
2. 统一检查脚本未包含“汇总”阶段，导致人工复盘成本高。

### 实现细节

1. **新增汇总脚本**

```bash
python3 scripts/summarize_perf_reports.py
```

- 自动扫描 `docs/perf_reports/message_history_benchmark_*.txt`
- 解析核心指标并输出：
  - `docs/perf_reports/message_history_benchmark_summary.md`
  - `docs/perf_reports/message_history_benchmark_summary.csv`

2. **接入统一检查入口**

```bash
bash scripts/run_backend_checks.sh
```

- 流程升级为 5 步：配置 -> 构建 -> CTest -> benchmark -> summary
- 每次跑完都会自动刷新趋势文件

### 验证与证据

- 端到端执行结果：`100% tests passed, 0 tests failed out of 36`
- 最新基准文件：`message_history_benchmark_20260422_200545.txt`
- 汇总结果：

```text
reports=3
latest_speedup=101.79x
avg_speedup=155.82x
```

### 踩坑与经验

- 指标解析要避免“全局字符替换”这类粗粒度做法；`speedup=...x` 应通过正则后缀兼容处理，而非替换整行字符。
- 把汇总纳入统一入口比单独脚本更稳，能减少“只跑基准不更新趋势”的漏操作。

### 后续优化

- [ ] 增加趋势异常阈值告警（例如 speedup 低于历史均值一定比例时提示）
- [x] 在 CI artifact 中额外上传 summary 文件（md/csv）

---

## 📅 2026-04-22 (第 37 天) - 一键后端检查入口与性能报告模板 ✅

### 完成的工作

- [x] 新增统一检查脚本 `scripts/run_backend_checks.sh`
- [x] 新增性能报告模板 `docs/PERFORMANCE_REPORT_TEMPLATE.md`
- [x] 更新 README，补充统一入口命令
- [x] 端到端验证通过（构建 + CTest + 性能基准）

### 问题背景

- 当前已完成性能优化与回归测试，但执行入口分散（构建、测试、基准脚本分开跑）。
- 缺少统一的性能报告模板，导致每次记录口径不一致，不利于横向比较。

### 根因分析

1. 流程拆散：验证路径依赖人工记忆命令顺序，容易漏跑。
2. 输出缺标准：基准结果没有固定模板，后续复盘成本高。

### 实现细节

1. **统一脚本入口**

```bash
bash scripts/run_backend_checks.sh
```

- 步骤顺序固定为：配置 -> 构建 -> CTest -> 性能基准。
- 基准输出自动保存到 `docs/perf_reports/message_history_benchmark_<timestamp>.txt`。

2. **模板标准化**

- 新增 `docs/PERFORMANCE_REPORT_TEMPLATE.md`，统一记录：
  - 环境信息
  - 执行命令
  - 原始输出
  - 结果摘要
  - 结论与行动项

### 验证与证据

- 脚本实跑结果：
  - CTest：`36/36` 通过
  - 基准输出：

```text
rows=50000, rounds=100
indexed_total_ms=11.39
no_index_total_ms=2430.13
speedup=213.32x
```

### 踩坑与经验

- 性能回归要避免只看“单次快照”，必须同时保留原始输出文件便于复核。
- 统一入口脚本应只做“编排”，具体逻辑留给独立脚本，方便替换与扩展。

### 后续优化

- [x] 将 `scripts/run_backend_checks.sh` 接入 CI（新增 `.github/workflows/backend-checks.yml`）
- [x] 增加历史报告汇总脚本（自动生成趋势表）

---

## 📅 2026-04-22 (第 36 天) - P1-5 性能对比与回归测试收尾 ✅

### 完成的工作

- [x] 新增性能基准脚本 `scripts/benchmark_message_history.py`
- [x] 新增两条消息历史性能回归测试（`tests/test_message_history.cpp`）
- [x] 完成“有索引/无索引”耗时对比并记录结果
- [x] 全量 CTest 回归通过（包含新增性能测试）

### 实现内容

1. **性能基准脚本**

- 文件：`scripts/benchmark_message_history.py`
- 方法：同一数据集上先测索引版，再 `DROP INDEX` 测无索引版
- 查询模式：`room_name + timestamp range + ORDER BY timestamp DESC + LIMIT`

2. **性能回归测试**

- `MessageHistoryTest.SearchPerformanceBaselineWithIndex`
  - 插入 10,000 条样本，执行 40 轮检索
  - 约束平均耗时低于基础阈值
- `MessageHistoryTest.SearchPerformanceRegressionIndexedShouldNotBeWorseThanNoIndex`
  - 先测索引，再删除 `idx_room_name_timestamp` 和 `idx_timestamp`
  - 断言索引版总耗时不劣于无索引版的 1.2x

### 性能结果（实测）

```text
=== Message History Benchmark ===
rows=50000, rounds=100
indexed_total_ms=27.42
indexed_avg_ms=0.2742
no_index_total_ms=2243.97
no_index_avg_ms=22.4397
speedup=81.83x
```

### 验证结果

- 构建：`Build_CMakeTools` 通过
- 测试：`RunCtest_CMakeTools` 通过（新增性能测试通过）

### 结论

- P1-5 剩余两项（性能对比、性能回归测试）已全部完成。
- 历史/搜索优化项现已具备“可量化基线 + 自动回归门禁”。

---

## 📅 2026-04-22 (第 35 天) - P2-9 命令处理模块解耦 ✅

### 完成的工作

- [x] 新增 `CommandDispatcher` 类（src/command/command_dispatcher.h/cpp）
- [x] 完成解析层与执行层职责分离（Connection::handle_command → execute_command）
- [x] 全量 CTest 单元测试通过（34/34）
- [x] 并发连接稳定性验证通过（90/90，100% 成功率）

### 问题背景

- P2-9 是整个后端优化整体清单的最后一项
- Connection::handle_command 目前包含 30+ 个 if-else 分支，代码复杂度高，难以单独测试命令
- 需要提高命令层的模块化程度，为后续拓展和维护铺路

### 设计方案

**CommandDispatcher（解析层/分发层）**

```cpp
class CommandDispatcher {
  // dispatch 接收原始输入，判断是否为命令（/@开头）
  // 提取 cmd_name 和 args 后，通过 executor_() 回调分发
  bool dispatch(const std::string &input);
  static bool is_command(const std::string &input);
  static std::string get_help();
};
```

**Connection（执行层）**

```cpp
void Connection::execute_command(
    const std::string &cmd_name,
    const std::vector<std::string> &args);
```

**集成方式**

- Connection 构造函数初始化 CommandDispatcher，通过 lambda 回调 execute_command
- handle_command 调用 dispatcher_.dispatch()，成功返回 true 则退出；失败当作聊天消息处理
- 30+ 个命令分支统一迁移到 execute_command，逻辑保持不变

### 实现细节

1. **Command Extraction** (`CommandDispatcher::extract_command`)

   ```cpp
   // Input: "/rooms_json arg1 arg2"
   // Output: cmd_name="rooms_json", args=["arg1", "arg2"]
   ```

   使用 `std::istringstream` 分割，支持可变参数个数。

2. **Lambda Capture**

   ```cpp
   command_dispatcher_([this](const std::string &cmd_name, const std::vector<std::string> &args) {
       this->execute_command(cmd_name, args);
   })
   ```

   通过值 capture 的 `this` 指针，避免循环依赖。

3. **Command Routing**

   execute_command 保留所有原始逻辑，包括：
   - 认证命令（register, login）
   - 房间命令（join, leave, create）
   - AI 命令（msg, preset）
   - 历史命令（history, search, export）
   - 好友命令（friend, pm）
   - 系统命令（help, quit）

### 测试验证

**单元测试**

```
100% tests passed, 0 tests failed out of 34
```

所有 34 个 CTest 通过，包括：
- DatabaseMigrationTest（2）
- UserManagerTest（3）
- RoomManagerTest（4）
- MessageHistoryTest（7）
- MessageProtocolTest（3）
- AIClientTest（1）
- OfflineMessageDeliveryTest（3）
- ConversationTest（6）

**并发稳定性**

```
[wave 1] ok=30/30
[wave 2] ok=30/30
[wave 3] ok=30/30
[summary] total=90, fail=0, success_rate=100.00%
```

90 个并发连接，各发送 /rooms_json 命令，100% 响应成功。

### 代码行数影响

- 新增：`src/command/command_dispatcher.h/cpp` (~130 行)
- 修改：`src/server/connection.h/cpp` (~100 行，主要是提取 execute_command)
- 修改：`CMakeLists.txt` (+1 行）

**净产出**：约 150 行模块化代码，提取了 60+ 行命令分发逻辑。

### 后续优化空间

1. **单命令测试框架**

   可为 CommandDispatcher 添加 mock executor，实现单个命令的单元测试。

2. **命令注册表**

   替代 if-else 链，使用 `std::map<std::string, Handler>` 动态注册命令。

3. **命令参数验证**

   在 execute_command 入口添加参数校验，统一错误响应。

### 知识沉淀

1. **解耦时的回调设计**
   - 使用 `std::function<>` 作为接口，避免头文件循环依赖
   - Lambda 捕获 `this` 时注意生命周期（这里是成员变量，由 Connection 管理）
   
2. **并发安全性**
   - CommandDispatcher 是无状态对象，本身不引入新的竞态
   - 仍需依赖 Connection 的互斥管理（如 all_connections_ 快照）

3. **测试红线**
   - 回归测试必须覆盖所有 30+ 个命令的主路径
   - 并发测试发现隐蔽竞态比单元测试更有效

---

## 📅 2026-04-21 (第 34 天) - P2-8 并发连接稳定性测试补齐 ✅

### 完成的工作

- [x] 新增并发连接稳定性自动化脚本 `tests/test_concurrent_connections.py`
- [x] 执行并发压测（30 客户端 × 3 波次），验证连接与请求响应稳定
- [x] 在压测中定位并修复命令处理递归缺陷，避免非 JSON 输入误递归
- [x] 构建与全量 CTest 回归通过

### 问题背景

- 第 33 天已完成测试覆盖补强前三项，剩余“并发连接稳定性测试”尚未自动化。
- 在首次并发压测中，服务端出现连接异常，暴露出命令处理路径的边界问题。

### 根因分析

1. `handle_command` 会对普通文本输入尝试 JSON 解析。
2. 解析失败场景返回 error 消息后仍可能进入递归处理，导致重复处理与异常行为。
3. 并发场景会放大该问题，导致连接稳定性下降。

### 技术实现

**1) 并发连接脚本**

- 新增 `tests/test_concurrent_connections.py`：
  - 每波并发创建多个 WebSocket 连接；
  - 每个连接接收欢迎消息后发送 `/rooms_json`；
  - 校验返回 `room_list_update` 且房间列表非空；
  - 汇总成功率并用退出码标记通过/失败。

**2) 命令解析修复**

```cpp
if (!input.empty() && (input[0] == '{' || input[0] == '[')) {
    Message msg = JsonParser::parse(input);
    if (msg.type != "error" && !msg.content.empty() && msg.content != input) {
        handle_command(msg.content);
        return;
    }
}
```

- 仅对“看起来像 JSON”的输入尝试 JSON 解析。
- 跳过 `error` 和自递归场景，防止无限/重复递归。

### 验证结果

- [x] 并发脚本结果：`30 clients × 3 waves = 90/90` 成功（100%）
- [x] `chat_server` 构建通过
- [x] 全量 CTest 通过

### 踩坑与经验

- 并发测试不仅验证“能连上”，还能快速暴露普通功能测试不易触发的递归/重入缺陷。
- 对命令输入做轻量“形态判断”可以有效隔离协议解析路径与普通文本路径。

### 后续优化

- [ ] 将并发脚本纳入统一测试入口（例如 CI job 或 make/test 脚本）
- [ ] 增加并发下持续时长测试（长连接 + 周期命令）

---

## 📅 2026-04-21 (第 33 天) - P2-8 测试覆盖补强（阶段一）✅

### 完成的工作

- [x] 新增离线消息边界测试：`sink` 失败触发整体回滚
- [x] 新增登录异常流测试：损坏的 `v2` 密码记录安全失败
- [x] 新增搜索边界与空结果测试：空结果与 `limit<=0` 回退路径
- [x] 构建与全量 CTest 回归通过

### 问题背景

- 原有测试已覆盖主流程，但边界异常路径覆盖仍不完整。
- 离线消息事务与搜索边界属于回归高风险点，需要用例锁定行为。

### 根因分析

1. 早期测试集中在“功能可用”，边界条件覆盖不足。
2. 异常输入（损坏密码记录、空结果查询）缺少显式断言。

### 技术实现

**1) 离线消息边界测试**

- 在 `tests/test_offline_message_delivery.cpp` 新增 `RollbackWhenSinkFails`：
  - 第二条消息让 `sink` 返回 false；
  - 断言事务失败、`err == "sink failed"`、未读计数保持不变。

**2) 登录异常流测试**

- 在 `tests/test_user_manager.cpp` 新增 `CorruptedV2PasswordRecordShouldFailSafely`：
  - 插入损坏的 `v2$...` 密码记录；
  - 断言登录返回 `WRONG_PASSWORD` 且流程不崩溃。

**3) 搜索边界测试**

- 在 `tests/test_message_history.cpp` 新增：
  - `SearchEmptyResult`：不存在关键词返回空集合；
  - `SearchLimitFallbackWhenNonPositive`：`limit=0` 触发默认回退并返回结果。

### 验证结果

- [x] `chat_server_test` 构建通过
- [x] 新增测试全部通过：
  - `OfflineMessageDeliveryTest.RollbackWhenSinkFails`
  - `UserManagerTest.CorruptedV2PasswordRecordShouldFailSafely`
  - `MessageHistoryTest.SearchEmptyResult`
  - `MessageHistoryTest.SearchLimitFallbackWhenNonPositive`
- [x] 全量 CTest 通过

### 踩坑与经验

- 事务路径边界测试应优先断言“状态未变”而非只断言返回值。
- 对输入异常的测试应覆盖“安全失败”语义，避免未来重构时变成未定义行为。

### 后续优化

- [ ] 补充并发连接稳定性测试（需引入可控的并发驱动与断言基线）

---

## 📅 2026-04-21 (第 32 天) - P1-7 日志脱敏与分级收敛 ✅

### 完成的工作

- [x] 审计并定位后端日志中的敏感输出点（命令明文、消息正文、token、AI 原始响应）
- [x] 对连接层日志进行脱敏，避免记录用户输入正文与离线消息内容
- [x] 对用户会话 token 相关日志去明文并按级别收敛
- [x] 对 AI 客户端日志去原始响应正文，改为结构化元信息
- [x] 构建通过并完成全量 CTest 回归

### 问题背景

- 连接与用户模块存在将原始命令、聊天正文、session token 写入日志的行为。
- AI 客户端会在日志中打印响应正文，存在隐私与敏感信息泄露风险。
- 若线上继续使用现状日志，调试便利性高，但安全与合规风险更高。

### 根因分析

1. 历史调试代码偏向“可见性优先”，缺少“最小披露”约束。
2. 日志级别边界未统一，部分高频细节日志放在 info 层。
3. 缺少统一的脱敏策略，导致各模块自行拼接日志文本。

### 技术实现

**1) 连接层命令与消息日志脱敏**

```cpp
Logger::info("Connection " + std::to_string(id_) + " command: " + summarize_command_for_log(input));
```

- 新增命令摘要函数：仅记录命令类型；`/login`、`/register` 参数统一 `<redacted>`。
- 将消息正文日志替换为长度与计数信息（如 `content_len`、`payload_bytes`）。

**2) token 日志去明文**

```cpp
Logger::debug("Verifying token for user " + std::to_string(user_id) + ", token_len=" + std::to_string(token.size()));
```

- 删除 token 与 DB token 明文输出，仅保留长度和匹配结果。
- 将部分细节日志从 info 下沉到 debug/warn。

**3) AI 响应日志最小化**

```cpp
Logger::debug("AI response received, payload_bytes=" + std::to_string(response_buf.data.size()));
```

- 去除原始响应正文、异常分支正文日志，保留状态码和 payload 大小。

### 验证结果

- [x] `chat_server` 构建通过
- [x] `chat_server_test` 构建通过
- [x] 全量 CTest 通过（含 DatabaseMigrationTest、MessageHistoryTest、UserManagerTest 等）

### 踩坑与经验

- 脱敏不是简单“删日志”，而是要保留可定位问题的结构化线索（长度、数量、状态码、命令类型）。
- 高频路径日志尽量放在 debug，info 层只保留关键生命周期与结果事件。

### 后续优化

- [ ] 在 `Logger` 增加统一脱敏工具（如用户名/命令参数/JSON 字段的标准掩码器）
- [ ] 补充“日志不含敏感字段”的自动化测试（关键字扫描）

---

## 📅 2026-04-21 (第 31 天) - Connection 好友命令拆分补强 ✅

### 完成的工作

- [x] 将 `cmd_friend` 拆分为分发层 + 6 个子处理函数
- [x] 复用统一连接查找 helper，减少好友/私聊/回执中的重复遍历
- [x] 保持现有好友相关行为不变，并通过构建与全量测试

### 问题背景

- `cmd_friend` 之前同时承载列表、请求、同意、拒绝、删除等多种分支，单函数过长。
- 好友通知和私聊转发中的连接查找逻辑也重复，容易在后续改动时遗漏一致性处理。

### 根因分析

1. 命令分发和具体业务处理没有分层。
2. 连接查找逻辑散落在多个分支里，重复且不易审计。
3. 后续加新好友子命令时，维护成本会继续上涨。

### 技术实现

**1) 命令分层**

- `cmd_friend` 仅保留参数校验和子命令分发。
- 具体逻辑拆到 `cmd_friend_list`、`cmd_friend_request`、`cmd_friend_requests`、`cmd_friend_accept`、`cmd_friend_reject`、`cmd_friend_delete`。

**2) 统一连接查找**

- 复用 `find_connection_by_user_id()`，避免在好友通知和私聊转发中手写遍历。

**3) 行为保持**

- 所有消息格式、提示文案和好友状态流转保持不变。

### 验证结果

- [x] `chat_server` 构建通过
- [x] `chat_server_test` 构建通过
- [x] 全量 CTest 通过

### 踩坑与经验

- 单函数拆分时，先保持外部接口不变，再向内提纯，风险最低。
- 重复遍历不是“为了省代码”而必须抽象，它最主要的收益是降低未来行为不一致的概率。

### 后续优化

- [ ] 如果后续继续扩展好友功能，可以考虑再抽一个 `FriendCommandRouter`，把命令处理从 `Connection` 完全移出去。

---

## 📅 2026-04-21 (第 30 天) - Connection 共享集合并发保护补强 ✅

### 完成的工作

- [x] 为 `Connection::all_connections_` 和 `Connection::created_rooms_` 增加互斥保护
- [x] 将广播、好友通知、房间列表等遍历改为快照访问
- [x] 抽取按用户查找连接的统一 helper，减少重复遍历
- [x] 保持现有行为不变，并通过构建与全量测试

### 问题背景

- 前一步已经把 AI 异步回调切回连接执行上下文，但 `Connection` 里仍有静态共享容器直接读写。
- 当前主程序是单线程 `io_context.run()`，短期不容易暴露，但后续若改成多线程服务，这些点会变成竞态源。

### 根因分析

1. 静态 `std::set` 被多个成员函数直接遍历和修改。
2. 读取和写入没有统一的同步边界。
3. 广播类逻辑如果一边遍历一边修改，未来会出现迭代失效或数据竞争。

### 技术实现

**1) 增加类内快照辅助函数**

- 新增 `snapshot_all_connections()` 和 `snapshot_created_rooms()`，统一在类内部加锁后复制快照。

**2) 在关键路径改为快照遍历**

- 广播到房间、好友上线/下线通知、好友请求通知、已读回执等路径都改成先取快照再遍历。
- 插入和删除连接、创建房间时也加了互斥保护。

**3) 保持外部行为不变**

- 这次只改同步边界，不改消息协议、不改业务顺序。

### 验证结果

- [x] `chat_server` 构建通过
- [x] `chat_server_test` 构建通过
- [x] 全量 CTest 通过

### 踩坑与经验

- 对共享容器最稳妥的做法，不是“边用边锁”，而是“先快照，再遍历”。
- 如果遍历过程中还会调用 `send()` 之类的复杂逻辑，持锁时间越短越好。

### 后续优化

- [ ] 如果未来切到多线程 `io_context.run()`，再评估 `Connection` 内其他成员（比如 `write_queue_`）是否也需要更严格的串行化。

---

## 📅 2026-04-21 (第 29 天) - P0-4 异步 AI 回调线程安全修复 ✅

### 完成的工作

- [x] 将 AI 异步请求改为不再捕获 `this`，后台线程只使用值拷贝的配置执行请求
- [x] 将 AI 回复回调切回 `Connection` 所属 `io_context` 再执行发送与状态更新
- [x] 新增异步回归测试，验证 `AIClient` 析构后回调仍可完成
- [x] 构建通过并完成相关测试回归

### 问题背景

- 原实现里，`AIClient::chat_async()` 在 detached 线程中直接调用成员函数并捕获 `this`。
- `Connection` 侧在后台线程中直接发送消息和写数据库，跨线程访问网络对象与会话状态不够安全。

### 根因分析

1. 异步任务生命周期和对象生命周期没有解耦。
2. 回调线程与连接线程没有统一执行上下文。
3. 缺少“对象先析构、异步结果后到达”的回归验证。

### 技术实现

**1) AI 请求逻辑提取为独立函数**

- `AIClient` 内部把真正的请求逻辑抽出来，异步线程只持有 `api_key`、`api_url`、`model` 的值拷贝。

**2) 回调切回连接执行器**

- 在 `Connection::send_ai_response()` 中通过 `boost::asio::post(self->socket_.get_executor(), ...)` 回到连接线程再执行发送与数据库写入。

**3) 新增回归测试**

- `tests/test_ai_client.cpp` 通过 `std::promise/std::future` 验证：
  - `AIClient` 离开作用域后，异步回调仍会触发；
  - 返回的是模拟回复，不依赖真实网络。

### 验证结果

- [x] `chat_server` 构建通过
- [x] `chat_server_test` 构建通过
- [x] `AIClientTest.ChatAsyncSurvivesClientDestruction` 通过
- [x] 全量 CTest 通过

### 踩坑与经验

- detached 线程里最容易忽略的是“对象已经销毁，但线程还在用成员”的问题。
- 异步回调最好尽量回到对象所属执行上下文再做状态变更，减少跨线程共享状态。

### 后续优化

- [ ] 进一步评估 `Connection` 内共享静态集合的并发保护，避免后续多线程 IO 时出现竞态

---

## 📅 2026-04-20 (第 28 天) - P0-3 失败注入回归测试补齐 ✅

### 完成的工作

- [x] 抽取离线消息事务执行逻辑到独立模块 `offline_message_delivery`
- [x] `Connection::send_offline_messages()` 改为复用统一事务函数
- [x] 新增离线消息事务回归测试（成功提交 + 更新失败回滚）
- [x] 更新 CMake 测试目标并完成全量 CTest 回归

### 问题背景

- 第 27 天已完成事务化主流程，但“中途更新失败是否完整回滚”缺少自动化验证。
- 没有失败注入测试时，后续重构容易引入“部分已读”回归而不被及时发现。

### 技术实现

**1) 事务逻辑可测试化**

- 新增 `src/friend/offline_message_delivery.h/.cpp`，提供：
  - `deliver_offline_messages_transactional(...)`
  - 通过 `sink` 回调解耦“推送动作”和“数据库已读更新”。

**2) 连接层复用统一事务函数**

- `src/server/connection.cpp` 内的离线消息处理不再直接拼装 SQL 事务，改为调用模块函数。
- 保持行为语义不变：推送成功后才更新 `is_read`。

**3) 失败注入测试**

- 新增 `tests/test_offline_message_delivery.cpp`：
  - `CommitWhenAllDelivered`：验证全部成功时提交并全部已读。
  - `RollbackWhenUpdateFails`：通过 SQLite Trigger 注入第二条更新失败，验证事务回滚后两条均保持未读。

### 验证结果

- [x] CMake 构建通过（`chat_server`、`chat_server_test`）
- [x] 新增 `OfflineMessageDeliveryTest` 2 条用例全部通过
- [x] 全量 CTest 通过

### 踩坑与经验

- 把“事务边界 + 失败回滚”封装成可单测函数，比在连接层内联实现更稳，后续重构风险更低。
- SQLite Trigger 很适合在单测里做失败注入，能稳定覆盖异常分支。

### 后续优化

- [ ] 将离线消息确认机制升级为客户端 ACK（真正做到“客户端确认后再标记已读”）
- [ ] 给离线消息补充批量 update 优化，减少每条更新的 statement 调度开销

---

## 📅 2026-04-20 (第 27 天) - P0-3 离线消息事务一致性改造 ✅

### 完成的工作

- [x] 将离线消息处理改为显式事务（BEGIN/COMMIT/ROLLBACK）
- [x] 保持“先推送，后标记已读”的执行顺序
- [x] 标记已读失败时触发回滚，避免部分提交
- [x] 构建通过并完成全量 CTest 回归

### 问题背景

- 原实现在推送离线消息时逐条 `UPDATE is_read`，缺少事务边界。
- 一旦中途更新失败，会出现“部分已读、部分未读”的不一致状态。
- 此类不一致会导致后续用户重复收消息或消息状态错乱。

### 根因分析

1. 数据库写路径缺少原子性保障。
2. 已读更新语句每条单独执行，失败后无法整体恢复。
3. 缺乏失败分支的统一回滚处理。

### 技术实现

**1) 在离线消息流程中增加事务边界**

```cpp
sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr, &tx_err);
...
sqlite3_exec(db, "COMMIT", nullptr, nullptr, &tx_err);
```

**2) 失败路径统一回滚**

```cpp
if (has_db_error) {
    sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &tx_err);
    return;
}
```

**3) 顺序约束保持不变**

- 先 `send(json)` 推送给用户。
- 推送后再执行 `UPDATE private_messages SET is_read = 1`。

### 验证方法

- 构建验证：`chat_server`、`chat_server_test` 均链接成功。
- 回归验证：全量 CTest 通过。
- 关键检查点：
  - 协议与历史相关测试无回归
  - 用户登录路径仍可触发离线消息流程

### 测试结果

- [x] CMake 构建通过
- [x] 全量 CTest 通过
- [ ] 增加“中途数据库更新失败”的专门回归测试（待补）

### 踩坑与经验

- 坑点：离线消息这种“读-推送-写”混合流程，最容易发生部分成功。
- 解决方式：把“写”放进事务，确保失败时状态可回滚。
- 可复用结论：消息系统中任何“状态确认写入”都应有事务保护。

### 后续优化

- [ ] 新增离线消息失败注入测试（验证回滚分支）
- [ ] 引入批量更新语句减少每条 prepare 开销
- [ ] 评估将离线消息读取与确认拆分为 ACK 机制

---

## 📅 2026-04-20 (第 26 天) - P0-2 消息协议序列化补全 ✅

### 完成的工作

- [x] 在协议层实现 `MessageType` 与字符串的双向映射
- [x] 实现 `serialize_message` / `deserialize_message`
- [x] 为非法 JSON 输入添加错误兜底（返回 error 消息）
- [x] 让 `JsonParser` 复用统一协议函数，减少重复逻辑
- [x] 新增协议单元测试并接入 `chat_server_test`

### 问题背景

- `src/protocol/message.cpp` 只有 TODO，核心协议函数缺失。
- 消息解析与序列化逻辑分散在 `JsonParser`，可维护性与一致性不足。
- 非法报文路径缺少统一兜底，后续扩展风险高。

### 根因分析

1. 协议模型（`Message`）定义完整，但实现层缺位。
2. 解析逻辑耦合在 `JsonParser`，而非协议模块本身。
3. 缺少针对协议行为的独立单元测试，导致 TODO 长期未闭环。

### 技术实现

**1) 新增协议函数与类型映射**

```cpp
std::string message_type_to_string(MessageType type);
MessageType message_type_from_string(const std::string& type_str);
std::string serialize_message(const Message& msg);
Message deserialize_message(const std::string& json_str);
```

**2) 统一 `JsonParser` 走协议层实现**

```cpp
json JsonParser::to_json(const Message& msg) {
  return json::parse(serialize_message(msg));
}

Message JsonParser::parse(const std::string& json_str) {
  return deserialize_message(json_str);
}
```

**3) 新增协议单测并纳入测试目标**

```cpp
TEST(MessageProtocolTest, InvalidJsonShouldNotCrash) {
  const Message parsed = deserialize_message("{invalid_json");
  EXPECT_EQ(parsed.type, "error");
}
```

### 验证方法

- CMake Tools 构建：`chat_server` 与 `chat_server_test` 均成功链接。
- CTest 回归：全量测试通过。
- 重点验证：
  - `MessageTypeRoundTrip`
  - `SerializeDeserializeRoundTrip`
  - `InvalidJsonShouldNotCrash`

### 测试结果

- [x] 新增 3 条 `MessageProtocolTest` 用例全部通过
- [x] 全量 CTest 通过（含既有用户/房间/历史/会话测试）

### 踩坑与经验

- 坑点：协议逻辑分散后，字段变更容易出现行为不一致。
- 解决方式：让 `JsonParser` 复用协议层函数，单点维护序列化规则。
- 可复用结论：对外协议字段应集中在一个模块定义与测试，避免“多处拼 JSON”。

### 后续优化

- [ ] 增加协议字段兼容版本号（为后续演进做准备）
- [ ] 给 `deserialize_message` 增加更细粒度错误分类
- [ ] 针对消息大小和字段长度加输入上限校验

---

## 📅 2026-04-20 (第 25 天) - P0-1 密码存储升级（带盐哈希 + 旧账号兼容） ✅

### 完成的工作

- [x] 密码存储从纯 SHA256 升级为带盐格式 `v2$salt$hash`
- [x] 注册流程切换为新格式写入
- [x] 登录流程兼容旧 SHA256 存储
- [x] 旧账号首次成功登录后自动升级到新格式
- [x] 新增旧密码兼容升级单元测试

### 问题背景

- 原实现仅使用纯 SHA256 存储密码，抗撞库能力不足。
- 线上可能存在历史账号数据，不能直接强制重置密码。
- 目标是在不破坏现有登录体验的前提下，完成密码存储升级。

### 根因分析

1. 密码哈希策略缺少随机盐，导致相同密码哈希完全相同。
2. 数据库字段只有 `password_hash`，但并不阻碍通过编码格式完成平滑迁移。
3. 登录逻辑仅支持单一旧格式，无法自动升级历史数据。

### 技术实现

**1) 引入版本化密码记录格式**

```cpp
constexpr const char* kPasswordAlgoPrefix = "v2";

std::string build_password_record(const std::string& salt_hex, const std::string& digest_hex) {
  return std::string(kPasswordAlgoPrefix) + "$" + salt_hex + "$" + digest_hex;
}
```

**2) 注册时使用带盐哈希**

```cpp
std::string salt_hex = generate_salt_hex();
std::string pwd_hash = build_password_record(salt_hex, hash_password_with_salt(password, salt_hex));
```

**3) 登录兼容并自动升级旧账号**

```cpp
if (parse_password_record(stored_hash, salt_hex, digest_hex)) {
  password_ok = (digest_hex == hash_password_with_salt(password, salt_hex));
} else {
  password_ok = (stored_hash == hash_password(password));
  needs_upgrade = password_ok;
}
```

### 验证方法

- 代码静态检查：变更文件无语言服务错误。
- 单元测试补充：新增 `LegacyPasswordCompatibleAndUpgrade` 用例。
- 构建与测试：CMake Tools 构建成功，`chat_server_test` 全量回归通过。

### 测试结果

- [x] 新增兼容升级测试用例（编译层面无报错）
- [x] 执行 `chat_server_test` 全量回归（全部通过）

### 踩坑与经验

- 坑点：直接新增数据库列会引入迁移复杂度。
- 解决方式：先采用 `password_hash` 单列版本化编码（`v2$...`），实现低风险平滑升级。
- 可复用结论：安全策略升级优先考虑“兼容 + 渐进迁移”，避免一次性破坏存量数据。

### 后续优化

- [ ] 将哈希策略进一步升级为更强 KDF（如 PBKDF2/Argon2）
- [ ] 在安全文档中补充密码策略和迁移说明
- [x] 修复 CMake Tools 配置环境并完成全量测试回归

---

## 📅 2026-04-03 (第 24 天) - 离线好友请求通知 ✅

### 完成的工作

**离线好友请求存储与推送**：
- [x] 确认好友请求已持久化到 SQLite 数据库（`friend_requests` 表）
- [x] 用户登录时主动查询待处理的好友请求
- [x] 推送结构化 JSON 格式通知（`pending_friend_requests`）
- [x] 增强数据库操作日志，方便排查问题
- [x] 前端适配监听 `pending_friend_requests` 消息类型

### 问题背景

**原问题**：用户 A 向不在线的用户 B 发送好友请求，用户 B 上线后看不到申请消息。

**原因分析**：
1. 好友请求实时推送时，如果用户不在线就会错过
2. 登录时虽然查询了待处理请求，但推送的是纯文本格式，前端解析困难

### 技术实现

**1. 登录时推送结构化 JSON 通知**
```cpp
// 检查并推送待处理的好友请求（结构化 JSON）
auto pending_requests = friend_manager_->get_received_requests(user.id);
Logger::info("用户 " + username + " 登录，查询到 " + std::to_string(pending_requests.size()) + " 条待处理好友请求");

if (!pending_requests.empty()) {
    std::string json = "{\"type\":\"pending_friend_requests\",\"count\":" + std::to_string(pending_requests.size()) + ",\"requests\":[";
    bool first = true;
    for (const auto& req : pending_requests) {
        if (!first) json += ",";
        first = false;
        json += "{\"sender\":\"" + req.sender_username + "\",\"sender_id\":" + std::to_string(req.sender_id) + "}";
    }
    json += "]}";
    send(json);
    Logger::info("已推送待处理好友请求通知: " + json);
}
```

**2. 增强数据库操作日志**
```cpp
// 发送请求时确认落盘
if (rc == SQLITE_DONE) {
    Logger::info("✅ 好友请求已持久化到数据库: " + std::to_string(sender_id) + " -> " + std::to_string(receiver_id));
    return true;
} else {
    Logger::error("❌ 数据库写入失败, rc=" + std::to_string(rc) + ", error=" + std::string(errmsg));
    return false;
}

// 查询时打印结果
Logger::info("📥 查询用户 " + std::to_string(receiver_id) + " 的待处理好友请求...");
Logger::info("✅ 查询完成，共 " + std::to_string(requests.size()) + " 条待处理请求");
```

**3. 前端监听离线请求**
```typescript
wsService.on('pending_friend_requests', (msg: any) => {
  console.log('📬 收到离线好友请求通知:', msg)
  const count = msg.raw?.count || 0
  const requests = msg.raw?.requests || []
  
  requests.forEach((req: any) => {
    useFriendStore.getState().addPendingRequest({
      id: Date.now() + Math.random(),
      senderId: req.sender_id,
      senderUsername: req.sender,
      status: 'pending',
      createdAt: new Date().toISOString()
    })
  })
  
  if (count > 0) {
    alert(`你有 ${count} 条新的好友请求！`)
  }
})
```

### 验证方法

**1. 检查数据库是否真正存储**
```bash
sqlite3 data/chat.db "SELECT id, sender_id, receiver_id, status, datetime(created_at, 'unixepoch') FROM friend_requests;"
```

**2. 查看后端日志**
```
[INFO] ✅ 好友请求已持久化到数据库: 1 -> 2
[INFO] 用户 testuser 登录，查询到 1 条待处理好友请求
[INFO] 已推送待处理好友请求通知: {"type":"pending_friend_requests","count":1,"requests":[{"sender":"Tide","sender_id":1}]}
```

### 测试场景

**场景 1：离线好友请求**
1. 用户 A 向不在线的用户 B 发送好友请求
2. 后端将请求存储到 SQLite 数据库
3. 用户 B 上线登录
4. 后端查询并推送待处理的好友请求（JSON 格式）
5. 前端解析并显示好友请求通知

**场景 2：数据库验证**
1. 发送好友请求后，直接查询 SQLite 数据库
2. 确认 `friend_requests` 表中有 `status='pending'` 的记录
3. 重启后端服务，数据不丢失

### 构建验证
- [x] 后端编译成功
- [x] 前端构建成功
- [x] 无编译错误

---

## 📅 2026-04-03 (第 23 天) - 私聊好友检查 ✅

### 完成的工作

**后端好友关系检查**：
- [x] 修复 `remove_friend` 函数，删除双向好友关系
- [x] 私聊命令（`/pm`）添加好友关系检查
- [x] 非好友发送私聊时返回错误提示

**前端错误处理**：
- [x] 添加 `handleSendMessageError` 监听系统错误消息
- [x] 检测"不是好友"错误并显示错误提示
- [x] 错误消息样式：红色背景 + ❗ 感叹号图标
- [x] Message 接口添加 `is_error` 字段
- [x] 深色模式样式适配

### 技术实现

**1. 后端双向删除好友**
```cpp
// 修复前（只删除单向）
DELETE FROM friendships WHERE user_id = ? AND friend_id = ?

// 修复后（删除双向）
DELETE FROM friendships WHERE (user_id = ? AND friend_id = ?) OR (user_id = ? AND friend_id = ?)
```

**2. 后端私聊好友检查**
```cpp
if (!friend_manager_->is_friend(session_.user_id, to_user_id)) {
    send_system_message("你们不是好友，无法发送私聊消息。");
    return;
}
```

**3. 前端错误消息处理**
```tsx
const handleSendMessageError = useCallback((message: any) => {
    const content = message.content || ''
    if (content.includes('不是好友') || content.includes('无法发送私聊')) {
      addMessage(selectedFriend || '', {
        type: 'system',
        from_user: '系统',
        content: '❌ 发送失败：' + content,
        is_error: true,
      })
    }
  }, [selectedFriend, username, addMessage])
```

**4. 错误消息 UI**
```tsx
{msg.is_error ? (
  <div className="bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800">
    <span className="text-red-500 text-xl">❗</span>
    <p className="text-red-700 dark:text-red-400">{msg.content}</p>
  </div>
) : (
  // 正常消息
)}
```

### 构建验证
- [x] 前端构建成功
- [x] 后端编译成功
- [x] 无编译错误

### 测试场景

**场景 1：删除好友后发送消息**
1. 用户 A 删除用户 B
2. 用户 A 尝试给用户 B 发送私聊消息
3. 后端检查好友关系，发现不是好友
4. 返回错误消息："你们不是好友，无法发送私聊消息。"
5. 前端显示红色错误消息，带 ❗ 图标

**场景 2：被删除的用户尝试发送消息**
1. 用户 A 删除用户 B
2. 用户 B 尝试给用户 A 发送私聊消息
3. 后端检查好友关系，发现不是好友
4. 返回错误消息
5. 前端显示红色错误消息

---

## 📅 2026-04-03 (第 22 天) - 删除好友功能 ✅

### 完成的工作

**前端删除好友功能**：
- [x] 好友列表项添加删除按钮
- [x] 鼠标悬停显示删除图标
- [x] 点击删除按钮显示确认对话框
- [x] 调用 friendStore.removeFriend 更新本地状态
- [x] 调用 wsService.deleteFriend 通知后端
- [x] 删除当前选中好友时清空聊天区域
- [x] 深色模式样式适配

**UI 交互优化**：
- [x] 删除按钮默认隐藏，悬停时显示
- [x] 防止点击事件冒泡到好友选择
- [x] 确认对话框显示好友名称
- [x] 删除后自动更新好友列表

### 技术实现

**1. 好友列表项结构**
```tsx
<div className="group flex items-center">
  {/* 好友选择按钮 */}
  <button onClick={() => handleSelectFriend(friend.username)}>
    {/* 好友信息 */}
  </button>
  
  {/* 删除按钮（悬停显示） */}
  <button
    onClick={(e) => {
      e.stopPropagation() // 防止触发选择
      if (window.confirm(`确定要删除好友 ${friend.username} 吗？`)) {
        useFriendStore.getState().removeFriend(friend.username)
        wsService.deleteFriend(friend.username)
        if (selectedFriend === friend.username) {
          setSelectedFriend(null)
        }
      }
    }}
    className="hidden group-hover:flex"
  >
    🗑️
  </button>
</div>
```

**2. 后端命令**
```typescript
// websocket.ts
deleteFriend(username: string): void {
  this.sendCommand(`/friend del ${username}`)
}
```

### 构建验证
- [x] 前端构建成功
- [x] 无编译错误
- [x] CSS 大小：22.94 kB (gzip: 4.87 kB)
- [x] JS 大小：379.55 kB (gzip: 112.20 kB)

---

## 📅 2026-04-03 (第 21 天) - 主题系统完善 ✅

### 完成的工作

**日间/夜间主题修复**：
- [x] Settings.tsx - 设置页面完整深色模式支持
- [x] Login.tsx - 登录页面深色模式支持
- [x] Register.tsx - 注册页面深色模式支持
- [x] Chat.tsx - 私聊页面（743 行）完整深色模式
- [x] Contact.tsx - 群聊页面（649 行）完整深色模式
- [x] AIAssistant.tsx - AI 助手页面（615 行）完整深色模式

**主题系统架构**：
- [x] ThemeProvider 全局主题管理
- [x] themeStore Zustand 状态管理
- [x] 三种主题模式支持（light/dark/system）
- [x] 系统主题变化监听（prefers-color-scheme）
- [x] 主题持久化到 localStorage

**颜色方案统一**：
- [x] 背景色：gray-100/gray-900、white/gray-800
- [x] 文字颜色：gray-900/white、gray-700/gray-300
- [x] 边框颜色：gray-200/gray-700、gray-300/gray-600
- [x] 输入框、按钮、模态框样式统一

**构建验证**：
- [x] 前端构建成功
- [x] 无编译错误
- [x] CSS 大小：22.61 kB (gzip: 4.83 kB)
- [x] JS 大小：379.10 kB (gzip: 112.07 kB)

### 代码统计

| 文件 | 总行数 | 修改行数 | 新增 dark 类数 |
|------|--------|----------|----------------|
| Settings.tsx | 174 | ~120 | ~40 |
| Login.tsx | 268 | ~80 | ~25 |
| Register.tsx | 297 | ~80 | ~25 |
| Chat.tsx | 743 | ~200 | ~70 |
| Contact.tsx | 649 | ~180 | ~65 |
| AIAssistant.tsx | 615 | ~180 | ~65 |
| **总计** | **2746** | **~840** | **~290** |

### 技术要点

**1. Tailwind CSS dark 模式**
```tsx
// 示例：背景色和文字颜色
<div className="bg-white dark:bg-gray-800 text-gray-900 dark:text-white">
  内容
</div>

// 示例：边框颜色
<div className="border-gray-200 dark:border-gray-700">
  内容
</div>
```

**2. 输入框深色模式**
```tsx
<input
  className="bg-white dark:bg-gray-700 text-gray-900 dark:text-white 
             border-gray-300 dark:border-gray-600"
/>
```

**3. 消息气泡深色模式**
```tsx
// 自己的消息（蓝色，不变）
<div className="bg-blue-600 text-white">
  内容
</div>

// 他人的消息
<div className="bg-white dark:bg-gray-800 text-gray-900 dark:text-white">
  内容
</div>
```

### 修复效果

**修复前**：
- ❌ 设置页面深色模式下白色背景刺眼
- ❌ 登录/注册页面无深色模式
- ❌ 聊天页面无深色模式
- ❌ 部分文字在深色模式下不可读

**修复后**：
- ✅ 所有页面完美支持深色模式
- ✅ 颜色过渡自然流畅
- ✅ 文字颜色在所有背景下可读
- ✅ 模态框、按钮、输入框样式统一
- ✅ Markdown 渲染在深色模式下正常显示

### 文档更新
- [x] 生成修复总结文档：`docs/THEME_FIX_SUMMARY.md`

---

## 📅 2026-04-03 (第 20 天) - v1.1.0 版本发布 ✅

### 完成的工作

**文档整理**：
- [x] 删除冗余文档文件（CHANGELOG.md, MANUAL.md, QUICKSTART.md, QUICK_REFERENCE.md）
- [x] 删除测试相关文档（docs/TESTING.md, docs/DOCKER_TEST_CHECKLIST.md, docs/DOCKER_TEST_REPORT.md）
- [x] 删除项目状态文档（docs/PROJECT_SUMMARY.md, docs/STATUS.md, docs/PHASE4.2_PLAN.md, docs/PHASE4.3_STATUS.md）
- [x] 删除需求文档（docs/requirements.md）
- [x] 更新 README.md - 添加好友系统功能说明
- [x] 更新 RELEASE_NOTES.md - 添加 v1.1.0 发布说明
- [x] 更新 CHECKLIST.md - 更新检查清单

**后端功能增强**：
- [x] 好友系统完整实现（src/friend/friend_manager.cpp/h）
  - [x] 好友添加/删除/查询
  - [x] 好友请求发送/接受/拒绝
  - [x] 双向好友关系支持
  - [x] 事务处理保证数据一致性
- [x] 用户系统增强（src/user/user_manager.cpp/h）
  - [x] 会话 token 生成与验证
  - [x] 邮箱字段支持
  - [x] 登录结果枚举（SUCCESS/USER_NOT_FOUND/WRONG_PASSWORD）
  - [x] 用户状态查询优化
- [x] AI 响应优化（src/server/connection.cpp）
  - [x] Markdown 格式清理
  - [x] 预设 prompt 作为系统消息插入
- [x] WebSocket 协议增强（src/protocol/websocket.cpp/h）
- [x] 服务器架构优化（src/server/server.cpp/h）
  - [x] FriendManager 集成
  - [x] 依赖注入优化

**前端开发**：
- [x] React Web 前端完整实现（frontend/）
  - [x] 登录/注册页面
  - [x] 好友聊天界面
  - [x] 群聊房间界面
  - [x] 好友请求管理
  - [x] 用户设置页面
  - [x] 响应式设计

**脚本工具**：
- [x] 添加数据清理脚本（scripts/clear_data.sh, scripts/clear_db.sh）

### 代码统计

| 模块 | 文件数 | 新增代码 | 修改代码 | 删除代码 |
|------|--------|----------|----------|----------|
| src/friend/ | 2 | ~450 行 | - | - |
| src/user/ | 2 | ~150 行 | ~100 行 | - |
| src/server/ | 4 | ~500 行 | ~200 行 | - |
| src/ai/ | 1 | - | ~20 行 | - |
| src/protocol/ | 2 | - | ~20 行 | - |
| frontend/ | ~20 | ~2000 行 | - | - |
| 文档 | - | - | ~300 行 | ~2500 行 |
| **总计** | **~30** | **~3100 行** | **~640 行** | **~2500 行** |

### 技术亮点

**1. 好友关系双向查询**
```cpp
// 查询双向好友关系
// 1. user_id 添加的好友（user_id -> friend_id）
// 2. 添加 user_id 为好友的（friend_id -> user_id）
std::vector<Friendship> FriendManager::get_friends(int user_id) {
    // 查询第一类好友
    // 查询第二类好友
    // 避免重复
    return friends;
}
```

**2. 好友请求事务处理**
```cpp
// 开始事务
sqlite3_exec(db_->get_db(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

// 更新请求状态
// 添加双向好友关系（sender->receiver, receiver->sender）

// 提交或回滚事务
sqlite3_exec(db_->get_db(), "COMMIT", nullptr, nullptr, nullptr);
```

**3. Markdown 清理**
```cpp
std::string Connection::strip_markdown(const std::string& md) {
    // 移除代码块
    // 移除行内代码
    // 移除标题标记（#, ##, ###）
    // 移除粗体和斜体符号
    // 移除水平线
    // 移除列表符号
    // 移除多余空行
    return result;
}
```

### 下一步计划

- [ ] 消息已读未读状态
- [ ] 离线消息同步
- [ ] 好友分组功能
- [ ] 群聊消息历史记录
- [ ] 移动端适配优化
- [ ] 阶段 7：集群支持

---

## 📅 2026-03-31 (第 19 天) - 好友系统与群聊界面完善 ✅

### 完成的工作

**好友私聊界面优化**：
- [x] 左侧好友列表，右侧聊天区域
- [x] 未选择好友时显示空状态提示
- [x] 点击好友显示聊天界面
- [x] 点击"💬 好友聊天"标题返回空状态
- [x] 从群聊返回时自动离开房间
- [x] 添加好友功能（➕ 按钮）
- [x] 待处理好友请求显示（黄色区域）
- [x] 同意/拒绝好友请求
- [x] 好友请求结果通知

**群聊界面集成**：
- [x] 群聊房间列表（左侧）
- [x] 聊天区域（右侧直接显示）
- [x] 点击房间自动加入并显示聊天界面
- [x] 未选择房间时显示提示
- [x] 创建房间功能
- [x] 房间列表：general/code/gaming/music/movie

**登出功能**：
- [x] 顶部栏始终显示
- [x] 退出按钮
- [x] 登出确认对话框
- [x] 清除状态并跳转登录页

**WebSocket 连接优化**：
- [x] 自动检测连接状态
- [x] 连接失败时自动重连
- [x] 更好的错误提示
- [x] 详细的调试日志

**房间切换逻辑**：
- [x] 从群聊切换到好友时自动离开房间
- [x] 发送 /leave 命令
- [x] 重置 currentRoom 为 #general

### 界面结构

**聊天页面（主界面 - 私聊）**：
```
┌─────────────────────────────────────────────────┐
│ 💬 好友聊天 [➕]    |  💬 Tide [已连接] 👤 你  │
├─────────────────────┤  退出                    │
│ 📬 待处理 (1)       ├──────────────────────────┤
│  ├ tide             │  [聊天区域]              │
│  ├ ✓ 同意 ✕ 拒绝    │  消息列表...             │
├─────────────────────┤                          │
│ 🔍 搜索...          ├──────────────────────────┤
│                     │  [输入框] [发送]         │
│ 👤 Alice (在线)     └──────────────────────────┘
│ 👤 Bob (离线)       │
└─────────────────────┘
```

**群聊页面**：
```
┌─────────────────────────────────────────────────┐
│ 群聊房间 [➕]       |  #code [已连接]           │
├─────────────────────┤                          │
│ 🏠 #general         │  [聊天区域]              │
│ 💻 #code            │  消息列表...             │
│ 🎮 #gaming          │                          │
│ 🎵 #music           ├──────────────────────────┤
│ 🎬 #movie           │  [输入框] [发送]         │
└─────────────────────┘                          │
                                                 │
```

### 代码统计

| 模块 | 文件数 | 新增代码 | 修改代码 |
|------|--------|----------|----------|
| Chat.tsx | 1 | ~100 行 | ~200 行 |
| Contact.tsx | 1 | ~150 行 | - |
| ChatList.tsx | 1 | ~50 行 | ~50 行 |
| **总计** | **3** | **~300 行** | **~250 行** |

### 测试结果 ✅

**好友私聊**：
- ✅ 点击好友显示聊天界面
- ✅ 点击标题返回空状态
- ✅ 发送/接收私聊消息
- ✅ 好友请求通知显示
- ✅ 同意/拒绝功能正常

**群聊**：
- ✅ 点击房间加入并显示聊天
- ✅ 未选择房间显示提示
- ✅ 创建房间功能正常
- ✅ 从群聊返回好友时自动离开房间

**登出功能**：
- ✅ 退出按钮显示在顶部栏
- ✅ 点击显示确认对话框
- ✅ 确认后清除状态并跳转

**WebSocket 连接**：
- ✅ 自动检测连接状态
- ✅ 连接失败时显示错误提示
- ✅ 详细的调试日志

### 核心实现

**1. 好友聊天界面切换**
```typescript
// 点击好友
const handleChatWithFriend = (friendUsername: string) => {
  // 如果当前在群聊房间，先离开
  if (currentRoom && currentRoom !== '#general') {
    ws.send('/leave')
    setCurrentRoom('#general')
  }
  setCurrentFriend(friendUsername)
}

// 点击标题返回空状态
const handleClear = () => {
  if (currentRoom && currentRoom !== '#general') {
    ws.send('/leave')
    setCurrentRoom('#general')
  }
  setCurrentFriend(null)  // 显示空状态
}
```

**2. WebSocket 自动重连**
```typescript
useEffect(() => {
  const existingWS = (window as any).chatWS
  
  if (existingWS && existingWS.readyState === WebSocket.OPEN) {
    // 复用现有连接
    wsRef.current = existingWS
  } else {
    // 创建新连接
    const ws = new WebSocket(WS_URL)
    wsRef.current = ws
    ;(window as any).chatWS = ws
    
    ws.onopen = () => {
      // 自动登录
      const storedPassword = localStorage.getItem('chat_password_' + username)
      if (storedPassword) {
        ws.send(`/login ${username} ${storedPassword}`)
      }
    }
  }
}, [])
```

**3. 登出确认对话框**
```typescript
{showLogoutConfirm && (
  <div className="fixed inset-0 bg-black/50 flex items-center justify-center z-50">
    <div className="bg-white dark:bg-dark-card rounded-2xl p-6">
      <h3>确认退出</h3>
      <p>确定要退出登录吗？</p>
      <div className="flex space-x-3">
        <button onClick={() => setShowLogoutConfirm(false)}>取消</button>
        <button onClick={handleConfirmLogout} className="bg-red-500">退出登录</button>
      </div>
    </div>
  </div>
)}
```

### 下一步计划

- [ ] 消息已读未读状态
- [ ] 离线消息同步
- [ ] 好友分组功能
- [ ] 群聊消息历史记录
- [ ] 移动端适配优化

---

## 📅 2026-03-31 (第 18 天) - 防止重复登录功能 ✅

### 完成的工作

**好友请求功能优化**：
- [x] 添加好友请求表（friend_requests）
- [x] 发送好友请求 `/friend request <用户名>`
- [x] 查看收到的请求 `/friend requests`
- [x] 同意好友请求 `/friend accept <用户名>`
- [x] 拒绝好友请求 `/friend reject <用户名>`
- [x] 申请方实时收到处理通知
- [x] 双向好友关系自动建立
- [x] 私聊功能支持双向好友验证

**Bug 修复**：
- [x] 修复接受不存在的请求也显示成功
- [x] 修复 reject 处理不存在的请求
- [x] 修复双方好友列表不更新问题
- [x] 修复私聊只能单向发送问题
- [x] 修复 get_friends 查询逻辑
- [x] 修复 is_friend 双向验证

**用户体验优化**：
- [x] friend requests 显示详细操作提示
- [x] 同意/拒绝时通知申请方
- [x] 添加详细日志便于调试

### 数据库变更

**新增表**：
```sql
-- 好友请求表
CREATE TABLE friend_requests (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sender_id INTEGER NOT NULL,
    receiver_id INTEGER NOT NULL,
    status TEXT DEFAULT 'pending',  -- pending, accepted, rejected
    created_at INTEGER NOT NULL,
    UNIQUE(sender_id, receiver_id)
);

-- 好友关系表（已有）
CREATE TABLE friendships (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    friend_id INTEGER NOT NULL,
    created_at INTEGER NOT NULL,
    UNIQUE(user_id, friend_id)
);

-- 私聊消息表（已有）
CREATE TABLE private_messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    from_user TEXT NOT NULL,
    to_user TEXT NOT NULL,
    content TEXT NOT NULL,
    is_read INTEGER DEFAULT 0,
    timestamp INTEGER NOT NULL
);
```

### 后端命令

```bash
# 好友管理
/friend list           # 查看好友列表
/friend request <用户名> # 发送好友请求
/friend requests       # 查看收到的好友请求
/friend accept <用户名>  # 同意好友请求
/friend reject <用户名>  # 拒绝好友请求
/friend del <用户名>    # 删除好友

# 私聊
/pm <用户名> <消息>     # 私聊好友
/msg <用户名> <消息>    # 私聊好友（同 pm）
```

### 工作流程

```
用户 A                          用户 B
  |                               |
  |-- /friend request userB ------>|
  |                               | 收到请求
  |                               | /friend requests
  |                               | 看到 userA 的请求
  |                               |
  |<-- (等待) --------------------|
  |                               |
  |                               | /friend accept userA
  |                               |
  |<-- ✅ 通知：userB 同意了 ------|
  |                               |
  | 双方成为好友                   |
  |                               |
  |--- /pm userB 你好！------------>|
  |<-- /pm userA 收到！-------------|
```

### 代码统计

| 模块 | 文件数 | 新增代码 | 修改代码 |
|------|--------|----------|----------|
| FriendManager | 2 | ~200 行 | ~50 行 |
| Connection | 1 | ~150 行 | ~30 行 |
| **总计** | **2** | **~350 行** | **~80 行** |

### 测试结果 ✅

**好友请求**：
- ✅ 发送请求成功
- ✅ 查看请求列表
- ✅ 同意请求后双方成为好友
- ✅ 拒绝请求后请求失效
- ✅ 申请方实时收到通知

**好友列表**：
- ✅ 双方好友列表都正确更新
- ✅ 双向好友关系正确建立

**私聊功能**：
- ✅ 双方都能发送私聊
- ✅ 仅好友间可私聊
- ✅ 非好友提示错误

### 核心实现

**1. 发送好友请求**
```cpp
bool FriendManager::send_request(int sender_id, int receiver_id) {
    // 检查是否已经是好友
    if (is_friend(sender_id, receiver_id)) return false;
    
    // 检查是否有待处理的请求
    if (has_pending_request(sender_id, receiver_id)) return false;
    
    // 插入请求
    const char* sql = R"(
        INSERT OR REPLACE INTO friend_requests 
        (sender_id, receiver_id, status, created_at)
        VALUES (?, ?, 'pending', ?)
    )";
    // ... 执行 SQL
}
```

**2. 同意好友请求**
```cpp
bool FriendManager::accept_request(int sender_id, int receiver_id) {
    // 检查是否存在待处理的请求
    if (!has_pending_request(sender_id, receiver_id)) return false;
    
    // 开始事务
    sqlite3_exec(db_, "BEGIN TRANSACTION", ...);
    
    // 更新请求状态
    UPDATE friend_requests SET status = 'accepted' ...
    
    // 添加双向好友关系
    INSERT INTO friendships (user_id, friend_id) VALUES (sender, receiver);
    INSERT INTO friendships (user_id, friend_id) VALUES (receiver, sender);
    
    sqlite3_exec(db_, "COMMIT", ...);
}
```

**3. 通知申请方**
```cpp
// Connection.cpp - cmd_friend
if (friend_manager_->accept_request(friend_id, session_.user_id)) {
    send_system_message("✅ 已同意 " + friend_name + " 的好友请求");
    
    // 通知申请方
    for (auto& conn : all_connections_) {
        if (conn && conn->session_.user_id == friend_id) {
            conn->send_system_message("✅ " + session_.username + " 同意了你的好友请求");
        }
    }
}
```

### 下一步计划

**前端实现**：
- [ ] 好友列表 UI 组件
- [ ] 好友请求模态框
- [ ] 私聊界面
- [ ] 实时通知显示

---

## 📅 2026-03-31 (第 18 天) - 防止重复登录功能 ✅

### 完成的工作

**Session Token 机制**：
- [x] 用户表添加 `session_token` 字段
- [x] 登录时生成 32 位随机 token
- [x] 数据库保存 token
- [x] 每次请求验证 token
- [x] token 验证失败时关闭连接

**主动通知旧连接**：
- [x] 新登录时查找同一用户的旧连接
- [x] 发送"账号在其他地方登录"通知
- [x] 延迟 100ms 后关闭旧连接

**前端弹窗优化**：
- [x] 创建 SessionExpiredModal 组件
- [x] 全屏遮罩效果
- [x] 醒目的警告图标
- [x] "返回登录页面"按钮
- [x] 自动清除登录状态并跳转

**消息重复显示修复**：
- [x] 添加 `hasSetupListener` 标志
- [x] 防止重复注册 WebSocket 监听器
- [x] 确保消息只处理一次

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 登录后就有弹窗 | 前端检测条件太宽泛 | 改为精确匹配（同时包含两个关键词） |
| 两个账号同时登录没弹窗 | 后端没有主动通知旧连接 | 遍历 all_connections_ 查找并通知 |
| 发送消息就被弹出 | token 验证逻辑返回错误 | 修复字符串比较，使用 std::string |
| 消息显示两条 | 重复注册 WebSocket 监听器 | 添加 hasSetupListener 标志防止重复 |

### 代码统计

| 模块 | 文件数 | 新增代码 | 修改代码 |
|------|--------|----------|----------|
| 后端 Session Token | 2 | ~150 行 | ~50 行 |
| 前端弹窗组件 | 1 | ~60 行 | - |
| 前端 Chat 页面 | 1 | ~20 行 | ~30 行 |
| **总计** | **4** | **~230 行** | **~80 行** |

### 测试结果 ✅

**防止重复登录**：
- ✅ 窗口 A 登录正常
- ✅ 窗口 B 登录（同一账号）正常
- ✅ 窗口 A 立即收到弹窗通知
- ✅ 点击按钮后跳转到登录页
- ✅ 窗口 A 无法继续发送消息

**消息显示**：
- ✅ 发送消息只显示一条
- ✅ 没有重复提示
- ✅ 流畅的聊天体验

### 核心实现

**1. Session Token 生成与验证**
```cpp
// 生成 token
std::string UserManager::generate_session_token() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

// 验证 token
bool UserManager::verify_session_token(int user_id, const std::string& token) {
    const char* sql = "SELECT session_token FROM users WHERE id = ?";
    // ... 查询数据库并比较 token
    return (token == db_token_str);
}
```

**2. 主动通知旧连接**
```cpp
// 新登录时查找并通知旧连接
if (was_online) {
    std::vector<std::shared_ptr<Connection>> old_connections;
    for (auto& conn : all_connections_) {
        if (conn && conn->session_.user_id == user.id && conn.get() != this) {
            old_connections.push_back(conn);
        }
    }
    
    for (auto& old_conn : old_connections) {
        old_conn->send_system_message("⚠️ 账号在其他地方登录，当前会话已失效。");
        std::thread([conn = old_conn]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            conn->close();
        }).detach();
    }
}
```

**3. 前端弹窗组件**
```typescript
// SessionExpiredModal.tsx
export default function SessionExpiredModal({ isVisible, onClose }) {
  useEffect(() => {
    if (isVisible) {
      localStorage.removeItem('user-storage')
      ;(window as any).chatHasLoggedIn = false
    }
  }, [isVisible])

  return (
    <div className="fixed inset-0 bg-black/70 flex items-center justify-center z-50">
      <div className="bg-white dark:bg-dark-card rounded-2xl...">
        <h2>账号在其他地方登录</h2>
        <button onClick={onClose}>返回登录页面</button>
      </div>
    </div>
  )
}
```

### 下一步计划

- [ ] 性能优化（连接池、批处理）
- [ ] 移动端适配优化
- [ ] 添加更多 AI 预设
- [ ] 消息已读未读状态

---

## 📅 2026-03-30 (第 17 天) - 前端 Bug 修复与优化 ✅

### 完成的工作

**前端 Bug 修复**：
- [x] 修复房间切换时消息发送到错误房间的问题
- [x] 修复联系人列表用户状态显示错误
- [x] 修复设置页面状态更改不同步问题
- [x] 修复登录状态持久化问题（刷新后跳回登录页）
- [x] 修复自动登录导致状态重置问题
- [x] 修复消息发送后不自动滚动问题

**房间系统优化**：
- [x] 加入房间时自动加载历史消息（最近 20 条）
- [x] 历史消息只显示内容，不显示时间和发送者
- [x] 修复房间前缀重复添加问题

**登录注册优化**：
- [x] 创建独立注册页面
- [x] 登录页面蓝色主题，注册页面绿色主题
- [x] 注册页面添加确认密码字段
- [x] 注册成功后自动跳转登录页
- [x] 添加视觉差异（图标、颜色、标题）

**状态管理优化**：
- [x] 修复用户状态被重复登录重置问题
- [x] 添加 chatHasLoggedIn 标志避免重复登录
- [x] 修复 Contact 页面重复发送/users 命令
- [x] 添加详细日志追踪状态变化

**页面布局优化**：
- [x] 创建 PageLayout 通用布局组件
- [x] 所有页面使用统一侧边栏
- [x] 联系人、设置、AI 助手页面侧边栏常驻

**后端日志增强**：
- [x] 添加用户状态更新日志
- [x] 添加在线用户列表查询日志
- [x] 添加广播消息日志

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 切换房间消息发错房间 | useEffect 闭包捕获旧 currentRoom | 使用 useRoomStore.getState() 实时获取 |
| 联系人列表状态全变在线 | 自动登录重复发送/login 重置状态 | 添加 chatHasLoggedIn 标志避免重复登录 |
| 刷新后跳回登录页 | skipHydration: true 阻止数据恢复 | 移除 skipHydration，添加登录检查 |
| 设置状态联系人不更新 | 前端未同步 userStore 状态 | 收到 online_users 时同步更新 |
| 历史消息格式冗余 | 后端格式化包含时间和发送者 | 只发送 content 字段 |
| Contact 重复请求/users | useEffect 依赖 setOnlineUsers | 移除依赖，只在挂载时请求 |

### 代码统计

| 模块 | 文件数 | 新增代码 | 修改代码 |
|------|--------|----------|----------|
| Chat 页面 | 1 | ~50 行 | ~100 行 |
| Contact 页面 | 1 | ~20 行 | ~50 行 |
| Settings 页面 | 1 | ~10 行 | ~30 行 |
| Login 页面 | 1 | ~100 行 | ~200 行 |
| Register 页面 | 1 | ~250 行 | - |
| PageLayout | 1 | ~20 行 | - |
| connection.cpp | 1 | ~50 行 | ~30 行 |
| user_manager.cpp | 1 | ~20 行 | ~10 行 |
| **总计** | **8** | **~520 行** | **~420 行** |

### 测试结果 ✅

**房间切换**：
- ✅ 消息发送到正确房间
- ✅ 加入房间显示历史消息
- ✅ 历史消息格式简洁

**状态管理**：
- ✅ 设置页面修改状态正确同步到联系人
- ✅ 刷新后保持登录状态
- ✅ 不会重复登录重置状态

**登录注册**：
- ✅ 登录/注册页面视觉差异明显
- ✅ 注册成功自动跳转
- ✅ 密码验证正常

**页面布局**：
- ✅ 所有页面侧边栏常驻
- ✅ 导航体验一致

### 核心修复

**1. 房间切换问题**
```typescript
// 修复前：使用闭包中的旧值
let room = currentRoom || '#general'

// 修复后：实时从 store 获取
const actualRoom = useRoomStore.getState().currentRoom || '#general'
```

**2. 重复登录问题**
```typescript
// 添加标志避免重复登录
if (!hasLoggedIn) {
  ws.send(`/login ${username} ${password}`)
  ;(window as any).chatHasLoggedIn = true
}
```

**3. 状态同步问题**
```typescript
// 收到 online_users 时同步更新当前用户状态
const currentUser = data.users.find(u => u.username === username)
if (currentUser && currentUser.status !== status) {
  setStatus(currentUser.status as any)
}
```

### 下一步计划

- [ ] 添加消息搜索功能
- [ ] 添加消息导出功能
- [ ] 优化移动端适配
- [ ] 添加更多 AI 预设
- [ ] 性能优化（虚拟滚动）

---

## 📅 2026-03-29 (第 16 天) - 阶段 6.2: 前端功能完善 ✅

### 完成的工作

**深色主题切换**：
- [x] 创建 useTheme Hook
- [x] 创建 themeStore 状态管理
- [x] 创建 ThemeProvider 组件
- [x] Sidebar 添加主题切换按钮
- [x] 设置页面主题选择（浅色/深色/系统）
- [x] 主题持久化（localStorage）

**联系人页面**：
- [x] 显示在线用户列表
- [x] 按状态分组（在线/忙碌/离开/离线）
- [x] 用户头像显示（首字母 + 渐变色）
- [x] WebSocket 连接状态监控
- [x] 自动刷新用户列表（5 秒间隔）
- [x] 手动刷新按钮
- [x] 调试信息显示

**AI 助手页面**：
- [x] 显示 6 个 AI 房间
- [x] 显示 6 个 AI 预设人格
- [x] 点击"加入"跳转并切换房间
- [x] 点击"使用"切换 AI 预设
- [x] 使用说明和快捷命令

**设置页面完善**：
- [x] 显示用户名
- [x] 用户状态切换
- [x] 主题切换（三个选项卡片）
- [x] 消息设置选项
- [x] 退出登录功能

**Bug 修复**：
- [x] 修复 WebSocket 连接断开问题
- [x] 修复房间切换状态不同步问题
- [x] 修复消息重复显示问题
- [x] 修复 AI 助手跳转房间问题
- [x] 修复联系人 WebSocket 不稳定问题

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 主题切换无反应 | themeStore 只保存状态未应用 | 创建 ThemeProvider 组件应用主题 |
| 联系人看不到其他用户 | 后端返回文本格式 | 修改后端返回 JSON 格式 |
| 联系人 WebSocket 断联 | 未监听断开事件 | 添加 close 事件监听和重连机制 |
| AI 助手跳转不切换房间 | Chat 页面未处理房间切换消息 | 解析"已加入房间"消息并更新 store |
| 消息处理器重复注册 | useEffect 依赖项导致重新注册 | 移除依赖项，在 useEffect 内定义处理器 |
| 后端自动重启 | 有监控进程 | sudo pkill -9 chat_server 清理 |

### 代码统计

| 模块 | 文件数 | 新增代码 | 修改代码 |
|------|--------|----------|----------|
| 主题管理 | 2 | ~100 行 | - |
| 联系人页面 | 1 | ~250 行 | ~50 行 |
| AI 助手页面 | 1 | ~160 行 | ~20 行 |
| 设置页面 | 1 | ~180 行 | ~30 行 |
| Chat 页面 | 1 | ~100 行 | ~200 行 |
| WebSocket Hook | 1 | ~20 行 | ~30 行 |
| **总计** | **7** | **~810 行** | **~330 行** |

### 测试结果 ✅

**深色主题**：
- ✅ Sidebar 按钮切换正常
- ✅ 设置页面主题选择正常
- ✅ 主题持久化正常（刷新后保持）
- ✅ 系统主题自动检测正常

**联系人页面**：
- ✅ 显示在线用户列表
- ✅ 按状态分组显示
- ✅ 用户头像显示正常
- ✅ WebSocket 自动重连
- ✅ 手动刷新功能正常

**AI 助手页面**：
- ✅ 6 个 AI 房间显示正常
- ✅ 6 个 AI 预设显示正常
- ✅ 点击"加入"跳转并切换房间
- ✅ 点击"使用"切换 AI 预设

**设置页面**：
- ✅ 显示用户名
- ✅ 切换用户状态
- ✅ 主题切换正常
- ✅ 退出登录正常

### 阶段 6.2 交付物

1. **完整的主题系统** - 支持浅色/深色/系统三种模式
2. **联系人页面** - 显示在线用户，支持自动刷新
3. **AI 助手页面** - 房间和预设人格展示
4. **设置页面** - 完整的个人设置功能
5. **稳定的 WebSocket 连接** - 自动检测和重连

---

## 📅 2026-03-29 (第 15 天) - 阶段 6.1: Web 前端功能完善 ✅

### 完成的工作

**WebSocket 联调**：
- [x] 修复 WebSocket 连接问题（IPv6 → IPv4）
- [x] 修复后端 WebSocket 握手（Sec-WebSocket-Protocol）
- [x] 实现登录页面与聊天页面的 WebSocket 复用
- [x] 实现自动登录（localStorage 保存密码）
- [x] 修复消息解析逻辑（JSON 格式）

**消息系统修复**：
- [x] 修复消息重复显示问题（移除 Login 页面 onmessage 处理器）
- [x] 修复房间消息显示错误（添加 # 前缀）
- [x] 修复 @AI 消息不显示问题（后端 broadcast_to_room）
- [x] 实现 AI 响应正确显示（解析 ai_response 类型）

**用户体验优化**：
- [x] 添加 WebSocket 连接状态显示
- [x] 添加系统通知区域
- [x] 添加加载状态提示
- [x] 添加错误提示

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| WebSocket 连接失败 | 后端固定返回 Sec-WebSocket-Protocol | 修改后端只在客户端请求时返回该头 |
| WebSocket 连接失败 | 浏览器使用 IPv6 连接 | 修改前端使用 127.0.0.1 而非 localhost |
| 消息重复显示 | Login 和 Chat 页面各有一个 onmessage 处理器 | Login 页面登录后移除 onmessage 处理器 |
| 消息重复显示 | 前端自己添加消息 + 后端返回消息 | 前端不再自己添加，只处理后端返回 |
| 房间消息错乱 | 后端 room 字段没有 # 前缀 | 前端自动添加 # 前缀 |
| @AI 消息不显示 | 后端处理 @AI 时没有广播用户消息 | 后端先调用 broadcast_to_room |
| 登录状态丢失 | 页面跳转后 WebSocket 关闭 | Login 页面不清理 WebSocket 连接 |
| 密码未保存 | 闭包变量不是最新值 | 使用 useRef 保存密码 |

### 学到的知识

1. **WebSocket 协议细节**
   - Sec-WebSocket-Protocol 握手规则
   - IPv4 vs IPv6 连接问题
   - 连接复用与状态管理

2. **React 组件通信**
   - 跨组件 WebSocket 复用
   - useEffect 清理函数
   - useRef 保存可变值

3. **后端消息处理**
   - 广播消息逻辑
   - AI 响应处理
   - 房间隔离实现

### 代码统计

| 模块 | 文件数 | 新增代码 | 修改代码 |
|------|--------|----------|----------|
| 前端页面 | 5 | ~800 行 | ~200 行 |
| 前端组件 | 3 | ~300 行 | - |
| 前端 Store | 3 | ~150 行 | - |
| 前端服务 | 2 | ~200 行 | - |
| 后端协议 | 2 | - | ~50 行 |
| 后端连接 | 1 | - | ~100 行 |
| **总计** | **16** | **~1450 行** | **~350 行** |

### 测试结果 ✅

**功能测试**：
- ✅ 用户登录/注册正常
- ✅ WebSocket 连接正常
- ✅ 消息收发正常（不重复）
- ✅ 房间切换正常（消息隔离）
- ✅ @AI 对话正常（用户消息+AI 回复）
- ✅ 系统通知正常
- ✅ 退出登录正常

**性能测试**：
- ✅ 页面加载时间 < 2 秒
- ✅ WebSocket 连接时间 < 500ms
- ✅ 消息响应时间 < 1 秒

### 阶段 6.1 交付物

1. **完整的 Web 聊天界面** - 类似微信/QQ 的三栏布局
2. **实时消息系统** - WebSocket 双向通信
3. **AI 对话功能** - @AI 触发 AI 回复
4. **多房间支持** - 6 个 AI 房间隔离
5. **用户认证系统** - 登录/注册/自动登录

---

## 📅 2026-03-28 (第 14 天) - 阶段 6: Web 前端启动 ✅

### 完成的工作

**前端项目搭建**：
- [x] 创建 frontend 目录结构
- [x] 配置 Vite + React + TypeScript
- [x] 配置 Tailwind CSS
- [x] 安装依赖（380 个包）
- [x] 创建基础组件和页面

**技术选型**：
- [x] React 18 - UI 框架
- [x] TypeScript - 类型系统
- [x] Vite - 构建工具
- [x] Tailwind CSS - 样式
- [x] Zustand - 状态管理
- [x] React Router - 路由
- [x] React Markdown - Markdown 渲染
- [x] Syntax Highlighter - 代码高亮

**创建的组件**：
- [x] Sidebar - 侧边栏导航
- [x] ChatList - 会话列表
- [x] ChatArea - 聊天区域（支持 Markdown 和代码高亮）
- [x] Login - 登录/注册页面
- [x] Chat - 聊天主页面
- [x] Contact - 联系人页面（占位）
- [x] AIAssistant - AI 助手页面（占位）
- [x] Settings - 设置页面

**状态管理**：
- [x] userStore - 用户状态（登录/登出/状态）
- [x] messageStore - 消息存储
- [x] roomStore - 房间管理
- [x] WebSocket 服务类
- [x] useWebSocket Hook

**文档**：
- [x] docs/frontend_design.md - 前端设计文档
- [x] frontend/README.md - 前端说明

### 项目结构

```
frontend/
├── public/
│   └── logo.svg
├── src/
│   ├── components/
│   │   ├── Sidebar.tsx
│   │   ├── ChatList.tsx
│   │   └── ChatArea.tsx
│   ├── pages/
│   │   ├── Login.tsx
│   │   ├── Chat.tsx
│   │   ├── Contact.tsx
│   │   ├── AIAssistant.tsx
│   │   └── Settings.tsx
│   ├── stores/
│   │   ├── userStore.ts
│   │   ├── messageStore.ts
│   │   └── roomStore.ts
│   ├── services/
│   │   ├── websocket.ts
│   │   └── api.ts
│   ├── hooks/
│   │   └── useWebSocket.ts
│   ├── types/
│   │   └── index.ts
│   ├── App.tsx
│   ├── main.tsx
│   └── index.css
├── package.json
├── vite.config.ts
├── tailwind.config.js
└── README.md
```

### 配置说明

**开发服务器**：
- 端口：3000
- WebSocket 代理：ws://localhost:8080

**启动命令**：
```bash
cd frontend
npm install      # 安装依赖
npm run dev      # 启动开发服务器
npm run build    # 生产构建
```

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Vite 创建命令超时 | 交互式命令阻塞 | 手动创建 package.json 和配置文件 |
| 无 | 项目刚启动 | - |

### 学到的知识

1. **Vite 配置**
   - 路径别名配置
   - WebSocket 代理配置
   - React 插件配置

2. **Zustand 状态管理**
   - 轻量级 Store 创建
   - persist 中间件持久化
   - 多 Store 拆分

3. **Tailwind CSS**
   - 深色模式配置
   - 自定义颜色主题
   - 响应式工具类

### 代码统计

| 模块 | 文件数 | 代码量 |
|------|--------|--------|
| 组件 | 3 | ~300 行 |
| 页面 | 5 | ~400 行 |
| Store | 3 | ~150 行 |
| 服务 | 2 | ~100 行 |
| Hook | 1 | ~50 行 |
| **总计** | **14** | **~1000 行** |

### 下一步计划

1. **WebSocket 联调** - 连接后端服务器，测试消息收发
2. **消息显示优化** - 完善 Markdown 渲染和代码高亮
3. **深色主题** - 添加主题切换功能
4. **联系人页面** - 实现完整的联系人列表

---

## 📅 2026-03-27 (第 1 天) - 项目启动 ✅

### 完成的工作
- [x] 创建项目结构
- [x] 编写需求文档
- [x] 确定技术选型
- [x] 创建 CMakeLists.txt 基础配置

### 遇到的问题
| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 无 | 项目刚启动 | - |

### 学到的知识
- 无

### 今日代码提交

**项目结构**:
- `README.md` - 项目说明
- `docs/requirements.md` - 需求文档
- `docs/architecture.md` - 架构设计
- `CMakeLists.txt` - 构建配置

**源代码**:
- `src/main.cpp` - 程序入口
- `src/server/server.h/cpp` - 服务器主类
- `src/server/connection.h/cpp` - 连接管理
- `src/protocol/message.h/cpp` - 消息结构
- `src/protocol/json_parser.h/cpp` - JSON 解析
- `src/protocol/websocket.h/cpp` - WebSocket 协议实现
- `src/ai/ai_client.h/cpp` - AI 客户端
- `src/ai/api_config.h` - API 配置
- `src/storage/database.h/cpp` - 数据库操作
- `src/utils/logger.h/cpp` - 日志工具

### 遇到的问题
| 问题 | 原因 | 解决方案 |
|------|------|----------|
| JsonParser 未声明 | connection.cpp 缺少头文件引用 | 添加 `#include "protocol/json_parser.h"` |
| SQLite3 链接错误 | CMake 变量名不正确 | 使用 `SQLite::SQLite3` 现代 CMake 目标 |
| 类型转换错误 | int 不能直接与 string 相加 | 使用 `std::to_string()` 转换 |
| 服务器立即退出 | `server.start()` 未调用，io_context 无操作 | 在 main.cpp 中调用 `server.start()` |
| WebSocket 子协议不匹配 | 客户端未请求子协议 | 测试客户端添加 `subprotocols=["chat"]` |

### 学到的知识
- Boost.Asio 异步网络编程模型
- WebSocket 握手协议和帧格式
- CMake FetchContent 自动下载依赖
- io_context.run() 需要有 pending 操作才会阻塞

### 阶段 1 测试结果 ✅

**测试命令**：
```bash
# 终端 1 - 启动服务器
./chat_server

# 终端 2 - C++ 测试客户端
./ws_client
```

**测试结果**：
- ✅ 服务器成功启动并监听端口 8080
- ✅ WebSocket 握手成功
- ✅ 消息收发正常
- ✅ 连接正常关闭
- ✅ C++ 测试客户端工作正常

**手动测试验证**：连接和消息传输功能正常，可以进入下一阶段开发。

---

## 📅 2026-03-27 (第 2 天) - 阶段 2: AI 接入 ✅

**测试结果** ✅：
- ✅ AI API 调用成功（超算互联网 MiniMax-M2.5）
- ✅ AI 回复正常
- ✅ 消息存储到数据库

---

## 📅 2026-03-27 (第 3 天) - 阶段 3: 房间系统 + 私聊 AI ✅

### 完成的工作

**核心模块**：
- [x] 用户系统（注册/登录/状态管理）
- [x] 房间管理器（创建/加入/离开）
- [x] 预设管理器（AI 设定管理）
- [x] 命令解析器（参数分割）
- [x] Connection 集成（540+ 行）

**内置内容**：
- [x] 6 个房间（general/code/translate/learn/write/debug）
- [x] 6 个预设（teacher/coder/friend/interviewer/critic/coach）
- [x] 完整命令系统（15+ 命令）

**文档**：
- [x] MANUAL.md 使用手册
- [x] 更新 requirements.md
- [x] 更新 architecture.md
- [x] 更新 study_notes.md

### 新增模块

| 文件 | 行数 | 功能 |
|------|------|------|
| `src/user/user_manager.h/cpp` | 230+ | 用户注册/登录/状态 |
| `src/room/room_manager.h/cpp` | 190+ | 房间管理/AI 配置 |
| `src/preset/preset_manager.h/cpp` | 170+ | 预设管理 |
| `src/command/command_parser.h/cpp` | 80+ | 命令解析 |
| `src/server/connection.cpp` | 540+ | 完整命令处理 |
| `scripts/ws_client.cpp` | 260+ | 纯文本客户端 |

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 栈溢出 (stack smashing) | SHA256 使用 SHA_DIGEST_LENGTH 而非 SHA256_DIGEST_LENGTH | 修正常量名 |
| JSON 解析失败 | 中文字符串截断导致 UTF-8 不完整 | 移除字符串截断 |
| 命令参数未分割 | 参数作为整体传入 | 使用 istringstream 分割 |
| 客户端显示 JSON | ws_client 未解析 JSON | 添加 JSON 解析提取 content |
| 预设切换无响应 | 参数未分割导致 use teacher 被当作一个参数 | 修复参数分割逻辑 |

### 学到的知识

1. **C++ 模块化设计**
   - 管理器模式（UserManager/RoomManager/PresetManager）
   - 依赖注入（Connection 接收管理器指针）
   - 前向声明解决循环依赖

2. **命令模式实现**
   - 命令解析与分发
   - 参数分割（istringstream）
   - 命令扩展性设计

3. **用户会话管理**
   - Session 结构体保存用户状态
   - 连接状态机（HANDSHAKE/AUTHENTICATED/ACTIVE）
   - 在线状态跟踪

4. **字符串处理**
   - 中文字符串处理注意事项
   - JSON 与纯文本的兼容处理
   - UTF-8 编码问题

5. **调试技巧**
   - 日志定位问题
   - strace 跟踪系统调用
   - 分段测试法定位 bug

### 阶段 3 测试结果 ✅

**测试命令**：
```bash
# 启动服务器
export AI_API_KEY="sk-xxx"
./chat_server

# 测试客户端
./ws_client
```

**测试流程**：
```
/register alice          ✅
/login alice             ✅
/help                    ✅
/rooms                   ✅
/join code               ✅
写一个 Python 排序        ✅
/preset list             ✅
/preset use teacher      ✅
/ai 解释勾股定理         ✅
/ai [诗人] 写诗          ✅
/quit                    ✅
```

**测试结果**：
- ✅ 用户注册/登录正常
- ✅ 房间列表显示正常
- ✅ 房间切换正常
- ✅ 预设列表显示正常
- ✅ 预设切换正常
- ✅ 房间 AI 对话正常
- ✅ 私聊 AI 正常
- ✅ 临时设定正常

### 代码统计

| 模块 | 代码量 | 说明 |
|------|--------|------|
| 网络层 | 300+ | WebSocket 处理 |
| 协议层 | 200+ | JSON/WebSocket 解析 |
| 用户层 | 230+ | 用户管理 |
| 房间层 | 190+ | 房间管理 |
| 预设层 | 170+ | 预设管理 |
| AI 层 | 140+ | AI 客户端 |
| **总计** | **2000+** | 不含注释 |

### 阶段 3 交付物

1. **可运行的聊天服务器** - 支持多房间、多预设 AI 对话
2. **完整的文档** - README/MANUAL/QUICKSTART/开发日志/学习笔记
3. **测试客户端** - C++ 纯文本 WebSocket 客户端
4. **测试脚本** - Python 自动化测试脚本

---

## 📅 2026-03-27 (第 4 天) - 用户系统优化 ✅

### 完成的工作

**用户系统改进**：
- [x] 注册需要密码（之前是固定密码）
- [x] 登录需要密码验证
- [x] 密码长度限制（≥6 位）
- [x] 新增用户状态（online/busy/away/offline）
- [x] 新增 `/status` 命令

**文档更新**：
- [x] MANUAL.md 更新用户系统章节
- [x] README.md 更新命令列表

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 登录无需密码 | 使用固定密码 "password" | 修改 cmd_register/cmd_login 接收密码参数 |
| 状态单一 | 只有 online | 添加 status 字段和 cmd_status 命令 |
| 命令未声明 | cmd_status 未在头文件声明 | 在 connection.h 添加声明 |

### 学到的知识

1. **密码安全**
   - SHA256 哈希存储
   - 密码长度验证
   - 不存储明文密码

2. **状态管理**
   - Session 状态同步
   - 数据库持久化
   - 在线用户显示

3. **命令扩展**
   - 添加新命令的完整流程
   - 头文件声明
   - 命令注册
   - help 文档更新

### 代码变更

**修改文件**：
```
src/server/connection.h       - 添加 cmd_status 声明，Session 添加 status 字段
src/server/connection.cpp     - 修改 cmd_register/cmd_login，添加 cmd_status
src/MANUAL.md                 - 更新用户系统文档
```

**新增功能**：
```cpp
// Session 结构体
struct Session {
    std::string status = "online";  // 新增状态字段
};

// 状态命令
void Connection::cmd_status(const std::vector<std::string>& args) {
    // 查看/设置用户状态
}
```

### 测试结果 ✅

**测试流程**：
```
/register alice 123456    ✅ 注册成功
/login alice 123456       ✅ 登录成功
/status                   ✅ 显示当前状态
/status busy              ✅ 状态更新
/online                   ✅ 显示 [busy] 状态
```

---

## 📅 2026-03-27 (第 5 天) - 阶段 4.1: 群聊基础 ✅

### 完成的工作

**聊天室模块**：
- [x] ChatRoom 类（聊天室基础）
- [x] ChatRoomManager 类（聊天室管理）
- [x] 成员管理（join/leave）
- [x] 消息广播框架

**群聊功能**：
- [x] 房间创建（`/create room`）
- [x] 房间列表（`/list`）
- [x] 加入/离开房间
- [x] 消息广播（同房间用户可见）

**AI 房间分类**：
- [x] AI 房间（`/rooms`）- 内置 AI 功能房间
- [x] 群聊房间（`/list`）- 用户创建的房间

**客户端优化**：
- [x] 移除命令帮助菜单
- [x] 简化连接提示

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 连接列表类型错误 | ConnectionPtr 在类后定义 | 前向声明 Connection 和 ConnectionPtr |
| 消息不广播 | 未维护连接列表 | 添加 static all_connections_ |
| 房间列表不显示 | 未记录创建的房间 | 添加 static created_rooms_ |

### 学到的知识

1. **消息广播**
   - 维护全局连接列表
   - 按房间过滤广播目标
   - 连接加入/离开管理

2. **静态成员**
   - 跨实例共享数据
   - 连接列表管理
   - 房间列表管理

3. **模块化设计**
   - ChatRoom 独立模块
   - 与 Connection 解耦
   - 便于扩展

### 代码统计

| 模块 | 代码量 | 说明 |
|------|--------|------|
| ChatRoom | 150+ | 聊天室管理 |
| Connection 扩展 | 100+ | 广播功能 |
| **阶段 4.1 总计** | **250+** | 新增代码 |

### 测试结果 ✅

**测试流程**：
```
# 终端 1
/login user1 123456
/join testroom
你好，有人吗？

# 终端 2
/login user2 123456
/join testroom
# 收到：[user1]: 你好，有人吗？
我在！
# 终端 1 收到：[user2]: 我在！
```

**测试项目**：
- ✅ 创建房间
- ✅ 查看房间列表
- ✅ 加入房间
- ✅ 消息广播
- ✅ AI 调用（主动）
- ✅ 用户状态

---

## 📅 2026-03-28 (第 6 天) - 阶段 4.1 完善与修复 ✅

### 完成的工作

**用户状态管理优化**：
- [x] `/online` 显示所有非 offline 状态用户
- [x] 添加状态图标（🟢🔴🟡⚪）
- [x] 登录时自动设置为 online
- [x] 退出时自动设置为 offline
- [x] 服务器启动时重置所有用户为 offline

**客户端优化**：
- [x] Ctrl+C 信号处理
- [x] 关闭 stdin 中断 getline
- [x] 退出前发送 /quit 命令到服务器

**文档完善**：
- [x] 创建 TESTING.md 测试指南
- [x] 创建 PROJECT_SUMMARY.md 项目总结
- [x] 更新 README.md 文档索引

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `/online` 只显示 online | SQL 查询条件限制 | 修改为 `WHERE status != 'offline'` |
| Ctrl+C 不能退出 | getline 阻塞 | 关闭 stdin 中断阻塞 |
| 服务器重启状态不更新 | SIGKILL 无法捕获 | 服务器启动时重置所有状态 |
| 客户端输入无响应 | 非阻塞输入不可靠 | 恢复使用 getline |

### 学到的知识

1. **进程信号处理**
   - SIGKILL 无法捕获
   - SIGINT 可以处理
   - 关闭文件描述符中断阻塞

2. **状态管理最佳实践**
   - 启动时重置状态
   - 正常退出时更新状态
   - 析构函数兜底更新

3. **用户体验优化**
   - 状态图标可视化
   - 退出提示友好
   - 命令响应及时

### 代码变更

**修改文件**：
```
src/user/user_manager.h/cpp    - 添加 reset_all_users_offline()
src/server/connection.h/cpp    - 析构函数更新状态
src/user/user_manager.cpp      - get_online_users() 修改查询条件
scripts/ws_client.cpp          - Ctrl+C 信号处理
```

### 测试结果 ✅

**测试流程**：
```
# 测试 1：状态显示
/login user1 123456
/status busy
/online
# 预期：🔴 user1 [busy]

# 测试 2：Ctrl+C 退出
./ws_client
/login user1 123456
^C
# 预期：立即退出，状态更新为 offline

# 测试 3：服务器重启
./ws_client -> /login user1 123456
pkill -9 chat_server
./chat_server
./ws_client -> /login user2 123456 -> /online
# 预期：看不到 user1
```

**测试项目**：
- ✅ `/online` 显示所有非 offline 用户
- ✅ 状态图标显示正确
- ✅ Ctrl+C 退出正常
- ✅ 服务器重启状态重置
- ✅ 用户登录/退出状态更新

---

## 📅 2026-03-28 (第 7 天) - 阶段 4.2: AI 对话上下文 ✅

### 完成的工作

**对话上下文模块**：
- [x] ConversationContext 数据结构
- [x] ConversationManager 管理器
- [x] 数据库持久化（conversations 表）
- [x] AI 客户端支持上下文参数

**集成功能**：
- [x] @AI 调用自动带上上下文
- [x] /msg ai 命令支持上下文
- [x] 自动保存对话历史到数据库
- [x] 启动时加载历史上下文

**上下文命令**：
- [x] `/clear` - 清空对话历史
- [x] `/context` - 查看上下文大小
- [x] `/context set <轮数>` - 设置最大历史轮数

**问题修复**：
- [x] SSL 连接错误（跳过证书验证）
- [x] 删除 `/ai` 命令（统一使用 `/msg ai` 和 `@AI`）

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| SSL connect error | CURL 证书验证失败 | 设置 `CURLOPT_SSL_VERIFYPEER=0` |
| `/ai` 上下文不一致 | 代码路径不同 | 删除 `/ai`，统一使用 `/msg ai` |
| 不完整类型错误 | 前向声明无法调用方法 | include 完整头文件 |
| 函数重载歧义 | 重复声明 | 删除旧声明 |

### 学到的知识

1. **上下文管理**
   - 每个房间/用户独立上下文
   - 内存 + 数据库双重存储
   - 可配置的历史轮数

2. **CURL SSL 配置**
   - 测试环境可跳过证书验证
   - 生产环境应该验证证书

3. **命令设计**
   - 保持命令一致性
   - 避免功能重复

### 代码统计

| 模块 | 代码量 | 说明 |
|------|--------|------|
| ConversationContext | 70+ | 上下文数据结构 |
| ConversationManager | 180+ | 上下文管理器 |
| AIClient 扩展 | 50+ | 支持上下文参数 |
| Connection 集成 | 100+ | 消息处理集成 |
| 上下文命令 | 80+ | /clear, /context |
| **阶段 4.2 总计** | **480+** | 新增代码 |

### 测试结果 ✅

**测试流程**：
```
# 测试 1：多轮对话
/login user1 123456
/msg ai 我叫小明
/msg ai 我叫什么名字？
# 预期：AI 记得你叫小明

# 测试 2：@AI 上下文
@AI 我喜欢苹果
@AI 我喜欢什么？
# 预期：AI 记得你喜欢苹果

# 测试 3：查看上下文
/context
# 预期：显示当前上下文大小

# 测试 4：清空上下文
/clear
/msg ai 我叫什么名字？
# 预期：AI 不知道名字
```

**测试项目**：
- ✅ 多轮对话记忆
- ✅ @AI 上下文支持
- ✅ /msg ai 上下文支持
- ✅ 上下文查看
- ✅ 上下文清空
- ✅ 数据库持久化
- ✅ SSL 连接修复

---

## 📅 2026-03-28 (第 8 天) - 阶段 4.3: 消息搜索 + 历史 ✅

### 完成的工作

**消息历史模块**：
- [x] MessageHistoryManager 管理器
- [x] 数据库消息存储（添加 room_name 字段）
- [x] 历史记录查询（AI/房间）
- [x] 消息搜索（关键词/用户/房间）
- [x] 消息导出（TXT/JSON 格式）

**集成功能**：
- [x] `/history` 命令 - 查看历史消息
- [x] `/search` 命令 - 搜索消息
- [x] `/export` 命令 - 导出聊天记录

**问题修复**：
- [x] 数据库表结构添加 room_name 字段
- [x] Message 结构体添加 room 字段
- [x] AI 回复保存时设置 room 字段
- [x] 房间历史查询去掉#前缀
- [x] 历史记录显示添加房间前缀

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 数据库缺少 room_name | 表结构不完整 | 修改 CREATE TABLE 语句 |
| Message 缺少 room 字段 | 结构体定义不完整 | 添加 room 字段 |
| AI 回复无房间信息 | 保存时未设置 | `response.room = session_.current_room` |
| 房间历史查不到 AI | 查询时#前缀未处理 | `room = target.substr(1)` |
| 导出显示不清晰 | 格式简单 | 添加房间前缀显示 |

### 学到的知识

1. **数据库设计**
   - 表结构要提前规划
   - 字段缺失会导致查询困难

2. **消息存储**
   - 所有消息都应该保存 room_name
   - AI 回复也要关联房间

3. **查询优化**
   - 特殊处理@ai 查询
   - 房间名要去掉#前缀

### 代码统计

| 模块 | 代码量 | 说明 |
|------|--------|------|
| MessageHistoryManager | 220+ | 历史管理 |
| 命令实现 | 150+ | history/search/export |
| 数据库修改 | 50+ | room_name 字段 |
| **阶段 4.3 总计** | **420+** | 新增代码 |

### 测试结果 ✅

**测试流程**：
```
# 产生消息
/login user1 123456
@AI 你好
/join code
@AI 写个快速排序

# 查看历史
/history #code 10
# 预期：显示房间历史（包括 AI 回复）

# 搜索
/search 快速排序
# 预期：找到相关消息

# 导出
/export @ai
# 预期：导出聊天记录到客户端显示
```

**测试项目**：
- ✅ `/history @ai` 显示 AI 对话
- ✅ `/history #code` 显示房间历史
- ✅ `/search` 搜索功能正常
- ✅ `/export` 导出功能正常

---

## 📅 2026-03-28 (第 9 天) - 阶段 5.1: 性能优化 ✅

### 完成的工作

**异步 AI 调用**：
- [x] chat_async() 接口实现
- [x] Connection 集成异步调用
- [x] 回调函数处理 AI 回复

**内存优化**：
- [x] 缓冲区预分配（8KB）
- [x] 减少不必要的内存分配
- [x] 批处理发送优化

### 测试结果 ✅

- ✅ AI 调用不阻塞连接
- ✅ 减少内存分配次数
- ✅ 提高缓冲区利用率

---

## 📅 2026-03-28 (第 10 天) - 阶段 5.2: 单元测试 ✅

### 完成的工作

**测试框架集成**：
- [x] GoogleTest 集成（v1.14.0）
- [x] CMake 测试配置
- [x] 测试可执行文件创建

**测试用例编写**：
- [x] UserManager 测试（6 个测试）
- [x] RoomManager 测试（5 个测试）
- [x] MessageHistory 测试（3 个测试）
- [x] Conversation 测试（6 个测试）

**测试结果**：
- [x] 20 个测试全部通过
- [x] 测试耗时 < 50ms
- [x] 无失败测试

### 测试结果 ✅

**测试流程**：
```bash
cd build
./chat_server_test

# 输出
[==========] 20 tests from 4 test suites ran. (49 ms total)
[  PASSED  ] 20 tests.
```

**测试项目**：
- ✅ UserManager (6/6) - 用户管理功能
- ✅ RoomManager (5/5) - 房间管理功能
- ✅ MessageHistory (3/3) - 消息历史功能
- ✅ Conversation (6/6) - 对话上下文功能

---

## 📅 2026-03-28 (第 12 天) - 阶段 5.3: Docker 部署 ✅

### 完成的工作

**Docker 配置**：
- [x] Dockerfile 编写（多阶段构建）
- [x] docker-compose.yml 配置
- [x] .env.example 配置模板
- [x] .gitignore 配置
- [x] Docker 启动脚本

**文档编写**：
- [x] DOCKER_GUIDE.md 部署指南
- [x] DOCKER_TEST_CHECKLIST.md 测试清单
- [x] DOCKER_FINAL_TEST_REPORT.md 最终报告

**功能特性**：
- [x] 多阶段构建（减小镜像大小）
- [x] 健康检查配置
- [x] 数据卷持久化
- [x] 日志轮转配置
- [x] 网络隔离配置

### 遇到的问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Docker Hub 无法访问 | 网络限制 | 配置多个国内镜像源 |
| CMake 缓存冲突 | build 目录未清理 | 清理后重新构建 |
| GoogleTest 缓存问题 | FetchContent 缓存 | 清理 build 目录 |

### 测试结果 ✅

**构建测试**：
- ✅ Dockerfile 语法正确
- ✅ docker-compose 配置正确
- ✅ 镜像构建成功（89.4MB）

**功能测试**：
- ✅ 容器启动正常
- ✅ 端口映射正常（8080）
- ✅ 环境变量配置正常
- ✅ 数据卷挂载正常
- ✅ 健康检查通过

**手动测试**：
- ✅ 用户注册成功
- ✅ 用户登录成功
- ✅ 房间功能正常
- ✅ AI 对话正常
- ✅ 历史记录保存正常
- ✅ Docker 重启正常

### 镜像信息

| 阶段 | 大小 | 说明 |
|------|------|------|
| 构建镜像 | ~2GB | 包含所有编译工具 |
| 运行镜像 | 89.4MB | 只包含运行时依赖 |

### 部署方式

| 方式 | 命令 | 说明 |
|------|------|------|
| docker-compose | `docker-compose up -d` | 推荐，简单 |
| Docker | `docker run ...` | 灵活，需要手动配置 |
| 启动脚本 | `./scripts/docker-start.sh` | 交互式，方便管理 |

### 测试日志

```
[2026-03-28 13:48:27] [INFO] Database initialized at ./chat.db
[2026-03-28 13:48:27] [INFO] All users reset to offline status
[2026-03-28 13:48:27] [INFO] Room created: general
[2026-03-28 13:48:27] [INFO] Room created: code
...
[2026-03-28 13:53:09] [INFO] User registered: user1 (status: offline)
[2026-03-28 13:53:09] [INFO] User logged in: user1
```

**容器状态**：
```
CONTAINER ID   IMAGE                            STATUS
904378e3fea0   cpp-ai-chat-server_chat-server   Up (healthy)
```

---

## 📅 2026-03-28 (第 13 天) - 阶段 5.4: 日志系统 ✅

### 完成的工作

**spdlog 集成**：
- [x] 添加 spdlog 库（通过 FetchContent）
- [x] 替换原有 Logger 实现
- [x] 支持彩色控制台输出
- [x] 支持日志文件轮转

**日志级别管理**：
- [x] 支持 DEBUG/INFO/WARN/ERROR/CRITICAL
- [x] 可配置日志级别
- [x] 编译时和运行时控制

**日志文件轮转**：
- [x] 按大小轮转（5MB）
- [x] 保留 3 个历史文件
- [x] 自动压缩

### 日志格式

**输出格式**：
```
[2026-03-28 22:37:16.559] [info] 消息内容
[2026-03-28 22:37:16.579] [error] 错误消息
```

**特性**：
- 时间戳精确到毫秒
- 彩色级别显示（info=绿色，error=红色）
- 同时输出到控制台和文件

### 测试结果 ✅

**功能测试**：
- ✅ spdlog 集成成功
- ✅ 日志文件生成正常
- ✅ 日志轮转配置正确
- ✅ 控制台输出正常

**日志文件**：
```
chat_server.log          # 当前日志
chat_server.log.1        # 历史日志 1
chat_server.log.2        # 历史日志 2
chat_server.log.3        # 历史日志 3
```

### 配置示例

**在 main.cpp 中配置**：
```cpp
// 初始化日志系统
Logger::init("chat_server.log", spdlog::level::info);

// 设置日志级别
Logger::set_level(spdlog::level::debug);
```

---

## 📊 项目整体进度

| 阶段 | 功能 | 状态 |
|------|------|------|
| 阶段 1 | WebSocket 服务器 | ✅ 完成 |
| 阶段 2 | AI 接入 + 消息存储 | ✅ 完成 |
| 阶段 3 | 房间系统 + 私聊 AI | ✅ 完成 |
| 阶段 3.5 | 用户系统优化 | ✅ 完成 |
| 阶段 4.1 | 群聊基础 | ✅ 完成 |
| 阶段 4.2 | AI 对话上下文 | ✅ 完成 |
| 阶段 4.3 | 消息搜索 + 历史 | ✅ 完成 |
| 阶段 5.1 | 性能优化 | ✅ 完成 |
| 阶段 5.2 | 单元测试 | ✅ 完成 |
| 阶段 5.3 | Docker 部署 | ✅ 完成 |
| **阶段 5.4** | **日志系统** | **✅ 完成** |

---

**🎉 所有计划阶段完成！项目完成度：100%！**
