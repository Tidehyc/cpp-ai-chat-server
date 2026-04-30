# 发布与回滚演练手册

版本: v1.1  
日期: 2026-04-27

---

## 1. 目标

将发布和回滚动作固化为可重复执行流程，确保:

- 每次发布有一致步骤
- 回滚有明确触发条件和执行路径
- 产出可审计的记录文件

范围边界:

- 本手册服务于阶段 A+B（公开 Beta）上线与回滚。
- 阶段 C（商用 GA）不在当前执行范围内。

---

## 2. 适用范围

- 生产编排: docker-compose.prod.yml
- 发布脚本: scripts/release_prod.sh
- 回滚脚本: scripts/rollback_prod.sh
- 记录目录: docs/release_records/

---

## 3. 发布前检查

1. 拉取最新代码并确认目标分支/标签
2. 确认 .env 完整，尤其 AI_API_KEY 与 Grafana 管理员密码
3. 确认证书文件已挂载到 nginx/ssl
4. 执行编排语法检查

```bash
docker-compose -f docker-compose.prod.yml config
```

如果本地尚未准备证书，发布脚本会自动在 `nginx/ssl/` 生成自签名证书，用于演练环境启动。

补充检查（建议）:

```bash
bash scripts/audit_backend_governance.sh
```

```bash
docker-compose -f docker-compose.prod.yml ps
```

---

## 4. 发布步骤

执行:

```bash
./scripts/release_prod.sh <release_version>
```

说明:

- release_version 可选，默认使用时间戳
- 脚本会自动:
  - 构建并启动容器
  - 校验后端与前端健康检查
  - 如缺少证书则自动生成自签名证书
  - 生成发布记录到 docs/release_records/

发布后立即执行:

```bash
docker-compose -f docker-compose.prod.yml ps
```

---

## 5. 回滚触发条件

满足任一条件建议执行回滚:

1. 后端健康检查持续失败
2. 前端 HTTPS/WSS 无法建立连接
3. 核心功能回归导致服务不可用
4. 告警达到 critical 且短期无法恢复

---

## 6. 回滚步骤

执行:

```bash
./scripts/rollback_prod.sh <target_version>
```

说明:

- target_version 可选，用于记录目标版本
- 脚本会自动:
  - 停止当前容器
  - 重新构建并拉起可部署状态
  - 健康检查验证
  - 生成回滚记录到 docs/release_records/

回滚后立即执行:

```bash
docker-compose -f docker-compose.prod.yml ps
```

---

## 7. 发布后验收

1. 后端健康检查:

```bash
curl -fsS http://127.0.0.1:8080/health
```

2. 前端健康检查:

```bash
curl -kfsS https://127.0.0.1/health
```

3. 监控与告警:

- Grafana 能访问并看到 Chat Server Ops Overview 看板
- Prometheus Targets 页面采集目标均为 UP

---

## 8. 记录规范

每次发布/回滚必须保留:

- 执行时间
- 执行人
- 执行命令
- 容器状态摘要
- 问题与处置说明

可参考模板:

- docs/release_records/TEMPLATE.md

建议在记录中附上以下命令输出摘要:

```bash
git --no-pager rev-parse --short HEAD
docker-compose -f docker-compose.prod.yml ps
curl -fsS http://127.0.0.1:8080/health
curl -kfsS https://127.0.0.1/health
```

---

## 9. 风险提示

- 当前回滚脚本默认回滚到“当前仓库可部署状态”，不自动切换历史 commit/tag。
- 如需精确版本回滚，先执行 git checkout 到目标版本，再执行回滚脚本。

---

## 10. 上线日按小时执行版（阶段 B）

### T-24h（冻结与备份）

1. 冻结发布范围，仅允许阻断级修复。
2. 执行备份并记录备份文件名。
3. 明确角色：发布执行人、回滚执行人、值班联系人。

建议命令:

```bash
bash scripts/backup_db.sh
ls -lh backups/db | tail -n 5
```

### T-4h（治理与准入）

1. 治理巡检。
2. 核对 required checks 与文档一致性。
3. 检查监控与告警通道可用。

建议命令:

```bash
bash scripts/audit_backend_governance.sh
```

### T-1h（发布前健康确认）

1. 检查当前容器状态。
2. 记录当前版本与可回滚点。
3. 进行发布前健康探测。

建议命令:

```bash
git --no-pager rev-parse --short HEAD
docker-compose -f docker-compose.prod.yml ps
curl -fsS http://127.0.0.1:8080/health
curl -kfsS https://127.0.0.1/health
```

### T0（发布窗口）

1. 执行发布脚本。
2. 检查核心路径：登录、消息、AI、历史、健康检查。

建议命令:

```bash
./scripts/release_prod.sh <release_version>
```

### T+0.5h（稳态观察）

1. 观察错误率、延迟、连接数、资源占用。
2. 触发阻断条件则执行回滚。

建议命令:

```bash
./scripts/rollback_prod.sh <target_version>
```

### T+2h（收口与复盘）

1. 输出发布/回滚记录。
2. 更新 devlog 与发布记录。
3. 形成遗留风险与下一步处理单。

---

## 11. 记录模板（可直接复制）

```text
[发布记录]
时间:
执行人:
版本:
命令:
健康检查结果:
容器状态摘要:
告警状态:
问题与处置:

[回滚记录]
时间:
执行人:
目标版本:
触发原因:
命令:
回滚后健康检查结果:
回滚后容器状态摘要:
后续行动:
```
