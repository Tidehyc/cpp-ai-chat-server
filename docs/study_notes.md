# 学习笔记

---

## 📚 阶段 1：WebSocket 服务器基础

### 核心知识点

#### 1. Boost.Asio 异步网络模型

**概念**：
Boost.Asio 是 C++ 的异步 IO 库，基于 Reactor 模式，支持高并发网络编程。

**核心组件**：
- `io_context` - IO 事件调度器，负责调度异步操作
- `tcp::acceptor` - TCP 连接接受器，用于监听和接受连接
- `tcp::socket` - TCP 套接字，用于数据传输
- `async_read`/`async_write` - 异步读写操作

**代码示例**：
```cpp
boost::asio::io_context io_context;
tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));

// 异步接受连接
void do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                // 处理新连接
            }
            do_accept();  // 继续接受下一个连接
        }
    );
}

// 运行事件循环
io_context.run();  // 阻塞直到所有操作完成
```

**关键点**：
- `io_context.run()` 需要有 pending 的异步操作才会阻塞
- 如果没有任何异步操作，`run()` 会立即返回

#### 2. WebSocket 协议

**握手流程**：
1. 客户端发送 HTTP Upgrade 请求
2. 服务器响应 101 Switching Protocols
3. 建立 WebSocket 连接，开始双向通信

**帧格式**：
- `0x1` - TEXT: 文本消息
- `0x2` - BINARY: 二进制消息
- `0x8` - CLOSE: 关闭连接
- `0x9` - PING: 心跳请求
- `0xA` - PONG: 心跳响应

#### 3. Sec-WebSocket-Accept 计算

**算法**：
```
Sec-WebSocket-Accept = base64(SHA1(Sec-WebSocket-Key + WS_GUID))
```

#### 4. CMake FetchContent

**概念**：
CMake 3.11+ 支持的依赖管理功能，可以自动下载和构建依赖库。

---

## 📚 阶段 2：AI 接入 + 消息存储

### 核心知识点

#### 1. 国家超算互联网 API

**API 端点**：
```
POST https://api.scnet.cn/api/llm/v1/chat/completions
```

**请求格式**（OpenAI 兼容）：
```json
{
    "model": "MiniMax-M2.5",
    "messages": [
        {"role": "user", "content": "你好"}
    ],
    "stream": false,
    "temperature": 0.7
}
```

**响应格式**：
```json
{
    "id": "chatcmpl-xxx",
    "choices": [
        {
            "message": {
                "role": "assistant",
                "content": "你好！我是 AI 助手..."
            }
        }
    ]
}
```

**鉴权方式**：
```
Authorization: Bearer YOUR_API_KEY
```

**可用模型**：
| 模型 | 说明 |
|------|------|
| MiniMax-M2.5 | MiniMax 最新模型 |
| DeepSeek-V3.2 | 深度求索 |
| Qwen3-30B-A3B | 通义千问 |
| Qwen3-235B-A22B | 通义千问大模型 |
| DeepSeek-R1-0528 | 深度求索推理 |

#### 2. libcurl HTTP 客户端

**基本用法**：
```cpp
CURL* curl = curl_easy_init();
if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.example.com");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buf);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}
```

#### 3. SQLite 数据库操作

**准备语句**：
```cpp
sqlite3_stmt* stmt = nullptr;
sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
```

**绑定参数**：
```cpp
sqlite3_bind_text(stmt, 1, msg.from.c_str(), -1, SQLITE_STATIC);
sqlite3_bind_int64(stmt, 5, msg.timestamp);
```

#### 4. nlohmann/json 使用

**构造 JSON**：
```cpp
json request_body = {
    {"model", model_},
    {"messages", {
        {{"role", "user"}, {"content", message}}
    }}
};
```

**解析 JSON**：
```cpp
auto response = json::parse(response_buf.data);
if (response.contains("choices")) {
    std::string reply = response["choices"][0]["message"]["content"];
}
```

### 技术对比

| HTTP 库 | 优点 | 缺点 |
|--------|------|------|
| libcurl | 功能强大，支持多协议 | API 较复杂 |
| cpp-httplib | 简单易用 | 功能较少 |
| Boost.Beast | 与 Boost.Asio 集成 | 学习曲线陡 |

### AI API 对比

| 提供商 | 模型 | 免费额度 | 价格 |
|--------|------|----------|------|
| 国家超算互联网 | MiniMax-M2.5, DeepSeek, Qwen3 | 1000 万 tokens/2 周 | 免费 |
| MiniMax 官方 | abab6.5 | ¥18/30 天 | ¥0.01/1K tokens |
| 通义千问 | qwen-turbo | 新用户赠送 | ¥0.002/1K tokens |

---

## 📚 阶段 2 完成总结

**完成时间**：2026-03-27

**实现功能**：
1. ✅ MiniMax AI API 接入（超算互联网）
2. ✅ 消息存储到 SQLite
3. ✅ @AI 触发 AI 回复
4. ✅ 模拟回复模式

**技术要点**：
- libcurl HTTP 客户端
- OpenAI 兼容 API 格式
- SQLite 数据库操作
- nlohmann/json 解析

---

_最后更新：2026-03-27_
