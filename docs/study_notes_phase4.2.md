# 学习笔记 - 阶段 4.2：AI 对话上下文

_版本：v0.4.2_
_最后更新：2026-03-28_

---

## 📚 核心目标

实现 AI 多轮对话能力，让 AI 能够记住之前的对话内容。

---

## 一、架构设计

### 1.1 数据模型

```
ConversationContext
├── room_name: string       # 房间名
├── user_id: string         # 用户 ID
├── history: Message[]      # 对话历史
└── max_history: int        # 最大保留轮数（默认 10）
```

### 1.2 管理器模式

```
ConversationManager
├── contexts_: map<key, Context>  # 内存缓存
├── db_: Database                 # 数据库引用
│
├── get_context(room, user)       # 获取上下文
├── add_message(room, user, ...)  # 添加消息
├── clear_context(room, user)     # 清空上下文
├── save_context(room, user)      # 保存到数据库
└── load_context(room, user)      # 从数据库加载
```

---

## 二、关键技术点

### 2.1 上下文键名设计

```cpp
std::string get_key() const {
    if (user_id.empty()) {
        return room_name;  // 群聊：只用房间名
    }
    return room_name + ":" + user_id;  // 私聊：房间 + 用户
}
```

**说明**：
- 群聊：同一房间共享上下文
- 私聊：每个用户独立上下文

### 2.2 上下文存储策略

**内存 + 数据库双重存储**：

```
添加消息
    ↓
更新内存（contexts_）
    ↓
定期保存到数据库（conversations 表）
    ↓
服务器重启时从数据库加载
```

**优点**：
- 内存：快速访问
- 数据库：持久化，重启不丢失

### 2.3 AI 请求构造

```cpp
// 构造带上下文的请求
json request_body = {
    {"model", model_},
    {"messages", json::array()}
};

// 添加上下文消息
for (const auto& msg : context) {
    request_body["messages"].push_back({
        {"role", msg.role},      // "user" 或 "assistant"
        {"content", msg.content}
    });
}

// 添加当前消息
request_body["messages"].push_back({
    {"role", "user"},
    {"content", current_message}
});
```

---

## 三、数据库设计

### 3.1 conversations 表

```sql
CREATE TABLE conversations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_name TEXT NOT NULL,
    user_id TEXT,
    history_json TEXT,         -- JSON 格式存储历史
    updated_at INTEGER NOT NULL,
    UNIQUE(room_name, user_id)
);
```

### 3.2 JSON 格式

```json
{
  "history": [
    {
      "role": "user",
      "content": "我叫小明",
      "timestamp": 1711627200
    },
    {
      "role": "assistant",
      "content": "你好小明，很高兴认识你！",
      "timestamp": 1711627205
    }
  ]
}
```

---

## 四、命令设计

### 4.1 `/clear` - 清空上下文

```bash
/clear

# 服务器响应
对话历史已清空。
```

**实现**：
```cpp
void Connection::cmd_clear(...) {
    if (conversation_manager_) {
        conversation_manager_->clear_context(
            session_.current_room,
            std::to_string(session_.user_id)
        );
        send_system_message("对话历史已清空。");
    }
}
```

### 4.2 `/context` - 查看上下文

```bash
# 查看当前状态
/context
# 响应：当前上下文大小：5 条消息
#       最大历史：10 轮

# 设置最大历史
/context set 20
# 响应：已设置最大历史为 20 轮
```

---

## 五、使用场景

### 5.1 多轮对话

```
用户：我叫小明
AI:  你好小明，很高兴认识你！

用户：我今年 25 岁
AI:  好的，小明今年 25 岁。

用户：我是做什么工作的？
AI:  抱歉，你还没有告诉我你的工作。
```

### 5.2 房间隔离

```
房间 #general:
  用户：我喜欢苹果
  AI: 好的，你喜歡苹果。

房间 #code:
  用户：我喜欢什么？
  AI: 抱歉，我不知道你喜欢什么。
  （不同房间上下文隔离）
```

### 5.3 用户隔离

```
用户 A（私聊）:
  /msg ai 我叫小红
  AI: 你好小红...

用户 B（私聊）:
  /msg ai 我叫什么名字？
  AI: 抱歉，我不知道你的名字。
  （不同用户上下文隔离）
```

---

## 六、遇到的问题及解决

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| SSL connect error | CURL 证书验证 | `CURLOPT_SSL_VERIFYPEER=0` |
| 不完整类型错误 | 前向声明 | include 完整头文件 |
| 函数重载歧义 | 重复声明 | 删除旧声明 |
| 上下文不一致 | 命令路径不同 | 统一使用 `/msg ai` |

---

## 七、性能优化

### 7.1 内存管理

```cpp
void ConversationContext::add_message(...) {
    history.push_back(message);
    
    // 保持历史记录在限制范围内
    while (history.size() > max_history * 2) {
        history.erase(history.begin());
    }
}
```

**说明**：每轮对话 2 条消息（user + assistant），限制为 `max_history * 2`

### 7.2 数据库优化

```cpp
// 使用 INSERT OR REPLACE
INSERT OR REPLACE INTO conversations (...)
VALUES (?, ?, ?, ?)
```

**说明**：避免重复记录，自动更新

---

## 八、最佳实践

### 8.1 上下文大小设置

| 场景 | 推荐值 | 说明 |
|------|--------|------|
| 日常聊天 | 5-10 轮 | 节省 token |
| 技术咨询 | 10-20 轮 | 保持上下文 |
| 代码调试 | 20-30 轮 | 完整错误信息 |

### 8.2 清空时机

- 话题切换时
- 上下文过大时
- 隐私敏感时

---

## 九、代码统计

| 文件 | 代码量 | 说明 |
|------|--------|------|
| conversation.h | 70+ | 数据结构 |
| conversation.cpp | 180+ | 管理器实现 |
| ai_client.* | 50+ | AI 上下文支持 |
| connection.* | 100+ | 消息处理集成 |
| **总计** | **400+** | |

---

_最后更新：2026-03-28_
