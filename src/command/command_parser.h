#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace chat {

// 命令解析结果
struct Command {
    std::string name;
    std::vector<std::string> args;
    std::string raw_args;
    bool is_valid() const { return !name.empty(); }
};

// 命令处理器
class CommandParser {
public:
    using Handler = std::function<void(const Command&)>;
    
    // 解析命令
    static Command parse(const std::string& input);
    
    // 检查是否是命令
    static bool is_command(const std::string& input);
    
    // 获取命令帮助
    static std::string get_help(const std::string& cmd_name = "");
    
    // 注册命令处理器
    static void register_handler(const std::string& cmd_name, Handler handler);
    
    // 执行命令
    static bool execute(const Command& cmd);
    
private:
    static std::map<std::string, Handler> handlers_;
};

} // namespace chat
