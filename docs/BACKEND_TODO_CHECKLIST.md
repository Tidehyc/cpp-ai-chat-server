# 后端代办清单（Backend TODO Checklist）

_版本：v1.2.0（完成）_
_更新日期：2026-04-22_

---

## 说明

本清单聚焦后端可执行改进项，按优先级排序。

- 🔴 P0：先做，影响稳定性或安全性
- 🟡 P1：次优先，影响性能和可维护性
- 🟢 P2：持续优化项

---

## 🔴 P0（先做）

### 1. 密码存储升级为带盐哈希

- [x] 设计密码字段升级方案（salt + hash）
- [x] 实现注册/登录兼容逻辑
- [x] 为旧用户提供平滑迁移策略
- [x] 新增单元测试覆盖正常/异常登录

**涉及模块**：`src/user/user_manager.cpp`

**验收标准**：

- 数据库不再存储纯 SHA256 结果
- 新老账号均可登录
- 测试通过且不破坏现有注册流程

---

### 2. 补全消息协议序列化实现

- [x] 明确消息对象与 JSON 协议字段映射
- [x] 实现消息序列化/反序列化
- [x] 添加非法输入和缺字段防御
- [x] 补充协议层单元测试

**涉及模块**：`src/protocol/message.cpp`

**验收标准**：

- 协议转换逻辑可用且可回归测试
- 非法报文不会导致崩溃

---

### 3. 离线消息读写一致性（事务化）

- [x] 将离线消息拉取与已读更新纳入事务策略
- [x] 失败场景支持回滚或重试
- [x] 明确“推送成功”与“标记已读”顺序
- [x] 增加回归测试用例（部分失败场景）

**涉及模块**：`src/server/connection.cpp`, `src/friend/offline_message_delivery.cpp`, `tests/test_offline_message_delivery.cpp`

**验收标准**：

- 不出现“推送失败但已读”或“重复推送”异常
- 事务边界清晰且日志可追踪

---

### 4. 异步 AI 回调线程安全审计

- [x] 梳理 AI 异步回调线程与连接线程模型
- [x] 确保发送与状态变更在安全上下文执行
- [x] 修复潜在竞态访问
- [x] 增加并发场景稳定性验证

**涉及模块**：`src/ai/ai_client.cpp`, `src/server/connection.cpp`, `tests/test_ai_client.cpp`

**验收标准**：

- 并发请求下无崩溃、无明显数据竞争
- 消息发送时序稳定

---

## 🟡 P1（次优先）

### 5. 历史与搜索 SQL 优化

- [x] 梳理 history/search 热路径 SQL
- [x] 增加必要复合索引
- [x] 对比优化前后耗时
- [x] 新增性能回归测试（基础样本）

**涉及模块**：`src/history/message_history.cpp`, `src/storage/database.cpp`

**结果记录（2026-04-22）**：

- 基准脚本：`python3 scripts/benchmark_message_history.py`
- 样本规模：50,000 行消息，100 轮查询
- 有索引总耗时：27.42 ms（avg 0.2742 ms）
- 无索引总耗时：2243.97 ms（avg 22.4397 ms）
- 速度提升：约 81.83x

**回归测试**：

- `MessageHistoryTest.SearchPerformanceBaselineWithIndex`
- `MessageHistoryTest.SearchPerformanceRegressionIndexedShouldNotBeWorseThanNoIndex`

---

### 6. 规范数据库迁移流程

- [x] 增加 schema_version 管理
- [x] 统一迁移入口与执行顺序
- [x] 保证重复启动可幂等
- [x] 补充迁移回归测试

**涉及模块**：`src/storage/database.cpp`, `src/user/user_manager.cpp`

---

### 7. 日志脱敏与分级收敛

- [x] 审计日志中敏感内容输出
- [x] 默认隐藏用户隐私与消息正文
- [x] debug/info/warn/error 输出边界统一
- [x] 更新相关调试文档

**涉及模块**：`src/server/connection.cpp`, `src/utils/logger.cpp`

---

## 🟢 P2（持续优化）

### 8. 后端测试覆盖补强

- [x] 补充离线消息边界测试
- [x] 补充登录异常流测试
- [x] 补充搜索边界与空结果测试
- [x] 补充并发连接稳定性测试

**涉及模块**：`tests/test_user_manager.cpp`, `tests/test_message_history.cpp`, `tests/test_offline_messages.sh`

---

### 9. 命令处理模块解耦

- [x] 拆分 `Connection` 中过重命令逻辑
- [x] 明确解析层与执行层职责
- [x] 提升单命令可测试性
- [x] 减少大函数维护复杂度

**涉及模块**：`src/server/connection.cpp`, `src/command/command_dispatcher.cpp`, `src/command/command_parser.cpp`

**实现说明**：

- 新增 `CommandDispatcher` 类，负责解析层（从 "/" 前缀判断、分割 cmd_name + args）
- `Connection::execute_command()` 负责执行层（30+ 个 if-else 分支路由）
- 通过 `std::function<>` 回调模式解耦，避免循环依赖
- 支持单个命令独立测试，命令执行时序清晰

---

## 里程碑建议

- 里程碑 A（本周）：完成 P0-1 和 P0-2
- 里程碑 B（下周）：完成 P0-3 和 P0-4
- 里程碑 C：推进 P1 并补齐关键测试

---

## 进度记录

- [x] 2026-04-W4：P0 开始
- [x] 2026-04-W5：P0 完成
- [x] 2026-05-W1：P1 开始

---

## 下一阶段

- 下一阶段后端任务草案见：`docs/BACKEND_TODO_CHECKLIST_V1.3.md`
