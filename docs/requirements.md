# 需求文档

## 📋 功能需求

### 阶段 1：WebSocket 服务器基础 ✅

| 功能 | 描述 | 状态 |
|------|------|------|
| WebSocket 连接 | 支持客户端 WebSocket 连接 | ✅ 完成 |
| 消息收发 | 实时消息发送和接收 | ✅ 完成 |
| 连接管理 | 多连接并发处理 | ✅ 完成 |

### 阶段 2：AI 接入 + 消息存储 ✅

| 功能 | 描述 | 状态 |
|------|------|------|
| AI 对话 | 调用 AI API 回复消息 | ✅ 完成 |
| 消息存储 | 聊天记录持久化 | ✅ 完成 |
| 模拟模式 | 无 API Key 时返回模拟回复 | ✅ 完成 |

### 阶段 3：房间系统 + 私聊 AI 🟡

**目标**：房间决定 AI 功能，私聊自定义 AI 设定

#### 核心功能

| 功能 | 描述 | 优先级 |
|------|------|--------|
| 用户系统 | 注册/登录/状态管理 | P0 |
| 房间系统 | 创建/加入/离开房间 | P0 |
| 房间 AI 配置 | 每个房间不同 AI 提示词 | P0 |
| 私聊 AI | 基础私聊功能 | P0 |
| 预设系统 | 创建/使用 AI 预设 | P1 |
| 对话上下文 | 每个房间独立记忆 | P1 |
| 命令解析器 | /join, /preset 等命令 | P2 |
| 消息历史 | 查看聊天记录 | P2 |

#### 预设房间（内置）

| 房间名 | AI 系统提示词 | 功能 |
|--------|-------------|------|
| `#general` | "你是友好的 AI 助手" | 通用对话 |
| `#code` | "你是资深程序员，擅长代码编写、Debug、代码审查" | 编程助手 |
| `#translate` | "你是专业翻译，只输出翻译结果，不解释" | 多语言互译 |
| `#learn` | "你是耐心的老师，用通俗易懂的方式讲解知识" | 知识讲解 |
| `#write` | "你是专业作家，擅长各种文体写作" | 写作助手 |
| `#debug` | "你是 Debug 专家，擅长定位和解决问题" | 问题诊断 |

#### 预设设定（内置）

| 预设名 | AI 系统提示词 | 场景 |
|--------|-------------|------|
| `teacher` | "你是耐心的老师，通俗易懂" | 学习 |
| `coder` | "你是资深程序员，代码简洁高效" | 编程 |
| `friend` | "你是知心朋友，善于倾听" | 聊天 |
| `interviewer` | "你是面试官，会提问和评估" | 面试练习 |

---

## 🔧 非功能需求

| 需求 | 目标 |
|------|------|
| 并发连接 | 支持 1000+ 并发 |
| 响应延迟 | < 100ms (不含 AI) |
| AI 响应 | < 5s (依赖 API) |
| 可用性 | 99.9% |

---

## 📊 系统架构

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│   Client    │────▶│  C++ Server  │────▶│  AI API     │
│  WebSocket  │     │  (Boost.Asio)│     │  (HTTP)     │
└─────────────┘     └──────────────┘     └─────────────┘
                           │
                           ▼
                    ┌──────────────┐
                    │   SQLite     │
                    │  (存储)      │
                    └──────────────┘
```

### 模块设计

```
┌─────────────────────────────────────────────────────────┐
│                     C++ Chat Server                      │
├─────────────────────────────────────────────────────────┤
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

---

## 📁 数据模型

### 表结构

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
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- 消息表
CREATE TABLE messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER,
    from_user_id INTEGER NOT NULL,
    to_user_id INTEGER,
    content TEXT NOT NULL,
    message_type TEXT DEFAULT 'text',
    ai_response_to INTEGER,
    timestamp INTEGER NOT NULL,
    FOREIGN KEY (room_id) REFERENCES rooms(id),
    FOREIGN KEY (from_user_id) REFERENCES users(id),
    FOREIGN KEY (to_user_id) REFERENCES users(id)
);

-- 对话上下文表
CREATE TABLE conversations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    room_id INTEGER,
    context_json TEXT,
    updated_at INTEGER NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (room_id) REFERENCES rooms(id)
);
```

---

## 📅 开发阶段规划

### 阶段 1：WebSocket 服务器基础 ✅

**测试结果**：
- ✅ 服务器成功启动并监听端口 8080
- ✅ WebSocket 握手成功
- ✅ 消息收发正常
- ✅ 连接正常关闭

### 阶段 2：AI 接入 + 消息存储 ✅

**测试结果**：
- ✅ AI API 调用成功（超算互联网 MiniMax-M2.5）
- ✅ AI 回复正常
- ✅ 消息存储到数据库
- ✅ 模拟回复模式工作正常

### 阶段 3：房间系统 + 私聊 AI 🟡

**任务列表**：
- [ ] 用户系统（注册/登录）
- [ ] 房间管理（创建/加入/离开）
- [ ] 房间 AI 配置
- [ ] 私聊 AI 功能
- [ ] 预设系统
- [ ] 对话上下文管理
- [ ] 命令解析器

**预计完成时间**：2026-03-28

---

_最后更新：2026-03-27_
