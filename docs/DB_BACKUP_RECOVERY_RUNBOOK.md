# 数据库备份与恢复演练手册

本文定义 SQLite 数据库的备份、保留、恢复与演练步骤，作为阶段 B2 的执行依据。

## 1. 目标

- 每日生成一份可恢复的数据库备份。
- 保留最近 7 天备份，必要时可扩展到 14 天。
- 确保能在可控时间内恢复到可服务状态。

## 2. 备份策略

建议采用“每日全量备份 + 轮转保留”的方式。

脚本会优先从 Docker volume 中读取数据库文件；如果本地开发目录里已有 `build/chat.db`、`chat.db` 或 `data/chat.db`，也会自动作为兜底来源。

### 推荐命令

```bash
bash scripts/backup_db.sh
```

### 备份产物

- 默认路径：`backups/db/`
- 文件命名：`chat.db.YYYYMMDD_HHMMSS.sqlite3`

### 保留策略

- 保留最近 7 个每日备份。
- 若发布窗口或故障频繁，可临时提升到 14 个。
- 过期备份由运维定期清理。

## 3. 恢复策略

恢复前请确认：

1. 当前备份文件可读取。
2. 后端服务可短暂停机。
3. 恢复目标版本已确认。

恢复脚本同样支持本地开发路径兜底；如果 Docker volume 里暂时没有数据库文件，会自动尝试本地数据库路径。

### 推荐命令

```bash
bash scripts/restore_db.sh
```

如果希望恢复后立刻做健康检查和关键表抽样，也可以直接运行：

```bash
bash scripts/verify_restore_db.sh
```

也可以显式指定备份文件：

```bash
bash scripts/restore_db.sh backups/db/chat.db.20260425_030000.sqlite3
```

或者一键恢复并验证：

```bash
bash scripts/verify_restore_db.sh backups/db/chat.db.20260425_030000.sqlite3
```

## 4. 恢复演练步骤

1. 先执行一次备份，确认产物存在。
2. 选取最近一次备份进行恢复演练。
3. 恢复后执行健康检查。
4. 访问前端，确认历史消息和用户数据正常。
5. 记录恢复总耗时与是否出现数据损坏。
6. 如需快速复核，可直接执行 `bash scripts/verify_restore_db.sh [备份文件]`。

## 5. 验收标准

- 能成功恢复到健康状态。
- 恢复后 `/health` 正常。
- 关键数据集（用户、消息、房间）可查询。
- 恢复流程有明确的操作记录。

## 6. 回滚预案

若恢复失败：

1. 立即停止恢复流程。
2. 保留原备份文件不覆盖。
3. 回到上一个已验证备份重新恢复。
4. 记录失败原因并修订脚本。

## 7. 关联脚本与文档

- `scripts/backup_db.sh`
- `scripts/restore_db.sh`
- `scripts/verify_restore_db.sh`
- `docs/DEPLOYMENT.md`
- `docs/PRODUCT_LAUNCH_PLAYBOOK.md`
