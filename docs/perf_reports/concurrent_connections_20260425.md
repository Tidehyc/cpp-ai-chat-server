# 并发连接压测记录（2026-04-25）

## 基础信息

- 日期：2026-04-25
- 执行人：tide
- 分支/提交：main / cf9b031
- 入口：`wss://127.0.0.1:18443/ws`
- 脚本：`scripts/test_concurrent_connections.py`
- 参数：clients=30, waves=3, timeout=5

## 执行命令

```bash
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --clients 30 --waves 3 --timeout 5
```

## 原始输出

```text
[wave 1] ok=30/30
[wave 2] ok=30/30
[wave 3] ok=30/30
[summary] total=90, failed=0, success_rate=100.00%
```

## 结果摘要

- 总连接数：90
- 失败数：0
- 成功率：100.00%

## 追加验证（命令模式扩展）

### 追加场景

- `rooms_json` 模式，`clients=200, waves=1`：`success_rate=100.00%`
- `rooms_json` 模式，`clients=300, waves=1`（紧接上一次执行）：`success_rate=13.33%`
- 重启 backend 后再次执行 `rooms_json` 300 并发：`success_rate=80.00%`（`240/300`）

### 失败特征

- 失败样本主要为：`unexpected response type: system`
- 该系统消息对应命令路径限流反馈，不是 WebSocket 连接握手失败。

## 追加验证（分批发送策略）

### 执行参数

- 模式：`rooms_json`
- 并发：`clients=300, waves=1`
- 分批：`batch_size=30, batch_interval_ms=9000`

### 执行命令

```bash
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode rooms_json --clients 300 --waves 1 --timeout 5 --batch-size 30 --batch-interval-ms 9000
```

### 结果

- `[summary] total=300, failed=0, success_rate=100.00%`

### 解释

- 分批后命令请求在时间窗口内被平滑展开，避免了单时间片触发命令限流阈值。
- 该策略适合压测环境用于分离“容量问题”与“限流策略问题”。

## 追加验证（400 并发分批命令模式）

### 执行参数

- 模式：`rooms_json`
- 并发：`clients=400, waves=1`
- 分批：`batch_size=40, batch_interval_ms=9000`

### 执行命令

```bash
.venv/bin/python scripts/test_concurrent_connections.py --uri wss://127.0.0.1:18443/ws --mode rooms_json --clients 400 --waves 1 --timeout 5 --batch-size 40 --batch-interval-ms 9000
```

### 结果

- `[summary] total=400, failed=40, success_rate=90.00%`
- 第 7 批 `0/40`，其余批次均通过。

### 解释

- 400 并发分批下仍会触达命令限流边界，说明当前节奏参数在该并发级别下已接近上限。

## 追加验证（400 并发参数搜索）

### 对比结果

- `batch_size=40, batch_interval_ms=9000` => `90.00%`（`360/400`）
- `batch_size=25, batch_interval_ms=6000` => `97.50%`（`390/400`）
- `batch_size=20, batch_interval_ms=5000` => `100.00%`（`400/400`）

### 推荐参数

- 当前环境建议优先使用：`batch_size=20, batch_interval_ms=5000`
- 该参数在不调整命令限流阈值的前提下可达 `400/400`。

## 追加验证（500 并发）

### 执行参数

- `connect_only`：`clients=500, waves=1`
- `rooms_json`：`clients=500, waves=1, batch_size=20, batch_interval_ms=5000`

### 结果

- 连接容量：`[summary] total=500, failed=0, success_rate=100.00%`
- 命令模式（推荐节奏）：`[summary] total=500, failed=0, success_rate=100.00%`

### 解释

- 500 并发在推荐节奏下仍可稳定通过，说明当前命令路径的节奏控制策略还有余量。

## 追加验证（600 并发）

### 执行参数

- `connect_only`：`clients=600, waves=1`
- `rooms_json`：`clients=600, waves=1, batch_size=20, batch_interval_ms=5000`

### 结果

- 连接容量：`[summary] total=600, failed=0, success_rate=100.00%`
- 命令模式（推荐节奏）：`[summary] total=600, failed=0, success_rate=100.00%`

### 解释

- 600 并发下仍未触发连接瓶颈或推荐节奏下的命令限流，说明当前容量基线还有余量。
- 600 不是继续扩档的起点，而是当前高档位基线；日常回归可固定为 200 / 300 / 600。

## 结论与建议

- 当前 WSS `/ws` 入口在 90 个并发连接、3 个波次下稳定可用。
- 该脚本可以作为后续 B3 容量回归的基础入口，后续可按更高 clients 数继续拉高压力。
- `rooms_json` 在 300 并发下受命令限流阈值影响明显（重启后 240/300），应和纯连接容量基线分开解读。
- `rooms_json` 在 300 并发下使用分批策略（30/批，9s 间隔）可恢复到 300/300。
- `rooms_json` 在 600 并发下使用推荐节奏仍保持 600/600，当前节奏策略可继续沿用到更高档位。
- 日常回归建议固定为 200 / 300 / 600 三档；400 / 500 只在需要找上限时临时加测。

## 阈值治理记录（如发生调整）

- 是否涉及阈值变更：否
- 记录文件：无
- 备注：本次为稳定性验证，不引入新的门禁阈值。