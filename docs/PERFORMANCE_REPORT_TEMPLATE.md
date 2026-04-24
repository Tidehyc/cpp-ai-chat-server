# 性能报告模板（Message History）

## 基础信息

- 日期：YYYY-MM-DD
- 执行人：
- 分支/提交：
- 环境：CPU / 内存 / OS
- 数据规模：rows=50000, rounds=100（或自定义）

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
