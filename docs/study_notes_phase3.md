# 学习笔记 - 阶段 3

---

## 📚 阶段 3：房间系统 + 私聊 AI

**完成时间**：2026-03-27

**核心目标**：实现"房间决定 AI 功能，私聊自定义 AI 设定"

---

## 一、架构设计

### 1.1 模块划分

```
┌─────────────────────────────────────────────────────────┐
│                     Connection                           │
│  (命令处理中枢)                                          │
├─────────────────────────────────────────────────────────┤
│  UserManager  │  RoomManager  │  PresetManager          │
│  用户管理     │  房间管理     │  预设管理               │
├─────────────────────────────────────────────────────────┤
│                     Database (SQLite)                    │
└─────────────────────────────────────────────────────────┘
```

### 1.2 设计模式

**管理器模式**：
```cpp
class UserManager {
    Database* db_;  // 依赖注入
    std::set<int> online_users_;
    
    bool register_user(...);
    bool login(...);
    User get_user(...);
};
```

**依赖注入**：
```cpp
// Connection 接收所有管理器指针
Connection::Connection(
    Database* db, 
    AIClient* ai_client,
    UserManager* user_manager, 
    RoomManager* room_manager, 
    PresetManager* preset_manager
)
```

---

## 二、核心技术点

### 2.1 命令解析

**问题**：`/preset use teacher` 被解析为 `args = ["use teacher"]`

**原因**：参数字符串未分割

**解决方案**：
```cpp
// 错误写法
std::vector<std::string> args_vec;
if (!args.empty()) {
    args_vec.push_back(args);  // 整个字符串作为一个元素
}

// 正确写法
std::vector<std::string> args_vec;
if (!args_str.empty()) {
    std::istringstream iss(args_str);
    std::string arg;
    while (iss >> arg) {
        args_vec.push_back(arg);  // 按空格分割
    }
}
```

### 2.2 连接状态机

```cpp
enum class ConnectionState {
    HANDSHAKE,      // WebSocket 握手
    AUTHENTICATED,  // 已登录
    ACTIVE,         // 活跃状态
    CLOSING,        // 关闭中
    CLOSED          // 已关闭
};
```

**状态流转**：
```
HANDSHAKE → AUTHENTICATED → ACTIVE → CLOSING → CLOSED
```

### 2.3 Session 设计

```cpp
struct Session {
    int user_id = 0;
    std::string username;
    std::string current_room = "general";  // 当前房间
    std::string current_preset = "friend"; // 当前预设
    
    bool is_authenticated() const { return user_id > 0; }
};
```

---

## 三、C++ 技术要点

### 3.1 前向声明

**问题**：头文件循环依赖

```cpp
// connection.h 中
#include "user_manager.h"
#include "room_manager.h"
// ...

// user_manager.h 中
#include "connection.h"  // 循环依赖！
```

**解决**：
```cpp
// user_manager.h
namespace chat {
    class Database;  // 前向声明
    class UserManager;
}
```

### 3.2 字符串分割

**方法 1：istringstream**
```cpp
std::vector<std::string> split(const std::string& str) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        result.push_back(token);
    }
    return result;
}
```

**方法 2：find + substr**
```cpp
std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> result;
    size_t start = 0, end;
    while ((end = str.find(delim, start)) != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    result.push_back(str.substr(start));
    return result;
}
```

### 3.3 JSON 与纯文本兼容

**问题**：客户端发送 JSON，服务器期望纯文本

**解决**：
```cpp
void Connection::handle_command(const std::string& input) {
    // 尝试解析 JSON
    try {
        Message msg = JsonParser::parse(input);
        if (!msg.content.empty()) {
            handle_command(msg.content);  // 递归处理
            return;
        }
    } catch (...) {
        // 不是 JSON，继续处理为命令
    }
    
    // 处理命令...
}
```

### 3.4 中文字符串处理

**问题**：`substr()` 可能截断 UTF-8 多字节字符

**解决**：
- 避免在字节中间截断
- 使用完整字符串或按字符边界截断

```cpp
// 错误：可能截断 UTF-8
std::string preview = text.substr(0, 20);

// 正确：不截断或使用完整字符串
send_system_message("已加入房间 #" + room_name);
```

---

## 四、调试技巧

### 4.1 日志定位

```cpp
Logger::info("Connection " + std::to_string(id_) + " command: " + input);
```

**用途**：
- 追踪命令执行流程
- 定位参数解析问题

### 4.2 分段测试

```bash
# 1. 测试连接
./ws_client

# 2. 测试登录
/login testuser

# 3. 测试房间
/rooms
/join code

# 4. 测试 AI
写一个排序
```

### 4.3 strace 跟踪

```bash
# 跟踪系统调用
strace -f ./chat_server 2>&1 | tail -50

# 查看崩溃原因
strace ./chat_server 2>&1 | grep -A5 "SIG"
```

---

## 五、数据库设计

### 5.1 表结构

```sql
-- 用户表
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT,
    current_preset TEXT DEFAULT 'friend',
    status TEXT DEFAULT 'online',
    created_at INTEGER NOT NULL
);

-- 房间表
CREATE TABLE rooms (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL,
    ai_prompt TEXT NOT NULL,
    ai_temperature REAL DEFAULT 0.7,
    max_context INTEGER DEFAULT 10,
    created_by INTEGER,
    created_at INTEGER NOT NULL
);

-- 用户预设表
CREATE TABLE user_presets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    prompt TEXT NOT NULL,
    is_default INTEGER DEFAULT 0,
    UNIQUE(user_id, name)
);
```

---

## 六、经验总结

### 6.1 成功之处

1. **模块化设计** - 各模块职责清晰，易于维护
2. **命令扩展性** - 添加新命令只需注册 handler
3. **用户体验** - 纯文本客户端，命令直观易用
4. **文档完善** - MANUAL.md 提供完整使用指南

### 6.2 待改进

1. **错误处理** - 部分函数缺少错误返回
2. **内存管理** - 部分地方使用裸指针
3. **并发安全** - 多连接共享数据需加锁
4. **性能优化** - AI 响应可异步处理

### 6.3 关键教训

1. **常量名要准确** - `SHA_DIGEST_LENGTH` vs `SHA256_DIGEST_LENGTH`
2. **参数分割很重要** - 使用 `istringstream` 而非直接 push
3. **UTF-8 处理要小心** - 避免在多字节字符中间截断
4. **日志要及时** - 关键路径都要有日志

---

## 七、技术对比

| 技术 | 优点 | 缺点 |
|------|------|------|
| 管理器模式 | 职责清晰，易测试 | 代码量增加 |
| 依赖注入 | 解耦，易替换 | 构造函数参数多 |
| 状态机 | 状态清晰，易调试 | 状态转换要仔细 |
| 命令模式 | 易扩展，易维护 | 需要解析器 |

---

_版本：v0.3.0_
_最后更新：2026-03-27_
