#pragma once

#include "message.h"
#include <nlohmann/json.hpp>
#include <string>

namespace chat {

using json = nlohmann::json;

class JsonParser {
public:
    static std::string stringify(const Message& msg);
    static Message parse(const std::string& json_str);
    static json to_json(const Message& msg);
    static Message from_json(const json& j);
};

} // namespace chat
