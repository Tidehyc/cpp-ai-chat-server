# 性能基线记录（2026-04-25）

## 基础信息

- 日期：2026-04-25
- 执行人：tide
- 分支/提交：main / cf9b031
- 环境：Linux 6.17.0-19-generic #19~24.04.2-Ubuntu SMP PREEMPT_DYNAMIC Fri Mar  6 23:08:46 UTC 2
- 数据规模：rows=50000, rounds=100

## 门禁阈值上下文

- PERF_WARN_RATIO：0.80
- PERF_WARN_FAIL：0
- 当前门禁模式：feature 告警模式
- 阈值来源：默认
- 策略文档：[docs/PERF_GATE_CHANGE_POLICY.md](../PERF_GATE_CHANGE_POLICY.md)

## 执行命令

```bash
/usr/bin/python3 scripts/benchmark_message_history.py
/usr/bin/python3 scripts/summarize_perf_reports.py --warn-ratio 0.80
```

## 原始输出

```text
=== Message History Benchmark ===
rows=50000, rounds=100
indexed_total_ms=127.22
indexed_avg_ms=1.2722
no_index_total_ms=7539.98
no_index_avg_ms=75.3998
speedup=59.27x
```

## 结果摘要

- 有索引总耗时：127.22 ms
- 无索引总耗时：7539.98 ms
- 加速比：59.27x
- 回归结论：通过，但低于历史均值 80% 阈值，已触发告警

## 复跑结果（同日三次）

- 第 1 次复跑：117.91x
- 第 2 次复跑：124.00x
- 第 3 次复跑：140.87x

### 复跑观察

- 同一脚本在同机连续复跑时，加速比明显高于首次基线，说明缓存状态与系统负载对结果影响很大。
- 目前更适合作为“趋势观察值”，不宜把单次 59.27x 直接视为稳定下限。

## 结论与建议

- 是否满足当前性能基线：是，当前仍保持数量级上的索引收益
- 是否需要新增索引或改写 SQL：暂不需要，优先观察是否存在环境波动
- 后续行动项：
  - [x] 复跑 3 次以上，确认 59x 左右结果是否可复现
  - [x] 已确认结果存在明显热缓存波动，后续需区分冷/热缓存基线
  - [ ] 若后续还要做稳定门禁，再补固定缓存条件或更严格的环境隔离

## 阈值治理记录（如发生调整）

- 是否涉及阈值变更：否
- 记录文件：无
- 模板：[docs/perf_reports/PERF_GATE_CHANGE_RECORD_TEMPLATE.md](PERF_GATE_CHANGE_RECORD_TEMPLATE.md)