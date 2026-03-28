#pragma once

#include <string>

namespace chat {

struct ApiConfig {
    std::string api_key;
    std::string api_url;
    std::string model;
    
    // 通义千问配置示例
    static ApiConfig default_qwen() {
        ApiConfig config;
        config.api_url = "https://dashscope.aliyuncs.com/api/v1";
        config.model = "qwen-turbo";
        // api_key 需要从环境变量或配置文件读取
        return config;
    }
};

} // namespace chat
