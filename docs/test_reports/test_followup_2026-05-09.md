后续验证清单 — 2026-05-09

目的
- 将当前仍未自动化确认的项目拆成独立后续任务，避免与已完成验证混在同一份收尾记录中。

已完成的自动化验证
- 前端构建通过
- 前端依赖审计通过
- 后端测试通过
- Docker 构建通过
- Docker 容器健康检查通过
- Docker 镜像体积已达标（95.1MB）

待补跑的后续项
- 移动端适配验证：布局、触摸操作、窄屏显示（已按用户要求停止，不再作为当前后续项）

已补跑（2026-05-09）
- 浏览器交互验证（基础流）通过：
	- 登录页可注册并登录测试用户 `auto_ui_0509`
	- 登录后私聊/群聊/AI 页面可正常切换
	- AI 页面消息可发送并收到回复
	- 设置页在当前浏览器中可正常打开
- 人工验收复核：从主界面可继续切换到群聊与 AI，设置页仍可正常打开
- 登录页复测通过：在会话回到登录页后，使用 `auto_ui_0509` 重新登录可正常回到主界面
- 设置页内容区已补充底部留白，降低窄屏下被底部导航遮挡的风险
- 设置页 DOM 已复核：内容容器含 `pb-28`，移动底栏组件已挂载但在桌面态为 `display: none`
- 日志轮转验证（探针容器）通过：
	- 使用临时容器 `log-rotate-probe` 触发 `json-file` 轮转
	- 在宿主挂载目录中观察到 `*-json.log` 与 `*-json.log.1`
- 登录页与注册页已复查：当前实现未发现额外的底部导航遮挡风险，无需再补修
- 修复一处前端回归：AI 页面发送消息后出现 `Maximum update depth exceeded`
	- 修复文件：`frontend/src/pages/AIAssistant.tsx`
	- 修复方式：稳定空消息引用，并将滚动副作用依赖收敛到消息数量

已确认的配置事实
- `docker inspect cpp-ai-chat-server` 显示日志驱动为 `json-file`，参数为 `max-size=10m`、`max-file=3`
- 这说明日志轮转策略已配置到运行容器，但实际滚动行为仍需在高日志量场景下再做一次确认

当前限制
- 当前会话账号无权限读取 `/var/lib/docker/containers/...` 下的宿主日志文件，无法直接确认 `*-json.log.1/.2/.3` 的生成情况
- 当前集成浏览器会话中，`page.setViewportSize({ width: 390, height: 844 })` 后页面 `window.innerWidth` 仍为 `1114`，无法可靠触发移动端断点样式

建议复核命令（需 root 或 docker 组具备对应文件访问权限）
```bash
LOG_PATH=$(docker inspect cpp-ai-chat-server --format '{{.LogPath}}')
sudo ls -lh "$(dirname "$LOG_PATH")" | grep json.log
```

建议执行顺序
1. 当前不再推进移动端适配验证

移动端验证建议（人工）
1. 当前不再执行；如未来重新开启，再按原流程补测。
2. 已整理一份当前状态记录：[`docs/test_reports/mobile_manual_acceptance_2026-05-09.md`](mobile_manual_acceptance_2026-05-09.md)

记录规范
- 每完成一项，就把对应结果回填到 [docs/TEST_CHECKLIST.md](../TEST_CHECKLIST.md) 的“已确认验证记录”或相关专项小节中。
- 如果发现回归，优先记录具体复现命令与日志片段，再修复实现。
