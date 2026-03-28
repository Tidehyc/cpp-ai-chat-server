# 阶段 4.3 开发计划：消息搜索 + 历史

_创建日期：2026-03-28_
_状态：进行中_

---

## 📋 目标

实现消息历史记录、搜索和导出功能。

---

## ✅ 已完成工作

### 1. 数据模块（100%）

**文件**：
- `src/history/message_history.h` - 数据结构和接口
- `src/history/message_history.cpp` - 实现

**功能**：
- ✅ MessageRecord 数据结构
- ✅ SearchCriteria 搜索条件
- ✅ MessageHistoryManager 管理器
- ✅ `get_history()` - 获取历史消息
- ✅ `search()` - 搜索消息
- ✅ `export_messages()` - 导出消息（JSON/TXT）

**代码量**：170+ 行

### 2. 编译集成（100%）

- ✅ CMakeLists.txt 更新
- ✅ 基础模块编译通过

---

## ⚪ 待完成工作

### 3. 集成到服务器（预计 2-3 小时）

**修改文件**：
- `src/main.cpp` - 初始化 MessageHistoryManager
- `src/server/server.h/cpp` - 传递 MessageHistoryManager
- `src/server/connection.h` - 添加引用
- `src/server/connection.cpp` - 构造函数更新

**工作量**：
- 修改 5 个文件
- 新增代码约 50 行

---

### 4. 命令实现（预计 2-3 小时）

**新增命令**：

#### 4.1 `/history` 命令
```bash
# 查看 AI 对话历史
/history @ai 10

# 查看房间历史
/history #code 20

# 查看私聊历史
/history alice 30
```

**实现位置**：`connection.cpp`
**代码量**：约 40 行

#### 4.2 `/search` 命令
```bash
# 关键词搜索
/search 快速排序

# 按用户搜索
/search from:alice

# 按房间搜索
/search in:#code

# 组合搜索
/search from:alice 快速排序
```

**实现位置**：`connection.cpp`
**代码量**：约 60 行

#### 4.3 `/export` 命令
```bash
# 导出 AI 对话
/export @ai

# 导出房间记录
/export #code

# 指定格式
/export @ai format=json
/export @ai format=txt
```

**实现位置**：`connection.cpp`
**代码量**：约 40 行

---

### 5. 命令注册和 help 更新（预计 30 分钟）

**修改文件**：
- `connection.cpp` - 命令注册和实现
- `connection.h` - 命令声明
- `cmd_help()` - 更新帮助信息

**代码量**：约 30 行

---

### 6. 测试（预计 1-2 小时）

**测试用例**：

1. **历史记录**
   - [ ] `/history @ai 10` - 查看 AI 历史
   - [ ] `/history #code 20` - 查看房间历史
   - [ ] 边界测试（limit=0, limit=1000）

2. **搜索功能**
   - [ ] `/search 关键词` - 关键词搜索
   - [ ] `/search from:user` - 按用户搜索
   - [ ] `/search in:room` - 按房间搜索
   - [ ] 组合搜索

3. **导出功能**
   - [ ] `/export @ai` - 导出为文本
   - [ ] `/export @ai format=json` - 导出为 JSON
   - [ ] 大文件导出（1000+ 条消息）

---

## 📊 工作量估算

| 任务 | 预计工时 | 完成度 |
|------|---------|--------|
| 数据模块 | 2h | 100% |
| 编译集成 | 0.5h | 100% |
| 服务器集成 | 2-3h | 0% |
| 命令实现 | 2-3h | 0% |
| 命令注册 | 0.5h | 0% |
| 测试 | 1-2h | 0% |
| **总计** | **8-10h** | **20%** |

---

## 🎯 下一步

### 立即执行

1. **集成到服务器**（2-3 小时）
   - 修改 main.cpp
   - 修改 server.h/cpp
   - 修改 connection.h/cpp

2. **实现命令**（2-3 小时）
   - `/history` 命令
   - `/search` 命令
   - `/export` 命令

3. **测试**（1-2 小时）
   - 功能测试
   - 边界测试
   - 性能测试

---

## 📝 注意事项

1. **数据库性能**
   - 搜索时添加索引
   - 限制最大返回数量

2. **内存管理**
   - 大量消息导出时分批处理
   - 避免内存泄漏

3. **用户体验**
   - 搜索结果显示格式
   - 导出文件保存位置

---

_最后更新：2026-03-28_
