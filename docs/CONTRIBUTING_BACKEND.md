# 后端贡献流程（受保护分支）

本文定义后端代码的标准提交流程，目标是保证每次合并都经过一致校验，避免直接改 `main` 导致回归。

## 1. 标准流程

1. 从最新 `main` 拉取并创建功能分支。
2. 在功能分支开发与自测。
3. 提交 PR 到 `main` 或 `release/*`。
4. 等待 Required Checks 全通过。
5. 完成 Code Review 后再合并。

推荐分支命名：

- `feature/<topic>`
- `fix/<topic>`
- `refactor/<topic>`

## 2. 本地最小自检命令

在提交 PR 前，至少执行以下命令：

```bash
# 1) 后端构建 + CTest + 性能门禁（默认告警不失败）
bash scripts/run_backend_checks.sh

# 2) 严格门禁模式（warning 也失败），建议改动核心路径时执行
PERF_WARN_FAIL=1 PERF_WARN_RATIO=0.80 bash scripts/run_backend_checks.sh

# 3) 发布脚本语法检查
bash -n scripts/release_prod.sh && bash -n scripts/rollback_prod.sh
```

如果修改了容器/部署相关内容，建议追加：

```bash
docker-compose -f docker-compose.prod.yml config
```

## 3. PR 提交要求

请使用仓库 PR 模板，必须填写：

- 变更范围
- 验证命令与结果
- 风险评估
- 回滚方案

模板文件：

- `.github/pull_request_template.md`

## 4. Required Checks 命名差异说明

GitHub 在不同页面可能显示不同名称：

- 工作流名：`backend-checks`
- 工作流/作业组合：`backend-checks / backend-checks`

设置分支保护时，优先选择包含 `backend-checks` 的检查项。

参考：

- `docs/BRANCH_PROTECTION_REQUIRED_CHECKS.md`
- `docs/BRANCH_PROTECTION_RELEASE_TEMPLATE.md`

## 5. 禁止事项

- 禁止直接 push 到 `main`。
- 禁止跳过 Required Checks 合并。
- 禁止在未给出回滚方案时合并高风险改动。

## 6. 建议合并策略

- 小步提交，单 PR 只做一类改动。
- PR 描述中附关键日志/测试摘要，不只贴“已测试”。
- 涉及协议或数据结构变更时，同步更新文档与兼容说明。
