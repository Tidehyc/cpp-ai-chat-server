#include <gtest/gtest.h>
#include <thread>
#include "utils/rate_limiter.h"

TEST(RateLimiterTest, BlocksWhenWindowLimitReached)
{
    chat::RateLimiter limiter(2, std::chrono::seconds(5));

    EXPECT_TRUE(limiter.allow("ip:127.0.0.1"));
    EXPECT_TRUE(limiter.allow("ip:127.0.0.1"));
    EXPECT_FALSE(limiter.allow("ip:127.0.0.1"));
}

TEST(RateLimiterTest, AllowsAgainAfterWindowExpires)
{
    chat::RateLimiter limiter(1, std::chrono::seconds(1));

    EXPECT_TRUE(limiter.allow("user:alice"));
    EXPECT_FALSE(limiter.allow("user:alice"));

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_TRUE(limiter.allow("user:alice"));
}
