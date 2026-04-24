#include "command_dispatcher.h"
#include "utils/logger.h"
#include <sstream>

namespace chat
{

    bool CommandDispatcher::is_command(const std::string &input)
    {
        return !input.empty() && input[0] == '/';
    }

    bool CommandDispatcher::extract_command(
        const std::string &input,
        std::string &out_cmd_name,
        std::vector<std::string> &out_args)
    {
        if (!is_command(input))
        {
            return false;
        }

        // 去掉前导 '/'
        std::string cmd_input = input.substr(1);

        // 分割命令名和参数
        size_t space_pos = cmd_input.find(' ');
        if (space_pos == std::string::npos)
        {
            // 没有参数
            out_cmd_name = cmd_input;
            out_args.clear();
        }
        else
        {
            // 有参数
            out_cmd_name = cmd_input.substr(0, space_pos);
            std::string args_str = cmd_input.substr(space_pos + 1);

            // 分割参数
            std::istringstream iss(args_str);
            std::string arg;
            while (iss >> arg)
            {
                out_args.push_back(arg);
            }
        }

        return true;
    }

    bool CommandDispatcher::dispatch(const std::string &input)
    {
        std::string cmd_name;
        std::vector<std::string> args;

        if (!extract_command(input, cmd_name, args))
        {
            return false; // 不是命令
        }

        // 调用 executor（由 Connection 提供的回调）
        if (executor_)
        {
            executor_(cmd_name, args);
        }

        return true; // 成功分发
    }

    std::string CommandDispatcher::get_help()
    {
        return "可用命令:\n"
               "  /register <user> <pass>  - 注册账户\n"
               "  /login <user> <pass>     - 登录\n"
               "  /rooms                   - 列出所有房间\n"
               "  /join <room>             - 加入房间\n"
               "  /leave                   - 离开房间\n"
               "  /msg ai <消息>           - 私聊 AI\n"
               "  /preset list             - 列出预设\n"
               "  /preset use <name>       - 使用预设\n"
               "  /history [room|keyword]  - 查看历史\n"
               "  /search <keyword>        - 搜索消息\n"
               "  /friend <action> [args]  - 管理好友\n"
               "  /pm <user> <消息>        - 私聊用户\n"
               "  /help                    - 显示帮助\n"
               "  /quit                    - 退出";
    }

} // namespace chat
