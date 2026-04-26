# 并发压测参数矩阵（2026-04-25）

## 1. 目的

统一记录“连接容量”和“命令路径”两类压测在不同参数下的结果，避免后续回归重复试错。

## 2. 说明

- 入口统一为：`wss://127.0.0.1:18443/ws`
- 压测脚本：`scripts/test_concurrent_connections.py`
- `connect_only` 用于验证连接容量基线
- `rooms_json` 用于验证命令路径行为（会受命令限流影响）

## 3. 参数矩阵与结果

| 模式 | clients | waves | timeout | batch_size | batch_interval_ms | 成功率 | 备注 |
|---|---:|---:|---:|---:|---:|---:|---|
| connect_only | 200 | 1 | 5 | 0 | 0 | 100.00% | 连接容量基线通过 |
| connect_only | 250 | 1 | 5 | 0 | 0 | 100.00% | 连接容量扩展通过 |
| connect_only | 300 | 1 | 5 | 0 | 0 | 100.00% | 连接容量扩展通过 |
| rooms_json | 200 | 1 | 5 | 0 | 0 | 100.00% | 命令模式在该并发可稳定通过 |
| rooms_json | 300 | 1 | 5 | 0 | 0 | 13.33% | 紧接前一次执行，命令限流窗口累积影响明显 |
| rooms_json | 300 | 1 | 5 | 0 | 0 | 80.00% | 重启 backend 后复测（240/300），与命令限流窗口一致 |
| rooms_json | 300 | 1 | 5 | 30 | 9000 | 100.00% | 分批发送后恢复 300/300 |
| connect_only | 400 | 1 | 5 | 0 | 0 | 100.00% | 连接容量继续扩展通过 |
| rooms_json | 400 | 1 | 5 | 40 | 9000 | 90.00% | 第 7 批出现限流（0/40），总计 360/400 |
| rooms_json | 400 | 1 | 5 | 25 | 6000 | 97.50% | 第 10 批有 10 个请求触发限流 |
| rooms_json | 400 | 1 | 5 | 20 | 5000 | 100.00% | 参数搜索中的更优节奏（400/400） |
| connect_only | 500 | 1 | 5 | 0 | 0 | 100.00% | 连接容量继续扩展通过 |
| rooms_json | 500 | 1 | 5 | 20 | 5000 | 100.00% | 推荐节奏在 500 并发下仍稳定 |
| connect_only | 600 | 1 | 5 | 0 | 0 | 100.00% | 连接容量继续扩展通过 |
| rooms_json | 600 | 1 | 5 | 20 | 5000 | 100.00% | 推荐节奏在 600 并发下仍稳定 |

## 4. 推荐回归顺序

日常回归建议固定为三档：

1. `connect_only` 200：确认基础连接能力。
2. `rooms_json` 300：确认命令路径与限流窗口是否符合预期。
3. `connect_only` / `rooms_json` 600（推荐节奏 `20/5000`）：确认高档基线仍然稳定。

只有在需要继续找上限时，才临时加做 400 / 500 / 700+ 的探索档位。

## 5. 推荐命令

```bash
# 连接容量基线
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode connect_only --clients 300 --waves 1 --timeout 5

# 命令路径基线
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode rooms_json --clients 200 --waves 1 --timeout 5

# 高并发命令（分批）
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode rooms_json --clients 300 --waves 1 --timeout 5 --batch-size 30 --batch-interval-ms 9000

# 400 并发对照（连接容量 / 命令模式）
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode connect_only --clients 400 --waves 1 --timeout 5
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode rooms_json --clients 400 --waves 1 --timeout 5 --batch-size 40 --batch-interval-ms 9000

# 400 并发推荐参数（分批优化）
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode rooms_json --clients 400 --waves 1 --timeout 5 --batch-size 20 --batch-interval-ms 5000

# 500 并发对照
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode connect_only --clients 500 --waves 1 --timeout 5
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode rooms_json --clients 500 --waves 1 --timeout 5 --batch-size 20 --batch-interval-ms 5000

# 600 并发对照
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode connect_only --clients 600 --waves 1 --timeout 5
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode rooms_json --clients 600 --waves 1 --timeout 5 --batch-size 20 --batch-interval-ms 5000
```

## 6. 结论

- 当前环境下，连接容量与命令吞吐应分开验证。
- 在不调整限流阈值前提下，分批发送是高并发命令压测的有效稳定策略。
- 在 400 并发下，连接容量仍稳定（100%）；命令模式可通过分批参数优化到 100%（`20/5000`）。
- 在 500 并发下，连接容量与命令模式在推荐节奏下仍保持 100%。
- 在 600 并发下，连接容量与命令模式在推荐节奏下仍保持 100%。
- 日常回归不需要每次都扫 400 / 500 / 600；优先按 200 / 300 / 600 三档固定执行。