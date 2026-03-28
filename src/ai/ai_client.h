#pragma once

#include <string>
#include <vector>
#include <functional>

namespace chat {

// 对话消息（用于上下文）
struct AIMessage {
    std::string role;    // "user" 或 "assistant"
    std::string content;
};

// AI 响应回调
using AICallback = std::function<void(const std::string& reply)>;

class AIClient {
public:
    explicit AIClient(const std::string& api_key, const std::string& api_url);
    ~AIClient();

    // 同步聊天（阻塞）
    std::string chat(const std::string& message, 
                     const std::vector<AIMessage>& context = {});
    
    // 异步聊天（非阻塞）
    void chat_async(const std::string& message,
                    const std::vector<AIMessage>& context,
                    AICallback callback);
    
    void set_model(const std::string& model);

private:
    std::string api_key_;
    std::string api_url_;
    std::string model_ = "MiniMax-M2.5";
};

} // namespace chat
