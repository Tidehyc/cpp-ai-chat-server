# 分支保护与 Required Checks 配置指南

本文给出 cpp-ai-chat-server 的推荐分支保护策略，目标是让性能趋势检查在关键分支形成稳定门禁。

## 适用范围

- 目标分支：`main`
- CI 工作流：`.github/workflows/backend-checks.yml`
- 关键检查：后端构建 + CTest + 性能趋势检查

## 推荐策略

1. 对 `main` 开启分支保护。
2. 要求 PR 必须通过状态检查（Require status checks to pass）。
3. 将后端检查设为 required。
4. 禁止直接 push 到 `main`（仅通过 PR 合并）。

## GitHub UI 操作步骤

1. 进入仓库 `Settings` -> `Branches`。
2. 在 `Branch protection rules` 新建或编辑 `main` 规则。
3. 勾选 `Require a pull request before merging`。
4. 勾选 `Require status checks to pass before merging`。
5. 在 required checks 列表中选择后端检查项。

建议选择名称（以仓库实际显示为准）：

- `backend-checks`
- 若界面按工作流/作业展开显示，通常为 `backend-checks / backend-checks`

6. 可选勾选 `Require branches to be up to date before merging`。
7. 保存规则。

## 与当前 CI 策略对应关系

当前 workflow 已按分支自动切换性能告警门禁：

- `main` 分支 push：`PERF_WARN_FAIL=1`
- 目标分支为 `main` 的 PR：`PERF_WARN_FAIL=1`
- 其他分支：`PERF_WARN_FAIL=0`

因此把 `backend-checks` 设为 required 后，`main` 会自动启用更严格的性能回退门禁。

## 合并前自检命令

```bash
# 日常检查（仅告警）
bash scripts/run_backend_checks.sh

# 严格门禁模式（触发 warning 即失败）
PERF_WARN_FAIL=1 PERF_WARN_RATIO=0.80 bash scripts/run_backend_checks.sh
```

## 常见问题

1. 找不到 required check 名称
- 先触发一次 PR CI，等待检查出现在 PR 页面后再回到 Branch protection 选择。

2. 检查项命名不一致
- GitHub 可能显示为工作流名或作业名组合，优先选择包含 `backend-checks` 的那一项。

3. 想对 release 分支也启用严格门禁
- 复制 `main` 的分支保护规则到 `release/*`，并保留 required checks 设置。

## 关联文档

- `docs/CONTRIBUTING_BACKEND.md`
- `docs/BRANCH_PROTECTION_RELEASE_TEMPLATE.md`
