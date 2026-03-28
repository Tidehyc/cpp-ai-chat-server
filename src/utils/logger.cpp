#include "logger.h"
#include <spdlog/fmt/fmt.h>

namespace chat {

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::init(const std::string& log_file, spdlog::level::level_enum level) {
    // 创建控制台输出
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);
    
    // 创建文件输出（支持轮转）
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file,
        1024 * 1024 * 5,  // 5MB 轮转
        3                 // 保留 3 个文件
    );
    file_sink->set_level(level);
    
    // 创建 logger
    logger_ = std::make_shared<spdlog::logger>("chat_server", spdlog::sinks_init_list{console_sink, file_sink});
    
    // 设置格式
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    
    // 设置级别
    logger_->set_level(level);
    
    // 注册到 spdlog
    spdlog::register_logger(logger_);
}

void Logger::info(const std::string& msg) {
    if (logger_) {
        logger_->info(msg);
    }
}

void Logger::warn(const std::string& msg) {
    if (logger_) {
        logger_->warn(msg);
    }
}

void Logger::error(const std::string& msg) {
    if (logger_) {
        logger_->error(msg);
    }
}

void Logger::debug(const std::string& msg) {
    if (logger_) {
        logger_->debug(msg);
    }
}

void Logger::critical(const std::string& msg) {
    if (logger_) {
        logger_->critical(msg);
    }
}

void Logger::set_level(spdlog::level::level_enum level) {
    if (logger_) {
        logger_->set_level(level);
    }
}

std::shared_ptr<spdlog::logger> Logger::get_logger() {
    return logger_;
}

} // namespace chat
