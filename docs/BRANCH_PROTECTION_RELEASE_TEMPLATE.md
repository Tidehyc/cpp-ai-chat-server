# Release 分支保护模板

本文提供 `release/*` 分支保护配置模板，并给出与 `main` 的差异建议。

## 1. 目标

- 保持 `release/*` 与 `main` 一致的核心质量门禁。
- 在上线窗口内减少“临时修复绕过检查”的风险。

## 2. 推荐规则（release/*）

分支规则匹配：`release/*`

建议开启：

1. Require a pull request before merging
2. Require status checks to pass before merging
3. Required checks 选择：
   - `backend-checks`
   - 或 `backend-checks / backend-checks`（视 UI 展示）
4. Restrict who can push to matching branches（可选，建议仅发布负责人）

## 3. 与 main 的差异建议

相同点：

- 必须走 PR
- 必须通过 `backend-checks`
- 禁止直接 push

差异点建议：

- `Require branches to be up to date before merging`
  - `main`：建议开启（保证主线始终基于最新）
  - `release/*`：可按窗口期决定
    - 发布冻结期建议开启
    - 紧急 hotfix 场景可临时关闭，但需在 PR 中记录原因

## 4. 仓库管理员核对清单

- [ ] `main` 规则已开启且 required checks 生效
- [ ] `release/*` 规则已创建
- [ ] `release/*` 已包含 `backend-checks` required check
- [ ] 已验证一次 release 分支 PR，检查项正确触发
- [ ] 已确认是否启用 up-to-date 要求，并记录策略

## 5. 一次人工演练建议

1. 创建 `release/test-policy` 分支。
2. 提交一个文档改动 PR 到该分支。
3. 确认 `backend-checks` 正常触发并可见。
4. 验证 PR 未通过前不可合并。
5. 演练结束后删除测试分支。

## 6. 关联文档

- `docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md`
- `docs/CONTRIBUTING_BACKEND.md`
