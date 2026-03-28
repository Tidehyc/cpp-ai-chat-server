#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>

namespace chat {

class Logger {
public:
    // 初始化日志系统
    static void init(const std::string& log_file = "chat_server.log", 
                     spdlog::level::level_enum level = spdlog::level::info);
    
    // 日志方法
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);
    static void debug(const std::string& msg);
    static void critical(const std::string& msg);
    
    // 设置日志级别
    static void set_level(spdlog::level::level_enum level);
    
    // 获取 logger 实例
    static std::shared_ptr<spdlog::logger> get_logger();

private:
    static std::shared_ptr<spdlog::logger> logger_;
};

} // namespace chat
