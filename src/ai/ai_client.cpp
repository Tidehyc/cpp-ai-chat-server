#include "ai_client.h"
#include "protocol/json_parser.h"
#include "utils/logger.h"
#include <curl/curl.h>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace chat
{

    struct ResponseBuffer
    {
        std::string data;
    };

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        size_t realsize = size * nmemb;
        auto *buf = static_cast<ResponseBuffer *>(userp);
        buf->data.append(static_cast<char *>(contents), realsize);
        return realsize;
    }

    namespace
    {

        std::string extract_error_detail(const std::string &payload)
        {
            if (payload.empty())
            {
                return "";
            }

            try
            {
                const auto parsed = json::parse(payload);

                if (parsed.contains("error"))
                {
                    const auto &error_field = parsed["error"];
                    if (error_field.is_string())
                    {
                        return error_field.get<std::string>();
                    }
                    if (error_field.is_object() && error_field.contains("message") && error_field["message"].is_string())
                    {
                        return error_field["message"].get<std::string>();
                    }
                }

                if (parsed.contains("message") && parsed["message"].is_string())
                {
                    return parsed["message"].get<std::string>();
                }
            }
            catch (const std::exception &)
            {
                // 非 JSON 错误体时，回退为简短原文
            }

            return payload.substr(0, 120);
        }

        std::string format_upstream_error(long status_code, const std::string &payload)
        {
            const std::string detail = extract_error_detail(payload);
            const std::string detail_suffix = detail.empty() ? "" : "（" + detail + "）";

            switch (status_code)
            {
            case 400:
                return "AI 请求参数无效，请联系管理员检查模型配置。" + detail_suffix;
            case 401:
            case 403:
                return "AI 服务鉴权失败，请检查 API Key 配置。" + detail_suffix;
            case 402:
                return "AI 服务额度不足，请充值或更换可用 API Key 后重试。" + detail_suffix;
            case 429:
                return "AI 服务请求过于频繁，请稍后重试。" + detail_suffix;
            default:
                if (status_code >= 500)
                {
                    return "AI 服务暂时不可用，请稍后重试。" + detail_suffix;
                }
                return "AI 服务错误，状态码：" + std::to_string(status_code) + detail_suffix;
            }
        }

        std::string perform_chat_request(const std::string &api_key,
                                         const std::string &api_url,
                                         const std::string &model,
                                         const std::string &message,
                                         const std::vector<AIMessage> &context)
        {
            if (api_key.empty() || api_key == "your_api_key_here")
            {
                return "🤖 AI 模拟回复：我收到了你的消息 \"" + message + "\"。请配置 API Key 以启用真实 AI 功能。";
            }

            CURL *curl = curl_easy_init();
            if (!curl)
            {
                Logger::error("Failed to initialize CURL");
                return "AI 服务暂时不可用";
            }

            json request_body = {
                {"model", model},
                {"messages", json::array()},
                {"stream", false},
                {"temperature", 0.7}};

            if (!context.empty() && !context[0].content.empty())
            {
                request_body["messages"].push_back({{"role", "system"},
                                                    {"content", context[0].content}});
            }

            for (size_t i = 0; i < context.size(); ++i)
            {
                if (i == 0 && !context.empty() && !context[0].content.empty())
                    continue;

                request_body["messages"].push_back({{"role", context[i].role},
                                                    {"content", context[i].content}});
            }

            request_body["messages"].push_back({{"role", "user"},
                                                {"content", message}});

            std::string body_str = request_body.dump();

            std::string auth_header = "Bearer " + api_key;
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, ("Authorization: " + auth_header).c_str());

            ResponseBuffer response_buf;

            curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void *>(&response_buf));
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

            Logger::info("Calling AI API with " + std::to_string(context.size()) + " context messages");
            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                Logger::error("CURL request failed: " + std::string(curl_easy_strerror(res)));
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                return "AI 服务请求失败：" + std::string(curl_easy_strerror(res));
            }

            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (response_code == 200)
            {
                try
                {
                    Logger::debug("AI response received, payload_bytes=" + std::to_string(response_buf.data.size()));
                    auto response = json::parse(response_buf.data);

                    if (response.contains("choices") && !response["choices"].empty())
                    {
                        std::string reply = response["choices"][0]["message"].value("content", "无回复内容");
                        Logger::info("AI response received (choices format)");
                        return reply;
                    }

                    Logger::error("Unexpected API response format");
                    return "AI 响应格式异常：" + response.dump(50);
                }
                catch (const std::exception &e)
                {
                    Logger::error("Failed to parse response: " + std::string(e.what()));
                    Logger::debug("AI parse failed, payload_bytes=" + std::to_string(response_buf.data.size()));
                    return "AI 响应解析失败：" + std::string(e.what());
                }
            }

            Logger::error("API error, status code: " + std::to_string(response_code));
            Logger::debug("API error payload bytes=" + std::to_string(response_buf.data.size()));
            return format_upstream_error(response_code, response_buf.data);
        }

    } // namespace

    AIClient::AIClient(const std::string &api_key, const std::string &api_url)
        : api_key_(api_key), api_url_(api_url)
    {
        static std::once_flag curl_once;
        std::call_once(curl_once, []()
                       {
                           curl_global_init(CURL_GLOBAL_ALL);
                           std::atexit([]()
                                       { curl_global_cleanup(); }); });
    }

    AIClient::~AIClient() = default;

    std::string AIClient::chat(const std::string &message, const std::vector<AIMessage> &context)
    {
        return perform_chat_request(api_key_, api_url_, model_, message, context);
    }

    void AIClient::set_model(const std::string &model)
    {
        model_ = model;
    }

    void AIClient::chat_async(const std::string &message,
                              const std::vector<AIMessage> &context,
                              AICallback callback)
    {
        const std::string api_key = api_key_;
        const std::string api_url = api_url_;
        const std::string model = model_;

        std::thread([api_key, api_url, model, message, context, callback]()
                    {
                        std::string reply = perform_chat_request(api_key, api_url, model, message, context);
                        callback(reply); })
            .detach();
    }

} // namespace chat
