#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include "protocol/message.h"

namespace chat
{

    struct ModerationEvent
    {
        std::string actor;
        std::string target;
        std::string action;
        std::string reason;
        int64_t created_at = 0;
    };

    class Database
    {
    public:
        explicit Database(const std::string &db_path);
        ~Database();

        bool init();
        void save_message(const Message &msg);
        void save_moderation_event(const std::string &actor,
                                   const std::string &target,
                                   const std::string &action,
                                   const std::string &reason = "");
        std::vector<ModerationEvent> get_recent_moderation_events(int limit = 20);
        std::vector<Message> get_messages(const std::string &user, int limit = 50);
        void mark_messages_read(const std::string &from_user, const std::string &to_user);
        int get_unread_count(const std::string &user);

        // 获取 SQLite 数据库句柄
        sqlite3 *get_db() { return db_; }

    private:
        sqlite3 *db_;
        std::string db_path_;
    };

} // namespace chat
