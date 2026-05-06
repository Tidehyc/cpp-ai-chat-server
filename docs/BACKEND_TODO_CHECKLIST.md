# 后端代办清单（Backend TODO Checklist）

_版本：v1.3.0（当前）_
_更新日期：2026-04-23_

---

## 说明

本清单承接 v1.2.0 完成后的下一阶段工作，重点从“功能可用”转向“流程规范化 + 发布治理 + 可观测性增强”。

- 🔴 P0：必须优先完成，直接影响受保护分支协作与发布风险
- 🟡 P1：质量增强项，提升可维护性和告警可操作性
- 🟢 P2：持续优化项，降低长期维护成本

---

## 🔴 P0（先做）

### 1. 贡献流程标准化（受保护分支模型）

- [x] 在贡献文档明确“feature branch -> PR -> required checks -> merge”
- [x] 统一 PR 模板（变更范围、验证命令、风险说明、回滚方案）
- [x] 补充规则命名常见差异说明（workflow/job 展示名）
- [x] 增加最小自检命令清单（本地可复现实操）

**涉及模块**：`README.md`, `docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md`, 新增 `docs/CONTRIBUTING_BACKEND.md`

**验收标准**：

- 新成员可按文档独立完成一次标准 PR 提交流程
- 文档中包含可直接复制执行的命令，不依赖口头传达

---

### 2. Release 分支保护模板落地

- [x] 为 `release/*` 设计与 `main` 一致的 required checks 基线
- [x] 输出“主分支/发布分支”差异化规则建议（如是否必须 up-to-date）
- [x] 形成仓库设置清单（便于仓库管理员逐项核对）

**涉及模块**：`docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md`, 新增 `docs/BRANCH_PROTECTION_RELEASE_TEMPLATE.md`

**验收标准**：

- 文档可覆盖 `main` 与 `release/*` 两类规则配置
- 至少一次 release 分支规则人工核对演练

---

## 🟡 P1（次优先）

### 3. CI 失败分层与动作指引

- [x] 将 backend-checks 失败原因分层：构建失败 / 测试失败 / 性能门禁失败
- [x] 在 Job Summary 中输出“下一步处理建议”
- [x] 统一失败退出码语义并写入文档

**涉及模块**：`.github/workflows/backend-checks.yml`, `scripts/run_backend_checks.sh`, `scripts/summarize_perf_reports.py`, `README.md`

**验收标准**：

- 失败时可在 Job Summary 一眼定位类别
- 不同失败类别至少各有 1 条可执行修复建议

---

### 4. 性能门禁阈值治理与变更流程

- [x] 定义门禁阈值变更流程（谁可改、何时改、如何记录）
- [x] 增加阈值调整记录模板（含变更理由与影响评估）
- [x] 将阈值策略与趋势报告模板联动

**涉及模块**：`README.md`, `docs/PERFORMANCE_REPORT_TEMPLATE.md`, 新增 `docs/PERF_GATE_CHANGE_POLICY.md`

**验收标准**：

- 任一阈值变更都可追溯（理由、时间、执行人）
- 报告模板中包含阈值上下文信息

---

## 🟢 P2（持续优化）

### 5. 例行巡检自动化脚本

- [x] 新增轻量巡检脚本（检查工作流文件、门禁参数、关键文档链接一致性）
- [x] 输出机器可读结果（0/1 + 简要报告）
- [x] 接入本地 pre-release 自检说明

**涉及模块**：新建 `scripts/audit_backend_governance.sh`, `README.md`

**验收标准**：

- 一条命令可完成治理配置健康检查
- 巡检输出可直接贴到 PR 评论

---

### 6. 文档版本一致性治理

- [x] 建立版本同步规则（README / RELEASE_NOTES / devlog）
- [x] 在发布前核对版本号、日期、关键特性摘要一致
- [x] 新增“发布文档核对清单”

**涉及模块**：`README.md`, `RELEASE_NOTES.md`, `docs/devlog.md`, 新增 `docs/RELEASE_DOC_CHECKLIST.md`

**验收标准**：

- 每次发布文档核对步骤固定化
- 无“README 版本落后于 Release Notes”的情况

---

## 里程碑建议

- 里程碑 A（本周）：完成 P0-1、P0-2
- 里程碑 B（下周）：完成 P1-3、P1-4
- 里程碑 C（后续）：推进 P2 并形成发布前巡检流程

---

## 进度记录

- [x] 2026-04-W4：v1.3 清单创建
- [x] 2026-04-W5：P0 完成
- [x] 2026-05-W1：P1 完成（含 P1-3、P1-4）
- [x] 2026-05-W2：P2-5 完成
- [x] 2026-05-W2：P2-6 完成
