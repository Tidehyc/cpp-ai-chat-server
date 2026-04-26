# 纯连接并发压测记录（2026-04-25）

## 基础信息

- 日期：2026-04-25
- 执行人：tide
- 分支/提交：main / cf9b031
- 入口：`wss://127.0.0.1:18443/ws`
- 脚本：`scripts/test_concurrent_connections.py`
- 模式：`connect_only`
- 参数：clients=200, waves=1, timeout=5

## 执行命令

```bash
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode connect_only --clients 200 --waves 1 --timeout 5
```

## 原始输出

```text
[wave 1] ok=200/200
[summary] total=200, failed=0, success_rate=100.00%
```

## 结果摘要

- 总连接数：200
- 失败数：0
- 成功率：100.00%

## 追加复跑（同日）

### 复跑参数

- `clients=250, waves=1, timeout=5`
- `clients=300, waves=1, timeout=5`

### 复跑结果

- 250 并发：`[summary] total=250, failed=0, success_rate=100.00%`
- 300 并发：`[summary] total=300, failed=0, success_rate=100.00%`

## 结论与建议

- 正式 WSS `/ws` 入口在 200/250/300 并发连接下均保持稳定。
- 这个结果适合作为“纯连接容量基线”，与 `rooms_json` 模式分开看，避免把命令限流误判成连接容量不足。
- 后续如果要更严格验证，可以在保持 `connect_only` 成功的前提下继续尝试 `clients=400+`。

## 阈值治理记录（如发生调整）

- 是否涉及阈值变更：否
- 记录文件：无
- 备注：本次为连接容量验证，不引入新的门禁阈值。