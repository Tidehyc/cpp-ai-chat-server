# GitHub Release 指南

_版本：v1.0.0_

---

## 📦 创建 Release 步骤

### 1. 提交代码到 Git

```bash
cd /home/tide/workplace/cpp-ai-chat-server

# 初始化仓库（如果还没有）
git init

# 添加所有文件
git add .

# 提交
git commit -m "feat: Initial release v1.0.0

Features:
- WebSocket server with Boost.Asio
- AI integration with context support
- Multi-room system
- User management
- Message search and history
- Docker deployment
- Unit tests (100% pass rate)
- Logging system with spdlog"

# 添加标签
git tag -a v1.0.0 -m "Release v1.0.0 - Initial release"
```

### 2. 推送到 GitHub

```bash
# 添加远程仓库（替换为你的仓库地址）
git remote add origin https://github.com/YOUR_USERNAME/cpp-ai-chat-server.git

# 推送主分支
git push -u origin main

# 推送标签
git push origin v1.0.0
```

### 3. 在 GitHub 上创建 Release

**方式 1：使用 GitHub Web 界面**

1. 访问：https://github.com/YOUR_USERNAME/cpp-ai-chat-server/releases
2. 点击 "Draft a new release"
3. 选择标签：`v1.0.0`
4. 填写标题：`v1.0.0 - Initial Release`
5. 粘贴下面的 Release Notes
6. 点击 "Publish release"

**方式 2：使用 GitHub CLI**

```bash
# 安装 gh（如果还没有）
# Ubuntu: sudo apt install gh
# macOS: brew install gh

# 创建 release
gh release create v1.0.0 \
  --title "v1.0.0 - Initial Release" \
  --notes-file RELEASE_NOTES.md \
  --generate-notes
```

---

## 📝 Release Notes 模板

使用 `RELEASE_NOTES.md` 文件的内容作为 Release Notes。

---

## 🏷️ 标签命名规范

遵循语义化版本控制（Semantic Versioning）：

- `v1.0.0` - 主版本号。重大变更
- `v1.1.0` - 次版本号。新功能
- `v1.1.1` - 修订号。Bug 修复

---

## 📊 Release 检查清单

创建 Release 前确认：

- [ ] 所有测试通过
- [ ] 文档完整
- [ ] Docker 镜像构建成功
- [ ] 代码无编译警告
- [ ] LICENSE 文件存在
- [ ] README.md 更新
- [ ] CHANGELOG.md 更新
- [ ] 版本号已更新

---

## 🔗 相关链接

- [GitHub Releases 文档](https://docs.github.com/en/repositories/releasing-projects-on-github)
- [语义化版本规范](https://semver.org/)
- [GitHub CLI 文档](https://cli.github.com/)

---

_最后更新：2026-03-28_
