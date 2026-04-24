#pragma once

#include <cstdint>
#include <functional>
#include <string>

struct sqlite3;

namespace chat
{

    struct OfflineMessageRow
    {
        int id = 0;
        std::string from_user;
        std::string content;
        int64_t timestamp = 0;
    };

    // 返回 true 表示本次离线消息推送与已读更新整体成功提交。
    // 返回 false 表示过程中发生错误，数据库变更已回滚。
    bool deliver_offline_messages_transactional(
        sqlite3 *db,
        const std::string &to_user,
        const std::function<bool(const OfflineMessageRow &)> &sink,
        int *delivered_count = nullptr,
        std::string *error = nullptr);

} // namespace chat
