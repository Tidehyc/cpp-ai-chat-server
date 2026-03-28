#include "ai_client.h"
#include "protocol/json_parser.h"
#include "utils/logger.h"
#include <curl/curl.h>
#include <chrono>
#include <thread>

namespace chat {

// CURL 响应数据缓冲
struct ResponseBuffer {
    std::string data;
};

// CURL 写回调函数
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto* buf = static_cast<ResponseBuffer*>(userp);
    buf->data.append(static_cast<char*>(contents), realsize);
    return realsize;
}

AIClient::AIClient(const std::string& api_key, const std::string& api_url)
    : api_key_(api_key), api_url_(api_url) {
    curl_global_init(CURL_GLOBAL_ALL);
}

AIClient::~AIClient() {
    curl_global_cleanup();
}

std::string AIClient::chat(const std::string& message, const std::vector<AIMessage>& context) {
    // 如果没有 API Key，返回模拟回复
    if (api_key_.empty() || api_key_ == "your_api_key_here") {
        return "🤖 AI 模拟回复：我收到了你的消息 \"" + message + "\"。请配置 API Key 以启用真实 AI 功能。";
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::error("Failed to initialize CURL");
        return "AI 服务暂时不可用";
    }

    // 构造请求体（带上下文）
    json request_body = {
        {"model", model_},
        {"messages", json::array()},
        {"stream", false},
        {"temperature", 0.7}
    };

    // 添加上下文消息
    for (const auto& msg : context) {
        request_body["messages"].push_back({
            {"role", msg.role},
            {"content", msg.content}
        });
    }

    // 添加当前消息
    request_body["messages"].push_back({
        {"role", "user"},
        {"content", message}
    });

    std::string body_str = request_body.dump();

    // 设置请求头
    std::string auth_header = "Bearer " + api_key_;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: " + auth_header).c_str());

    // 响应缓冲
    ResponseBuffer response_buf;

    // 配置 CURL
    curl_easy_setopt(curl, CURLOPT_URL, api_url_.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&response_buf));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    
    // SSL 配置
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // 跳过证书验证（测试用）
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // 执行请求
    Logger::info("Calling AI API with " + std::to_string(context.size()) + " context messages");
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        Logger::error("CURL request failed: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return "AI 服务请求失败：" + std::string(curl_easy_strerror(res));
    }

    // 获取 HTTP 响应码
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // 解析响应
    if (response_code == 200) {
        try {
            Logger::info("Raw response: " + response_buf.data.substr(0, 300));
            auto response = json::parse(response_buf.data);
            
            // OpenAI/超算格式：{"choices": [{"message": {"content": "..."}}]}
            if (response.contains("choices") && !response["choices"].empty()) {
                std::string reply = response["choices"][0]["message"].value("content", "无回复内容");
                Logger::info("AI response received (choices format)");
                return reply;
            }
            
            Logger::error("Unexpected API response format: " + response.dump(100));
            return "AI 响应格式异常：" + response.dump(50);
        } catch (const std::exception& e) {
            Logger::error("Failed to parse response: " + std::string(e.what()));
            Logger::error("Raw data: " + response_buf.data.substr(0, 300));
            return "AI 响应解析失败：" + std::string(e.what());
        }
    } else {
        Logger::error("API error, status code: " + std::to_string(response_code));
        Logger::error("Response body: " + response_buf.data.substr(0, 300));
        return "AI 服务错误，状态码：" + std::to_string(response_code) + " - " + response_buf.data.substr(0, 200);
    }
}

void AIClient::set_model(const std::string& model) {
    model_ = model;
}

void AIClient::chat_async(const std::string& message,
                          const std::vector<AIMessage>& context,
                          AICallback callback) {
    // 在新线程中执行 AI 调用
    std::thread([this, message, context, callback]() {
        std::string reply = chat(message, context);
        callback(reply);
    }).detach();
}

} // namespace chat
