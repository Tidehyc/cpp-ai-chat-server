#include "json_parser.h"
#include <chrono>

namespace chat {

json JsonParser::to_json(const Message& msg) {
    return json{
        {"type", msg.type},
        {"from", msg.from},
        {"to", msg.to},
        {"content", msg.content},
        {"timestamp", msg.timestamp}
    };
}

Message JsonParser::from_json(const json& j) {
    Message msg;
    msg.type = j.value("type", "text");
    msg.from = j.value("from", "");
    msg.to = j.value("to", "");
    msg.content = j.value("content", "");
    msg.timestamp = j.value("timestamp", 0);
    return msg;
}

std::string JsonParser::stringify(const Message& msg) {
    try {
        // 先清理 content 中的控制字符
        std::string safe_content;
        for (unsigned char c : msg.content) {
            if (c >= 32 && c < 127) {
                safe_content += c;
            } else if (c >= 0x80) {
                safe_content += c;
            } else if (c == '\n' || c == '\r' || c == '\t') {
                safe_content += ' ';  // 替换为空格
            }
            // 其他控制字符忽略
        }
        
        Message safe_msg = msg;
        safe_msg.content = safe_content;
        return to_json(safe_msg).dump();
    } catch (const std::exception& e) {
        // 如果序列化失败，返回一个简单的错误消息
        Message error_msg;
        error_msg.type = "system";
        error_msg.content = "消息序列化失败";
        return to_json(error_msg).dump();
    }
}

Message JsonParser::parse(const std::string& json_str) {
    auto j = json::parse(json_str);
    return from_json(j);
}

} // namespace chat
