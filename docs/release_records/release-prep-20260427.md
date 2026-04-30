# 发布收口核对记录（草稿）

日期：2026-04-27  
范围：`copilot/perf-gate-fix-20260427` -> `main`  
状态：进行中（待 PR 合并）

---

## 1. 待合并提交

- `6a2a8da` `fix(ci): classify perf gate warnings correctly`
- `8c6b623` `fix(perf): stabilize benchmark gate and csv output`

## 2. 已完成准备项

- [x] 发布说明草稿已同步到 `RELEASE_NOTES.md`（v1.1.4 Draft）
- [x] 发布前文档核对清单已可执行（含 Playbook/runbook 一致性）
- [x] 治理巡检脚本检查通过（`bash scripts/audit_backend_governance.sh`）

## 3. 合并前检查项

- [ ] PR 创建并关联上述 2 个提交
- [ ] required checks 全绿
- [ ] 复核 `docs/RELEASE_DOC_CHECKLIST.md` 关键项

## 4. 合并后收尾项

- [ ] 在 `docs/PRODUCT_LAUNCH_PLAYBOOK.md` 勾选“合并 perf-gate 修复分支”
- [ ] 在 `docs/ROADMAP.md` 勾选主线收口剩余 2 项
- [ ] 追加 devlog 合并结果记录（包含 PR 链接与 merge commit）

## 5. 备注

- 受保护分支策略生效，`main` 不可直推，必须通过 PR 合并。
