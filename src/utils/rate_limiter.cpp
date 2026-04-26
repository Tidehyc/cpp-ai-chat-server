#include "rate_limiter.h"

namespace chat
{

    RateLimiter::RateLimiter(std::size_t max_events, std::chrono::seconds window)
        : max_events_(max_events), window_(window)
    {
    }

    bool RateLimiter::allow(const std::string &key)
    {
        const TimePoint now = Clock::now();
        const TimePoint cutoff = now - window_;

        std::lock_guard<std::mutex> lock(mutex_);
        auto &queue = events_[key];

        while (!queue.empty() && queue.front() < cutoff)
        {
            queue.pop_front();
        }

        if (queue.size() >= max_events_)
        {
            return false;
        }

        queue.push_back(now);
        return true;
    }

} // namespace chat