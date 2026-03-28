# 阶段 4.2 开发计划：AI 对话上下文

_创建日期：2026-03-28_

---

## 📋 目标

实现 AI 多轮对话能力，让 AI 能够记住之前的对话内容。

---

## 🎯 核心功能

### 1. 对话上下文存储

**需求**：
- 每个房间独立保存对话历史
- 每个用户独立的私聊历史
- 可配置的历史记录数量

**设计**：
```cpp
struct ConversationContext {
    std::string room_name;
    std::vector<Message> history;  // 最近 N 条消息
    int max_history = 10;          // 默认保留 10 轮
};
```

---

### 2. AI 上下文管理

**需求**：
- 发送消息时带上历史上下文
- 不同房间使用不同的上下文
- 支持清空上下文

**设计**：
```cpp
class ConversationManager {
    // 获取房间上下文
    ConversationContext get_context(const std::string& room_name);
    
    // 添加消息到上下文
    void add_message(const std::string& room_name, const Message& msg);
    
    // 清空上下文
    void clear_context(const std::string& room_name);
};
```

---

### 3. 命令扩展

**新增命令**：
```
/clear              - 清空当前房间对话历史
/context            - 查看当前上下文
/context <数量>      - 设置上下文大小
```

---

## 📁 需要新增/修改的文件

### 新增文件
```
src/conversation/
├── conversation.h       # 对话上下文结构
├── conversation.cpp     # 对话上下文实现
└── conversation_manager.h/cpp  # 上下文管理器
```

### 修改文件
```
src/server/connection.cpp    - 集成上下文管理
src/ai/ai_client.h/cpp       - 支持上下文参数
```

---

## 🔄 实现步骤

### 步骤 1：数据结构定义（预计 1 小时）
- [ ] 定义 `ConversationContext` 结构
- [ ] 定义 `ConversationManager` 类

### 步骤 2：上下文管理（预计 2 小时）
- [ ] 实现上下文存储（内存 + 数据库）
- [ ] 实现上下文获取/添加
- [ ] 实现上下文清空

### 步骤 3：AI 客户端集成（预计 2 小时）
- [ ] 修改 `AIClient::chat()` 支持上下文
- [ ] 构造带上下文的 AI 请求
- [ ] 解析 AI 响应

### 步骤 4：Connection 集成（预计 2 小时）
- [ ] 在 `handle_chat_message` 中添加上下文
- [ ] 实现 `/clear` 命令
- [ ] 实现 `/context` 命令

### 步骤 5：测试与优化（预计 1 小时）
- [ ] 测试多轮对话
- [ ] 测试上下文清空
- [ ] 性能优化

**预计总工时**：8 小时

---

## 🧪 测试用例

### 测试 1：基本多轮对话
```
用户：我叫小明
AI: 你好小明，很高兴认识你！

用户：我今年 25 岁
AI: 好的，小明今年 25 岁。

用户：我是做什么工作的？
AI: 抱歉，你还没有告诉我你的工作。
```

### 测试 2：房间隔离
```
房间 #general:
用户：我喜欢苹果

房间 #code:
用户：我喜欢苹果  （AI 不应该知道#general 的对话）
```

### 测试 3：上下文清空
```
用户：我叫小红
AI: 你好小红...

用户：/clear
AI: 对话历史已清空

用户：我叫什么名字？
AI: 抱歉，我不知道你的名字。
```

---

## 📊 验收标准

- [ ] AI 能记住最近 10 轮对话
- [ ] 不同房间对话隔离
- [ ] 支持清空对话历史
- [ ] 上下文大小可配置
- [ ] 性能无明显下降

---

## 🔮 后续扩展

### 阶段 4.3：消息搜索 + 历史
- 消息历史记录查询
- 关键词搜索
- 导出聊天记录

### 阶段 4.4：高级上下文
- 长期记忆（跨会话）
- 用户偏好学习
- 智能上下文摘要

---

_版本：v1.0_
_最后更新：2026-03-28_
