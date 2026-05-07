# 项目路线图 (Roadmap)

版本：v1.2.0-planning
更新日期：2026-04-28
适用范围：跨端里程碑与上线节奏（细项执行见专项清单）

---

## 文档入口

- 上线总手册：docs/PRODUCT_LAUNCH_PLAYBOOK.md
- 后端专项执行清单：docs/BACKEND_TODO_CHECKLIST.md
- 发布文档核对清单：docs/RELEASE_DOC_CHECKLIST.md
- 开发学习日志：docs/devlog.md

说明：本路线图以“上线风险收敛”为主轴，优先级与 Product Launch Playbook 保持一致。

---

## 当前状态（2026-04-27）

已完成能力（摘要）：

- 后端核心闭环：用户、好友、私聊、群聊、AI、历史
- 生产基线：HTTPS/WSS、反向代理安全头、容器非 root、健康检查
- 可观测基础：结构化日志、基础监控、阈值告警
- 稳定性基础：备份/恢复脚本、恢复演练、并发基线与报告
- CI 与分支保护：PR + required checks 已跑通

当前继续推进：

- P2 前端体验增强（消息体验、Markdown、代码高亮、通知）
- P3 合规与商业化能力
- 发布前文档核对与变更记录按 `docs/RELEASE_DOC_CHECKLIST.md` 常态化执行

---

## 优先级（按风险）

| 优先级 | 目标 | 当前状态 |
|--------|------|----------|
| P0 | 收口主线与上线风险闭环 | 已完成 |
| P1 | 稳态化性能门禁与发布治理 | 进行中 |
| P2 | 用户体验与运营能力增强 | 已完成 |
| P3 | 商用规模化与商业化能力 | 未开始 |

---

## P0（立即执行）

### 1) 主线收口：perf-gate 修复合并

状态：已完成
建议分支：copilot/perf-gate-fix-20260427（现有）

- [x] 合并 `fix(ci): classify perf gate warnings correctly`
- [x] 合并 `fix(perf): stabilize benchmark gate and csv output`
- [x] 同步发布说明与核对记录

验收：`main..HEAD` 为空，required checks 全绿。

### 2) A1 安全基线收尾

状态：已完成
建议分支：copilot/a1-secret-hygiene-20260427

- [x] 全仓密钥/凭据扫描（代码、脚本、文档）
- [x] 环境变量来源梳理（AI_API_KEY 等）
- [x] 敏感日志脱敏回归
- [x] 扫描策略区分 tracked（入库判定）与 --all-files（本地自检）

验收：扫描无明文密钥；日志样本不含凭据正文。

### 3) A2 可观测性补齐

状态：已完成
建议分支：copilot/a2-errorcode-alerting-20260427

- [x] 补齐关键路径 error_code 分类（登录、消息、AI、DB）
- [x] 告警通道落地（Webhook/企业 IM）
- [x] 完成一次告警演练并留证
- [x] 替换演练网关为真实告警网关并完成线上投递验证

验收：异常 5 分钟内可告警，`request_id + error_code` 可追踪。

### 4) A3 发布回滚实战

状态：已完成
建议分支：copilot/a3-release-rollback-drill-20260428

- [x] 完整执行发布 -> 验证 -> 回滚 -> 恢复验证
- [x] 记录耗时、风险点、RTO/RPO
- [x] 更新 runbook 与发布记录

验收：演练成功且无数据损坏，RTO/RPO 在目标内。

---

## P1（两周内）

### 5) B3 性能门禁稳态化

状态：已完成
建议分支：copilot/b3-perf-gate-stabilization-20260428

- [x] 固化日常回归档位：200 / 300 / 600
- [x] 区分告警阈值与失败阈值，减少误杀
- [x] 更新基线报告与门禁策略文档

验收：连续多次 CI 无 perf-gate 误杀，趋势波动可解释。

### 6) 文档治理持续化

状态：已完成

- [x] 发布前执行 `docs/RELEASE_DOC_CHECKLIST.md`
- [x] 在 `docs/PRODUCT_LAUNCH_PLAYBOOK.md` 维护阶段状态
- [x] 每轮关键变更补 `docs/devlog.md` 复盘条目

验收：README / RELEASE_NOTES / devlog / Playbook 叙事一致。

---

## P2（Beta 后）

### 8) 运营能力最小集

- [x] 用户封禁/解封
- [x] 广播与举报处理
- [x] 审计追踪最小集

### 9) 前端体验增强

- [x] 消息体验优化（已读状态、滚动与渲染细节）
- [x] Markdown 与代码高亮
- [x] 通知与多端体验增强

---

## P3（GA 前）

### 10) 合规与商业化能力

- [x] 隐私政策、用户协议、数据删除/导出流程
- [x] 审计留痕策略与值班闭环
- [x] 配额、账单、成本监控

---

## 近期里程碑

### M1：2026-04-27 ~ 2026-05-03（风险收敛）

- perf-gate 分支收口
- A1 安全基线收尾
- A2 可观测性补齐

### M2：2026-05-04 ~ 2026-05-10（稳定性实战）

- A3 发布/回滚演练收口
- B3 性能门禁稳态化

### M3：2026-05-11 ~ 2026-05-24（GA 前准备）

- 合规文档最小集
- 运营后台需求切片与实施计划

---

## 跟踪规则

- 每个风险主题单独分支、单独 PR。
- 每周更新一次本路线图“状态”与“里程碑”章节。
- 未达成验收标准的事项不得切换到下一阶段。
