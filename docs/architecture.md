# 架构设计

## 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                     C++ Chat Server                      │
├─────────────────────────────────────────────────────────┤
│  核心模块：                                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │  Network    │  │   Protocol  │  │      AI         │  │
│  │  Layer      │  │   Layer     │  │     Client      │  │
│  │  (Asio)     │  │  (WebSocket)│  │   (HTTP/HTTPS)  │  │
│  └─────────────┘  └─────────────┘  └─────────────────┘  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │   Room      │  │   Message   │  │    Database     │  │
│  │  Manager    │  │   Router    │  │    (SQLite)     │  │
│  └─────────────┘  └─────────────┘  └─────────────────┘  │
│  ┌─────────────┐  ┌─────────────┐                        │
│  │   User      │  │  Preset     │                        │
│  │  Manager    │  │  Manager    │                        │
│  └─────────────┘  └─────────────┘                        │
└─────────────────────────────────────────────────────────┘
```

## 模块设计

### 1. Network Layer (网络层)

**职责**：
- TCP 连接管理
- WebSocket 握手
- 异步读写

**关键类**：
- `Server` - 服务器主类
- `Connection` - 单个连接

### 2. Protocol Layer (协议层)

**职责**：
- WebSocket 帧解析
- 消息编码/解码
- 命令解析

**关键类**：
- `WebSocketParser` - WebSocket 解析器
- `Message` - 消息结构
- `CommandParser` - 命令解析器（新增）

### 3. AI Client (AI 客户端)

**职责**：
- HTTP 请求构造
- AI API 调用
- 响应解析

**关键类**：
- `AIClient` - AI 客户端
- `AIPrompt` - AI 提示词管理（新增）

### 4. Room Manager (房间管理器) - 新增

**职责**：
- 房间创建/删除
- 用户加入/离开
- 房间 AI 配置管理

**关键类**：
- `Room` - 房间类
- `RoomManager` - 房间管理器

### 5. User Manager (用户管理器) - 新增

**职责**：
- 用户注册/登录
- 用户状态管理
- 用户预设管理

**关键类**：
- `User` - 用户类
- `UserManager` - 用户管理器
- `Preset` - 预设类

### 6. Storage Layer (存储层)

**职责**：
- 用户数据存储
- 消息历史记录
- 对话上下文存储

**关键类**：
- `Database` - 数据库操作

---

## 数据流

### 房间消息流

```
Client → WebSocket → CommandParser → RoomManager
                                          │
                                          ▼
                                    获取房间 AI 配置
                                          │
                                          ▼
                                    AIClient (带提示词)
                                          │
                                          ▼
                                    AI 响应 → Client
```

### 私聊消息流

```
Client → WebSocket → CommandParser → UserManager
                                          │
                                          ▼
                                    获取用户预设
                                          │
                                          ▼
                                    AIClient (带预设提示词)
                                          │
                                          ▼
                                    AI 响应 → Client
```

---

## 核心类设计

### Room 类

```cpp
class Room {
public:
    int id;
    std::string name;
    std::string ai_prompt;      // AI 系统提示词
    double ai_temperature;      // AI 温度
    int max_context;            // 最大上下文轮数
    std::set<int> member_ids;   // 成员 ID 列表
    
    void join(int user_id);
    void leave(int user_id);
    std::string get_ai_prompt() const;
};
```

### User 类

```cpp
class User {
public:
    int id;
    std::string username;
    std::string password_hash;
    std::string current_preset;  // 当前使用的预设
    std::string status;          // 在线状态
    
    std::string get_ai_prompt() const;
    void set_preset(const std::string& preset_name);
};
```

### Preset 类

```cpp
class Preset {
public:
    std::string name;
    std::string prompt;
    bool is_builtin;
    
    static std::vector<Preset> get_builtin_presets();
};
```

---

## 命令系统设计

### 命令格式

```
/命令名 参数 1 参数 2 ...
```

### 命令列表

```cpp
// 房间相关
"/rooms"        // 列出所有房间
"/join"         // 加入房间
"/leave"        // 离开房间
"/create"       // 创建房间

// 私聊 AI 相关
"/msg"          // 发送消息
"/preset"       // 预设管理
"/ai"           // 私聊 AI

// 用户相关
"/register"     // 注册
"/login"        // 登录
"/status"       // 设置状态
"/online"       // 查看在线用户

// 系统相关
"/help"         // 帮助
"/ping"         // 测试连接
"/quit"         // 退出
```

---

## 数据库设计

### ER 图

```
┌─────────────┐       ┌─────────────┐
│    users    │       │    rooms    │
├─────────────┤       ├─────────────┤
│ id (PK)     │       │ id (PK)     │
│ username    │       │ name        │
│ password    │       │ ai_prompt   │
│ preset      │       │ temperature │
│ status      │       └──────┬──────┘
└──────┬──────┘              │
       │                     │
       │  ┌─────────────┐    │
       └──│  messages   │────┘
          ├─────────────┤
          │ id (PK)     │
          │ room_id     │
          │ from_user   │
          │ to_user     │
          │ content     │
          │ timestamp   │
          └─────────────┘
```

---

_最后更新：2026-03-27_
