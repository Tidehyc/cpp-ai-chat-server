# A1 安全基线核对记录（2026-04-27）

范围：阶段 A1（密钥治理、环境变量治理、敏感日志审计）  
执行人：Copilot  
状态：进行中

---

## 1. 密钥扫描

执行命令：

```bash
bash scripts/scan_secrets.sh
# 可选：包含本地未入库文件（如 .env）
bash scripts/scan_secrets.sh --all-files
```

结果：默认（tracked）模式已执行并通过。

执行结果（2026-04-27）：

```text
SCAN_STATUS=OK
SCAN_SUMMARY=No secret-like patterns found (placeholders ignored)
```

补充说明：

- 默认模式只扫描 Git 已入库文件，用于判定“无明文密钥入库”。
- `--all-files` 模式用于本地环境自检，可能命中开发者本地 `.env`，不作为入库判定依据。

`--all-files` 抽样结果（2026-04-27）：

```text
./.env:6:AI_API_KEY=***
./nginx/ssl/privkey.pem:1:[PRIVATE_KEY_HEADER_REDACTED]
SCAN_STATUS=WARN
SCAN_SUMMARY=Potential secret-like matches found; manual review required
```

解读：

- `.env` 与 `nginx/ssl/privkey.pem` 为本地运行资产，应保留在 `.gitignore` 范围内，不允许入库。
- `--all-files` 告警用于提醒本地敏感文件管控；A1 入库判定以默认 `tracked` 模式为准。

## 2. 环境变量来源梳理

已识别关键变量：

- `AI_API_KEY`
- `AI_API_URL`
- `AI_MODEL`
- `SERVER_PORT`
- `DB_PATH`
- `LOG_FILE`
- `GRAFANA_ADMIN_PASSWORD`

主要来源文件：

- `src/main.cpp`
- `docker-compose.yml`
- `docker-compose.prod.yml`
- `README.md`
- `docs/DEPLOYMENT.md`

## 3. 敏感日志审计

目标：确认日志不输出凭据正文、token 明文、Authorization 头。

结果：待执行并回填。

审计摘要（2026-04-27）：

- `src/ai/ai_client.cpp` 使用 Authorization 头发送请求，但未写入日志。
- `src/user/user_manager.cpp` 仅记录 `token_len` 与匹配结果，不输出 token 正文。
- 未发现 `Logger::*` 直接输出 API key / Bearer token 明文。

## 4. 结论

- [x] 无明文密钥入库（基于 `tracked` 扫描）
- [x] 环境变量来源清晰
- [x] 敏感日志脱敏通过（当前代码抽样）
