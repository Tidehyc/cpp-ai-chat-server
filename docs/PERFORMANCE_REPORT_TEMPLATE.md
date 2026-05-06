# 性能报告模板（Message History）

## 基础信息

- 日期：YYYY-MM-DD
- 执行人：
- 分支/提交：
- 环境：CPU / 内存 / OS
- 数据规模：rows=50000, rounds=100（或自定义）

## 门禁阈值上下文

- PERF_WARN_RATIO：
- PERF_WARN_FAIL：
- gate_mode：strict / warn-only
- gate_source：env / cli / workflow
- 当前门禁模式：main/release 严格门禁 or feature 告警模式
- 阈值来源：默认 / 本次临时覆盖 / 已变更策略
- 策略文档：`docs/PERF_GATE_CHANGE_POLICY.md`

## 执行命令

```bash
python3 scripts/benchmark_message_history.py
```

或统一入口：

```bash
bash scripts/run_backend_checks.sh
```

## 原始输出

```text
=== Message History Benchmark ===
rows=50000, rounds=100
indexed_total_ms=...
indexed_avg_ms=...
no_index_total_ms=...
no_index_avg_ms=...
speedup=...x
```

## 结果摘要

- 有索引总耗时：... ms
- 无索引总耗时：... ms
- 加速比：...x
- 回归结论：通过 / 失败

## 结论与建议

- 是否满足当前性能基线：是 / 否
- 是否需要新增索引或改写 SQL：
- 后续行动项：
  - [ ] 
  - [ ] 

## 阈值治理记录（如发生调整）

- 是否涉及阈值变更：是 / 否
- 记录文件：`docs/perf_reports/perf_gate_changes/YYYYMMDD-HHMM-<topic>.md`
- 模板：`docs/perf_reports/PERF_GATE_CHANGE_RECORD_TEMPLATE.md`
