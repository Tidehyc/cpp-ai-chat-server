#include "message.h"
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace chat
{

    using json = nlohmann::json;

    std::string message_type_to_string(MessageType type)
    {
        switch (type)
        {
        case MessageType::TEXT:
            return "text";
        case MessageType::JOIN:
            return "join";
        case MessageType::LEAVE:
            return "leave";
        case MessageType::ERROR:
            return "error";
        case MessageType::AI_REQUEST:
            return "ai_request";
        case MessageType::AI_RESPONSE:
            return "ai_response";
        case MessageType::SYSTEM:
            return "system";
        case MessageType::PING:
            return "ping";
        case MessageType::PONG:
            return "pong";
        default:
            return "text";
        }
    }

    MessageType message_type_from_string(const std::string &type_str)
    {
        static const std::unordered_map<std::string, MessageType> kTypeMap = {
            {"text", MessageType::TEXT},
            {"join", MessageType::JOIN},
            {"leave", MessageType::LEAVE},
            {"error", MessageType::ERROR},
            {"ai_request", MessageType::AI_REQUEST},
            {"ai_response", MessageType::AI_RESPONSE},
            {"system", MessageType::SYSTEM},
            {"ping", MessageType::PING},
            {"pong", MessageType::PONG}};

        auto it = kTypeMap.find(type_str);
        return it != kTypeMap.end() ? it->second : MessageType::TEXT;
    }

    std::string serialize_message(const Message &msg)
    {
        json j = {
            {"type", msg.type.empty() ? message_type_to_string(msg.msg_type) : msg.type},
            {"from", msg.from},
            {"to", msg.to},
            {"room", msg.room},
            {"content", msg.content},
            {"is_read", msg.is_read},
            {"timestamp", msg.timestamp}};
        return j.dump();
    }

    Message deserialize_message(const std::string &json_str)
    {
        Message msg;

        try
        {
            const auto j = json::parse(json_str);
            msg.type = j.value("type", "text");
            msg.msg_type = message_type_from_string(msg.type);
            msg.from = j.value("from", "");
            msg.to = j.value("to", "");
            msg.room = j.value("room", "");
            msg.content = j.value("content", "");
            msg.is_read = j.value("is_read", false);
            msg.timestamp = j.value("timestamp", static_cast<int64_t>(0));
        }
        catch (...)
        {
            // 返回默认消息，避免上层因非法报文崩溃。
            msg.type = "error";
            msg.msg_type = MessageType::ERROR;
            msg.content = "invalid json";
            msg.timestamp = 0;
        }

        return msg;
    }

} // namespace chat
