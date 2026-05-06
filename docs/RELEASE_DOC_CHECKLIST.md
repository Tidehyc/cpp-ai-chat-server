# 发布文档核对清单

本文用于每次发布前核对 README、Release Notes、devlog 和相关治理文档是否一致。

## 1. 版本同步规则

发布前请确认以下信息一致或有明确说明：

- README 顶部版本号
- `RELEASE_NOTES.md` 的当前版本标题与发布日期
- `docs/devlog.md` 的最新发布日期与发布说明
- `docs/PRODUCT_LAUNCH_PLAYBOOK.md` 中当前阶段状态
- `docs/ROADMAP.md` 的优先级与里程碑是否与 Playbook 保持一致
- `docs/RELEASE_ROLLBACK_RUNBOOK.md` 的执行流程是否与 Playbook 第 11 节一致
- `docs/BACKEND_TODO_CHECKLIST.md` 的完成状态
- `docs/perf_reports/message_history_benchmark_summary.md` 是否包含 PERF_WARN_RATIO / PERF_WARN_FAIL / gate_mode / gate_source
- `docs/perf_reports/PERFORMANCE_BASELINE_20260425.md` 与并发压测文档是否已同步最新结论

规则要求：

1. README、Release Notes、devlog 的版本/日期描述不得互相矛盾。
2. 若 README 版本号已更新，Release Notes 必须同步新增对应版本段落。
3. devlog 中的发布记录应覆盖本轮重要变更与验证结果。
4. 若是治理类变更，也要同步更新对应文档索引与 checklist。
5. 若涉及容量/并发结论，需确认日常回归口径是否仍为 `200 / 300 / 600`。

## 2. 发布前核对步骤

1. 检查 README 顶部版本号是否与本轮发布一致。
2. 检查 `RELEASE_NOTES.md` 是否新增或更新对应版本段落。
3. 检查 `docs/devlog.md` 是否记录本轮发布相关工作。
4. 检查 `docs/PRODUCT_LAUNCH_PLAYBOOK.md` 的阶段状态是否与实际进度一致。
5. 检查 `docs/ROADMAP.md` 的 P0/P1 与近期里程碑是否和 Playbook 当前计划一致。
6. 检查 `docs/RELEASE_ROLLBACK_RUNBOOK.md` 是否包含阶段 B 按小时执行步骤（T-24h 至 T+2h）。
7. 检查 `docs/BACKEND_TODO_CHECKLIST.md` 的勾选状态是否已同步。
8. 检查新文档是否已加入 README 文档索引。
9. 执行一次密钥扫描（`bash scripts/scan_secrets.sh`）并留存结果。
10. 检查性能与并发文档结论是否一致：
	- `docs/perf_reports/PERFORMANCE_BASELINE_20260425.md`
	- `docs/perf_reports/concurrent_connections_20260425.md`
	- `docs/perf_reports/concurrency_test_matrix_20260425.md`
11. 检查并发回归口径是否仍为 `200 / 300 / 600`，且 `400 / 500` 仅作为探索档位。
12. 检查告警链路配置是否可加载：`docker-compose -f docker-compose.prod.yml config`，并确认 `monitoring/prometheus/prometheus.yml` 已配置 Alertmanager 目标。
13. 检查 `docs/perf_reports/message_history_benchmark_summary.md` 是否同步了 `PERF_WARN_RATIO / PERF_WARN_FAIL / gate_mode / gate_source`。

## 3. 建议的人工核对命令

```bash
grep -n '^# C++ AI Chat Server' README.md
grep -n '^# Release Notes - v' RELEASE_NOTES.md
rg -n '^## 📅 2026-' docs/devlog.md
rg -n 'P0|P1|M1|M2|M3|A1|A2|A3|B3' docs/ROADMAP.md docs/PRODUCT_LAUNCH_PLAYBOOK.md
rg -n 'T-24h|T-4h|T-1h|T0|T\+0.5h|T\+2h' docs/PRODUCT_LAUNCH_PLAYBOOK.md docs/RELEASE_ROLLBACK_RUNBOOK.md
bash scripts/scan_secrets.sh
docker-compose -f docker-compose.prod.yml config
rg -n 'alertmanagers|alertmanager:9093' monitoring/prometheus/prometheus.yml monitoring/alertmanager/alertmanager.yml docker-compose.prod.yml
rg -n '200 / 300 / 600|400 / 500|高档位基线' docs/PRODUCT_LAUNCH_PLAYBOOK.md docs/perf_reports/concurrent_connections_20260425.md docs/perf_reports/concurrency_test_matrix_20260425.md README.md
rg -n 'PERF_WARN_RATIO|PERF_WARN_FAIL|gate_mode|gate_source' docs/perf_reports/message_history_benchmark_summary.md docs/PERFORMANCE_REPORT_TEMPLATE.md .github/workflows/backend-checks.yml
bash scripts/audit_backend_governance.sh
```

## 4. 记录要求

若本次发布涉及文档版本变更，请在发布记录中附上：

- README 版本号
- Release Notes 版本号
- devlog 最新条目日期
- 发布文档核对结果

## 5. 常见风险

- README 已更新但 Release Notes 未新增版本段落。
- devlog 只记录实现过程，没有记录最终发布结论。
- 新增治理文档后没有补 README 入口，导致后续维护者找不到。
- 版本号升级后未同步更新发布记录模板。

## 6. 关联文档

- `README.md`
- `RELEASE_NOTES.md`
- `docs/devlog.md`
- `docs/PRODUCT_LAUNCH_PLAYBOOK.md`
- `docs/ROADMAP.md`
- `docs/RELEASE_ROLLBACK_RUNBOOK.md`
- `docs/BACKEND_TODO_CHECKLIST.md`
- `docs/PERF_GATE_CHANGE_POLICY.md`
- `docs/perf_reports/PERFORMANCE_BASELINE_20260425.md`
- `docs/perf_reports/concurrent_connections_20260425.md`
- `docs/perf_reports/concurrency_test_matrix_20260425.md`
