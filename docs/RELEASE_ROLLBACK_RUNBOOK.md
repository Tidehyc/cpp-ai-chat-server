# 发布与回滚演练手册

版本: v1.0  
日期: 2026-04-24

---

## 1. 目标

将发布和回滚动作固化为可重复执行流程，确保:

- 每次发布有一致步骤
- 回滚有明确触发条件和执行路径
- 产出可审计的记录文件

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

---

## 9. 风险提示

- 当前回滚脚本默认回滚到“当前仓库可部署状态”，不自动切换历史 commit/tag。
- 如需精确版本回滚，先执行 git checkout 到目标版本，再执行回滚脚本。
