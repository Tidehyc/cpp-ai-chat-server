#pragma once

#include <string>
#include <cstdint>
#include <ctime>

namespace chat {

// 消息类型
enum class MessageType {
    TEXT,           // 文本消息
    JOIN,           // 加入聊天
    LEAVE,          // 离开聊天
    ERROR,          // 错误消息
    AI_REQUEST,     // AI 请求
    AI_RESPONSE,    // AI 响应
    SYSTEM,         // 系统消息
    PING,           // 心跳
    PONG            // 心跳响应
};

struct Message {
    std::string type;       // 消息类型字符串
    MessageType msg_type;   // 消息类型枚举
    std::string from;       // 发送者 ID
    std::string to;         // 接收者 ID 或 "ai"
    std::string room;       // 房间名
    std::string content;    // 消息内容
    int64_t timestamp;      // 时间戳

    Message() : msg_type(MessageType::TEXT), timestamp(0) {}
};

} // namespace chat
