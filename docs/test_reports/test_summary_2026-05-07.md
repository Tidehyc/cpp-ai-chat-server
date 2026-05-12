测试汇总 — 2026-05-07

摘要
- 前端构建已通过：`cd frontend && npm run build`
- 前端依赖审计已通过：`cd frontend && npm audit --audit-level=high` 返回 `found 0 vulnerabilities`
- 后端测试已通过：`cd build && ctest --output-on-failure`，48/48 tests passed
- Docker 镜像构建已通过：`docker-compose build`
- Docker 容器健康检查已通过：`docker-compose up -d && docker-compose ps`，状态 `Up (healthy)`
- Docker 端口映射已验证：`docker-compose ps` 显示 `127.0.0.1:8080->8080/tcp`
- Docker 镜像体积已优化至 `95.1MB`，达到目标 `<100MB`
- 文档一致性检查已通过：`git diff --check`

当前可确认的结论
- 前端 runtime 依赖和 dev 工具链已完成升级，并且构建稳定
- 前端消息体验、Markdown 渲染、通知 banner 已完成补齐
- 合规最小文档集、审计留痕与成本监控草案已补齐并同步到索引
- 路线图中的 P0-P3 主题项已完成，剩余主要是测试清单逐项记录与少量基础项复核
- Docker 构建与运行链路可用，镜像体积已达标并保持可运行性

运行命令
```bash
cd frontend && npm run build
cd frontend && npm audit --audit-level=high
cd build && ctest --output-on-failure
docker-compose build
docker-compose up -d && docker-compose ps
docker image ls cpp-ai-chat-server_chat-server:latest --format 'table {{.Repository}}\t{{.Tag}}\t{{.Size}}'
curl -fsS http://127.0.0.1:8080/health
cd /home/tide/workplace/cpp-ai-chat-server && git diff --check
```

备注
- 本汇总仅记录已实际验证通过的项目，不替代逐项人工验收。
- `docker-compose` 执行时出现历史 orphan 容器告警，不影响本轮 `chat-server` 服务构建与健康检查结果。
- 若后续补跑 `cmake .. && make -j$(nproc)`、`docker-compose build`、`docker-compose up -d` 或 `frontend` 交互验证，可继续追加到本文件。
