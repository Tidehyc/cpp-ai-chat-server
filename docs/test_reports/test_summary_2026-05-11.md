测试汇总 — 2026-05-11

摘要
- 前端构建已通过：`cd frontend && npm run build`
- 前端开发服务器已可在局域网访问：`http://10.20.54.121:5173`
- 后端容器已恢复为局域网监听：`0.0.0.0:8080->8080/tcp`
- 后端 WebSocket 握手已验证通过：`HTTP/1.1 101 Switching Protocols`
- Docker 构建、容器健康检查和镜像体积优化均已完成并保持可用
- 移动端专项验收已按用户要求停止，不再作为当前交付项推进

当前可确认的结论
- 项目核心服务链路可用：前端页面可访问，后端容器健康，WebSocket 握手正常
- 前端已从“连接本机 localhost”调整为更适合局域网访问的方式
- 先前发现的 AI 页面回归已修复并回归验证通过
- 镜像体积已达标（95.1MB），Docker 收尾目标已满足
- 项目进入收尾状态，后续只保留必要的维护或用户新增需求

运行命令
```bash
cd frontend && npm run build
docker-compose ps
curl -I --max-time 5 http://10.20.54.121:5173
curl -i --max-time 5 -H 'Connection: Upgrade' -H 'Upgrade: websocket' -H 'Sec-WebSocket-Version: 13' -H 'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==' http://10.20.54.121:8080/
```

备注
- 本汇总记录的是当前收尾时点的已验证状态，不代表后续不会再有需求变更。
- 若后续需要恢复移动端专项验收，可直接从 [`docs/test_reports/mobile_manual_acceptance_2026-05-09.md`](mobile_manual_acceptance_2026-05-09.md) 继续。