# 学习笔记 - 阶段 4.1

_版本：v0.4.1_
_最后更新：2026-03-28_

---

## 📚 阶段 4.1：群聊基础

**完成时间**：2026-03-27 ~ 2026-03-28

**核心目标**：实现群聊房间和消息广播功能

---

### 一、聊天室模块架构

#### 1.1 核心类设计

```cpp
// 聊天室
class ChatRoom {
    std::string name_;
    std::set<ConnectionPtr> members_;  // 成员连接
    
    void join(ConnectionPtr conn);
    void leave(ConnectionPtr conn);
    void broadcast(const std::string& message);
};

// 聊天室管理器
class ChatRoomManager {
    std::map<std::string, std::unique_ptr<ChatRoom>> rooms_;
    std::map<ConnectionPtr, std::set<std::string>> user_rooms_;
    
    bool create_room(const std::string& name);
    bool join_room(ConnectionPtr conn, const std::string& room_name);
    void leave_all_rooms(ConnectionPtr conn);
};
```

#### 1.2 消息广播流程

```
用户 A 发送消息
    ↓
Connection::handle_chat_message()
    ↓
broadcast_to_room(message)
    ↓
遍历 all_connections_
    ↓
筛选同一房间的连接
    ↓
发送消息
```

---

### 二、用户状态管理

#### 2.1 状态类型

| 状态 | 图标 | 说明 |
|------|------|------|
| `online` | 🟢 | 在线 |
| `busy` | 🔴 | 忙碌 |
| `away` | 🟡 | 离开 |
| `offline` | ⚪ | 离线 |

#### 2.2 状态流转

```
用户连接
    ↓
未登录（AUTHENTICATED）
    ↓
登录 → online
    ↓
设置状态 → busy/away
    ↓
退出/断开 → offline
```

#### 2.3 服务器重启状态重置

**问题**：`pkill -9` 杀死服务器时，无法执行清理逻辑

**解决方案**：服务器启动时重置所有用户状态

```cpp
UserManager::UserManager(Database* db) {
    // ... 初始化用户表
    reset_all_users_offline();  // 重置所有用户为 offline
}

void UserManager::reset_all_users_offline() {
    const char* sql = "UPDATE users SET status = 'offline'";
    sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, nullptr);
}
```

---

### 三、客户端信号处理

#### 3.1 Ctrl+C 退出问题

**问题**：`std::getline` 阻塞，信号处理无法中断

**解决方案**：关闭 stdin 中断阻塞

```cpp
void signal_handler(int signum) {
    std::fclose(stdin);  // 关闭 stdin
    g_running = false;
}

// 主循环
while (g_running) {
    std::cout << "> ";
    std::getline(std::cin, line);
    
    if (!g_running || std::cin.fail()) {
        break;  // 退出循环
    }
}
```

#### 3.2 退出流程

```
Ctrl+C
    ↓
signal_handler(SIGINT)
    ↓
关闭 stdin + 设置 g_running=false
    ↓
getline 返回（cin.fail()=true）
    ↓
发送 /quit 到服务器
    ↓
服务器更新状态为 offline
    ↓
关闭连接
```

---

### 四、房间分类设计

#### 4.1 AI 房间 vs 群聊房间

```
房间系统
├── AI 房间（/rooms）
│   ├── #general - 友好助手
│   ├── #code - 编程助手
│   ├── #translate - 翻译
│   ├── #learn - 学习
│   ├── #write - 写作
│   └── #debug - 调试
│
└── 群聊房间（/list）
    ├── 用户创建的房间
    └── 消息广播功能
```

#### 4.2 命令设计

```bash
# AI 房间
/rooms                # 列出 AI 房间
/join <房间名>         # 加入房间

# 群聊房间
/list                 # 列出群聊房间
/create room <名称>   # 创建房间

# 用户系统
/register <用户> <密码>  # 注册
/login <用户> <密码>     # 登录
/status [状态]          # 设置状态
/online                 # 查看在线用户
```

---

### 五、关键技术点

#### 5.1 静态成员管理

```cpp
class Connection {
    static std::set<std::string> created_rooms_;     // 房间列表
    static std::set<ConnectionPtr> all_connections_; // 连接列表
};

// 初始化
std::set<std::string> Connection::created_rooms_;
std::set<ConnectionPtr> Connection::all_connections_;
```

#### 5.2 消息广播过滤

```cpp
void Connection::broadcast_to_room(const std::string& message) {
    for (auto& conn : all_connections_) {
        if (conn && 
            conn.get() != this && 
            conn->session_.current_room == session_.current_room &&
            conn->session_.is_authenticated()) {
            conn->send_system_message(message);
        }
    }
}
```

#### 5.3 数据库查询优化

```cpp
// 查询所有非 offline 用户
const char* sql = R"(
    SELECT id, username, status
    FROM users
    WHERE status != 'offline'
    ORDER BY username
)";
```

---

### 六、遇到的问题及解决

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 消息不广播 | 未维护连接列表 | 添加 `static all_connections_` |
| Ctrl+C 不能退出 | getline 阻塞 | 关闭 stdin 中断 |
| 服务器重启状态不更新 | SIGKILL 无法捕获 | 启动时重置状态 |
| `/online` 只显示 online | SQL 条件限制 | 修改为 `!= 'offline'` |
| 客户端输入无响应 | 非阻塞输入不可靠 | 恢复使用 getline |

---

### 七、经验总结

#### 7.1 进程与信号

- **SIGKILL (pkill -9)**：无法捕获，进程立即终止
- **SIGINT (Ctrl+C)**：可以捕获，需要中断阻塞操作
- **最佳实践**：重要清理逻辑不能完全依赖信号处理

#### 7.2 状态管理

- **启动时重置**：防止遗留状态
- **正常退出更新**：保持状态准确
- **析构函数兜底**：双重保障

#### 7.3 用户体验

- **状态可视化**：使用图标（🟢🔴🟡⚪）
- **退出提示**：友好的退出消息
- **命令响应**：及时反馈

---

## 📊 阶段 4.1 代码统计

| 模块 | 新增代码 | 修改代码 | 说明 |
|------|---------|---------|------|
| ChatRoom | 150+ | - | 聊天室模块 |
| Connection | - | 200+ | 广播功能 + 状态管理 |
| UserManager | - | 50+ | 状态重置 |
| ws_client | - | 100+ | Ctrl+C 处理 |
| **总计** | **150+** | **350+** | |

---

_最后更新：2026-03-28_
