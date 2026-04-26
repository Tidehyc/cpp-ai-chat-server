#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace chat
{

    class RateLimiter
    {
    public:
        RateLimiter(std::size_t max_events, std::chrono::seconds window);

        // 返回 true 表示允许通过，false 表示触发限流。
        bool allow(const std::string &key);

    private:
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;

        std::size_t max_events_;
        std::chrono::seconds window_;
        std::unordered_map<std::string, std::deque<TimePoint>> events_;
        std::mutex mutex_;
    };

} // namespace chat