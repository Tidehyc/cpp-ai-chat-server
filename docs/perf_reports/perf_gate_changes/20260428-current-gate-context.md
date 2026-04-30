# 性能门禁阈值调整记录

## 基础信息

- 时间：2026-04-28  
- 执行人：Copilot  
- 评审人：待补充  
- 关联 PR：无（当前为门禁口径草稿）

## 变更内容

- 调整项：无，本次仅记录当前门禁口径
- 变更前：PERF_WARN_RATIO=0.80，PERF_WARN_FAIL=0
- 变更后：PERF_WARN_RATIO=0.80，PERF_WARN_FAIL=0
- 当前门禁模式：warn-only / strict（按分支策略切换）
- 门禁来源：cli / workflow

## 变更原因

- 触发背景：B3 门禁上下文已从脚本、模板、CI summary、README 四处收口，先形成一份可复用的当前口径草稿。
- 是否存在误报/漏报：否
- 业务与数据规模变化说明：当前仍以 `rows=50000, rounds=100` 的消息历史基准与 `200 / 300 / 600` 并发回归口径为准。

## 影响评估

- 对主分支门禁影响：无
- 对 feature 分支开发体验影响：无
- 风险等级：低

## 证据链接

- 趋势摘要文件：[docs/perf_reports/message_history_benchmark_summary.md](../message_history_benchmark_summary.md)
- 相关 benchmark 原始报告：
  - `docs/perf_reports/message_history_benchmark_*.txt`
- backend-checks job summary：

```text
PERF_WARN_RATIO=0.80
PERF_WARN_FAIL=0
gate_mode=warn-only
gate_source=cli
```

## 验证结果

- [x] CI 通过且门禁行为符合预期（当前为脚本/文档口径对齐）
- [x] Job Summary 分类与建议输出正常
- [x] 文档同步已完成

## 回滚预案

- 回滚条件：若后续阈值调整导致 3 次以内出现明显误判或漏报
- 回滚步骤：恢复上一个稳定阈值，并在记录中补充回滚原因与影响
- 预计恢复时间：待后续阈值调整时评估

## 备注

- 当前文件为门禁口径草稿，不代表阈值已发生调整。
- workflow 侧已约定 `gate_source=workflow`，本文件中的 `cli` 代表本地汇总脚本当前输出源。
