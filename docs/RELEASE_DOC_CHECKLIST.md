# 发布文档核对清单

本文用于每次发布前核对 README、Release Notes、devlog 和相关治理文档是否一致。

## 1. 版本同步规则

发布前请确认以下信息一致或有明确说明：

- README 顶部版本号
- `RELEASE_NOTES.md` 的当前版本标题与发布日期
- `docs/devlog.md` 的最新发布日期与发布说明
- `docs/PRODUCT_LAUNCH_PLAYBOOK.md` 中当前阶段状态
- `docs/BACKEND_TODO_CHECKLIST_V1.3.md` 的完成状态
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
5. 检查 `docs/BACKEND_TODO_CHECKLIST_V1.3.md` 的勾选状态是否已同步。
6. 检查新文档是否已加入 README 文档索引。
7. 检查性能与并发文档结论是否一致：
	- `docs/perf_reports/PERFORMANCE_BASELINE_20260425.md`
	- `docs/perf_reports/concurrent_connections_20260425.md`
	- `docs/perf_reports/concurrency_test_matrix_20260425.md`
8. 检查并发回归口径是否仍为 `200 / 300 / 600`，且 `400 / 500` 仅作为探索档位。

## 3. 建议的人工核对命令

```bash
grep -n '^# C++ AI Chat Server' README.md
grep -n '^# Release Notes - v' RELEASE_NOTES.md
rg -n '^## 📅 2026-' docs/devlog.md
rg -n '200 / 300 / 600|400 / 500|高档位基线' docs/PRODUCT_LAUNCH_PLAYBOOK.md docs/perf_reports/concurrent_connections_20260425.md docs/perf_reports/concurrency_test_matrix_20260425.md README.md
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
- `docs/BACKEND_TODO_CHECKLIST_V1.3.md`
- `docs/PERF_GATE_CHANGE_POLICY.md`
- `docs/perf_reports/PERFORMANCE_BASELINE_20260425.md`
- `docs/perf_reports/concurrent_connections_20260425.md`
- `docs/perf_reports/concurrency_test_matrix_20260425.md`
