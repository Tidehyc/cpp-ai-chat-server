# 发布记录 drill-20260424g

- 时间: 2026-04-25 19:38:14
- 执行人: tide
- 编排文件: docker-compose.prod.yml
- 前端端口: HTTP=18080, HTTPS=18443
- 发布状态: success

## 执行命令

1. docker-compose -f docker-compose.prod.yml down --remove-orphans
2. docker-compose -f docker-compose.prod.yml up -d --build
3. docker-compose -f docker-compose.prod.yml exec -T backend curl -fsS http://127.0.0.1:8080/health
4. curl -kfsS https://127.0.0.1:18443/health

## 容器状态摘要

          Name                         Command                       State                                                Ports                                      
---------------------------------------------------------------------------------------------------------------------------------------------------------------------
cpp-ai-chat-backend         /usr/bin/tini -- ./chat_server   Up (healthy)            8080/tcp                                                                        
cpp-ai-chat-blackbox        /bin/blackbox_exporter --c ...   Up                      9115/tcp                                                                        
cpp-ai-chat-frontend        /docker-entrypoint.sh ngin ...   Up (health: starting)   0.0.0.0:18443->443/tcp,:::18443->443/tcp, 0.0.0.0:18080->80/tcp,:::18080->80/tcp
cpp-ai-chat-grafana         /run.sh                          Up                      127.0.0.1:3000->3000/tcp                                                        
cpp-ai-chat-node-exporter   /bin/node_exporter --path. ...   Up                      9100/tcp                                                                        
cpp-ai-chat-prometheus      /bin/prometheus --config.f ...   Up                      127.0.0.1:9090->9090/tcp                                                        

## 备注

- 如需回滚，执行: ./scripts/rollback_prod.sh drill-20260424g
