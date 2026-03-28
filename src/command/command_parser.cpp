#include "command_parser.h"
#include "utils/logger.h"
#include <sstream>

namespace chat {

std::map<std::string, CommandParser::Handler> CommandParser::handlers_;

bool CommandParser::is_command(const std::string& input) {
    return !input.empty() && input[0] == '/';
}

Command CommandParser::parse(const std::string& input) {
    Command cmd;
    
    if (!is_command(input)) {
        return cmd;
    }
    
    std::istringstream iss(input.substr(1));  // 跳过 '/'
    std::string token;
    
    // 读取命令名
    if (iss >> token) {
        cmd.name = token;
    }
    
    // 读取剩余参数
    std::getline(iss, cmd.raw_args);
    if (!cmd.raw_args.empty() && cmd.raw_args[0] == ' ') {
        cmd.raw_args = cmd.raw_args.substr(1);
    }
    
    // 解析参数
    std::istringstream arg_ss(cmd.raw_args);
    std::string arg;
    while (arg_ss >> arg) {
        cmd.args.push_back(arg);
    }
    
    return cmd;
}

void CommandParser::register_handler(const std::string& cmd_name, Handler handler) {
    handlers_[cmd_name] = handler;
}

bool CommandParser::execute(const Command& cmd) {
    if (!cmd.is_valid()) {
        return false;
    }
    
    auto it = handlers_.find(cmd.name);
    if (it != handlers_.end()) {
        it->second(cmd);
        return true;
    }
    
    Logger::error("Unknown command: " + cmd.name);
    return false;
}

std::string CommandParser::get_help(const std::string& cmd_name) {
    if (!cmd_name.empty()) {
        // 返回特定命令的帮助
        if (handlers_.find(cmd_name) != handlers_.end()) {
            return "Command: /" + cmd_name;
        }
        return "Unknown command: " + cmd_name;
    }
    
    // 返回所有命令列表
    std::stringstream ss;
    ss << "可用命令:\n";
    ss << "  /rooms              - 列出所有房间\n";
    ss << "  /join <room>        - 加入房间\n";
    ss << "  /leave <room>       - 离开房间\n";
    ss << "  /preset list        - 列出预设\n";
    ss << "  /preset use <name>  - 使用预设\n";
    ss << "  /msg ai <消息>      - 私聊 AI\n";
    ss << "  /online             - 查看在线用户\n";
    ss << "  /help               - 显示帮助\n";
    ss << "  /quit               - 退出";
    
    return ss.str();
}

} // namespace chat
