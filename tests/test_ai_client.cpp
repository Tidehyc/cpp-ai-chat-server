#include <gtest/gtest.h>

#include "ai/ai_client.h"

#include <chrono>
#include <future>
#include <memory>
#include <string>

using namespace chat;

TEST(AIClientTest, ChatAsyncSurvivesClientDestruction)
{
    auto promise = std::make_shared<std::promise<std::string>>();
    std::future<std::string> future = promise->get_future();

    {
        AIClient client("", "http://127.0.0.1:1");
        client.chat_async(
            "hello",
            {},
            [promise](const std::string &reply)
            {
                try
                {
                    promise->set_value(reply);
                }
                catch (...)
                {
                    promise->set_exception(std::current_exception());
                }
            });
    }

    ASSERT_EQ(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    const std::string reply = future.get();
    EXPECT_NE(reply.find("AI 模拟回复"), std::string::npos);
    EXPECT_NE(reply.find("hello"), std::string::npos);
}
