#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>

namespace chat
{

    // 命令执行器的函数签名
    // 接收命令名和参数列表
    using CommandExecutor = std::function<void(const std::string &cmd_name, const std::vector<std::string> &args)>;

    // 命令分发器：负责解析和路由命令
    class CommandDispatcher
    {
    public:
        explicit CommandDispatcher(CommandExecutor executor) : executor_(executor) {}

        // 处理输入：判断是否是命令，如果是则分发到 executor_
        // 返回是否成功处理（是否是有效命令）
        bool dispatch(const std::string &input);

        // 获取是否为命令的判断结果（用于下一步处理聊天消息）
        static bool is_command(const std::string &input);

        // 获取帮助信息
        static std::string get_help();

    private:
        // 从输入中提取命令名和参数
        static bool extract_command(
            const std::string &input,
            std::string &out_cmd_name,
            std::vector<std::string> &out_args);

        CommandExecutor executor_;
    };

} // namespace chat
