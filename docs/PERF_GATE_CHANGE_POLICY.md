# 性能门禁阈值治理策略

本文定义性能门禁阈值（当前为 `PERF_WARN_RATIO`）的变更流程，确保每次调整可追溯、可审计、可回放。

## 1. 适用范围

- 门禁脚本：`scripts/run_backend_checks.sh`
- 汇总脚本：`scripts/summarize_perf_reports.py`
- CI 工作流：`.github/workflows/backend-checks.yml`
- 趋势输出：`docs/perf_reports/message_history_benchmark_summary.md`

## 2. 当前默认策略

- 默认阈值：`PERF_WARN_RATIO=0.80`
- 主分支与目标为主分支的 PR：`PERF_WARN_FAIL=1`
- 其他分支：`PERF_WARN_FAIL=0`（仅告警，不阻断）

## 3. 变更触发条件

满足任一条件可提出阈值调整：

1. 连续多次出现误报（业务未退化但门禁频繁触发）。
2. 基线能力已显著提升，需要更严格阈值约束。
3. 基础设施/数据规模变化导致历史可比性变化。

## 4. 角色与权限

- 提案人：任何后端开发者
- 评审人：至少 1 名后端负责人 + 1 名发布负责人
- 执行人：PR 合并人（不得绕过评审）

## 5. 标准变更流程

1. 收集最近至少 7 次 benchmark 报告趋势。
2. 在 PR 中说明“为什么要改阈值、改前后风险”。
3. 更新阈值配置（工作流环境变量或执行脚本参数）。
4. 同步更新文档：
   - `README.md`
   - `docs/PERFORMANCE_REPORT_TEMPLATE.md`
5. 新增一条阈值调整记录（见模板）。
6. 合并后在下一次发布演练中验证门禁行为。

## 6. 记录要求

每次阈值变化都必须新增记录文件：

- 建议路径：`docs/perf_reports/perf_gate_changes/`
- 建议命名：`YYYYMMDD-HHMM-<short-topic>.md`

可直接复制模板：

- `docs/perf_reports/PERF_GATE_CHANGE_RECORD_TEMPLATE.md`

## 7. 回滚策略

若阈值调整后 3 次以内出现明显误判或漏报：

1. 立即回滚到上一个稳定阈值。
2. 在记录文件补充“回滚原因与影响”。
3. 在下一次例会复盘并修订策略。

## 8. 最小审计清单

- [ ] 记录文件已新增并填写完整
- [ ] README 与报告模板已同步
- [ ] PR 中包含趋势数据与风险说明
- [ ] 合并后至少一次 CI 行为验证
- [ ] backend-checks job summary 已输出 PERF_WARN_RATIO / PERF_WARN_FAIL / gate_mode / gate_source
- [ ] gate_source 值域已统一为 env / cli / workflow，并与 CI 输出一致
