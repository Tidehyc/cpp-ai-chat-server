#include "database.h"
#include "utils/logger.h"
#include <sstream>
#include <iostream>

namespace chat
{

    namespace
    {

        bool exec_sql(sqlite3 *db, const char *sql, std::string *err = nullptr)
        {
            char *err_msg = nullptr;
            const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK)
            {
                if (err)
                {
                    *err = err_msg ? err_msg : "sqlite exec failed";
                }
                if (err_msg)
                {
                    sqlite3_free(err_msg);
                }
                return false;
            }
            return true;
        }

        bool column_exists(sqlite3 *db, const std::string &table_name, const std::string &column_name)
        {
            std::string sql = "PRAGMA table_info(" + table_name + ")";
            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
            {
                return false;
            }

            bool found = false;
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                if (name && column_name == name)
                {
                    found = true;
                    break;
                }
            }
            sqlite3_finalize(stmt);
            return found;
        }

        bool ensure_schema_meta(sqlite3 *db, std::string *err = nullptr)
        {
            const char *sql = R"(
        CREATE TABLE IF NOT EXISTS schema_meta (
            key TEXT PRIMARY KEY,
            value INTEGER NOT NULL
        )
    )";
            return exec_sql(db, sql, err);
        }

        int read_schema_version(sqlite3 *db, const char *key)
        {
            const char *select_sql = "SELECT value FROM schema_meta WHERE key = ?";
            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                return 0;
            }

            sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
            int version = 0;
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                version = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
            return version;
        }

        bool write_schema_version(sqlite3 *db, const char *key, int version, std::string *err = nullptr)
        {
            const char *sql = R"(
        INSERT INTO schema_meta (key, value)
        VALUES (?, ?)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value
    )";

            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                if (err)
                {
                    *err = sqlite3_errmsg(db);
                }
                return false;
            }

            sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, version);

            const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
            if (!ok && err)
            {
                *err = sqlite3_errmsg(db);
            }
            sqlite3_finalize(stmt);
            return ok;
        }

        bool migrate_core_schema(sqlite3 *db, std::string *err = nullptr)
        {
            if (!ensure_schema_meta(db, err))
            {
                return false;
            }

            int version = read_schema_version(db, "core_schema_version");

            if (version < 1)
            {
                const char *sql_v1 = R"(
            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                from_user TEXT NOT NULL,
                to_user TEXT NOT NULL,
                room_name TEXT,
                content TEXT NOT NULL,
                message_type TEXT DEFAULT 'text',
                is_read INTEGER DEFAULT 0,
                timestamp INTEGER NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_from_user ON messages(from_user);
            CREATE INDEX IF NOT EXISTS idx_to_user ON messages(to_user);
            CREATE INDEX IF NOT EXISTS idx_room_name ON messages(room_name);
            CREATE INDEX IF NOT EXISTS idx_timestamp ON messages(timestamp);
        )";
                if (!exec_sql(db, sql_v1, err) || !write_schema_version(db, "core_schema_version", 1, err))
                {
                    return false;
                }
                version = 1;
            }

            if (version < 2)
            {
                if (!column_exists(db, "messages", "is_read"))
                {
                    if (!exec_sql(db, "ALTER TABLE messages ADD COLUMN is_read INTEGER DEFAULT 0", err))
                    {
                        return false;
                    }
                }
                if (!write_schema_version(db, "core_schema_version", 2, err))
                {
                    return false;
                }
                version = 2;
            }

            if (version < 3)
            {
                const char *sql_v3 = R"(
            CREATE INDEX IF NOT EXISTS idx_room_name_timestamp ON messages(room_name, timestamp DESC);
            CREATE INDEX IF NOT EXISTS idx_from_user_timestamp ON messages(from_user, timestamp DESC);
            CREATE INDEX IF NOT EXISTS idx_to_user_timestamp ON messages(to_user, timestamp DESC);
            CREATE INDEX IF NOT EXISTS idx_to_user_is_read ON messages(to_user, is_read);
        )";
                if (!exec_sql(db, sql_v3, err) || !write_schema_version(db, "core_schema_version", 3, err))
                {
                    return false;
                }
            }

            if (version < 4)
            {
                const char *sql_v4 = R"(
            CREATE TABLE IF NOT EXISTS moderation_events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                actor TEXT NOT NULL,
                target TEXT NOT NULL,
                action TEXT NOT NULL,
                reason TEXT DEFAULT '',
                created_at INTEGER NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_moderation_events_actor ON moderation_events(actor, created_at DESC);
            CREATE INDEX IF NOT EXISTS idx_moderation_events_target ON moderation_events(target, created_at DESC);
            CREATE INDEX IF NOT EXISTS idx_moderation_events_action ON moderation_events(action, created_at DESC);
        )";
                if (!exec_sql(db, sql_v4, err) || !write_schema_version(db, "core_schema_version", 4, err))
                {
                    return false;
                }
            }

            return true;
        }

    } // namespace

    Database::Database(const std::string &db_path) : db_(nullptr), db_path_(db_path) {}

    Database::~Database()
    {
        if (db_)
        {
            sqlite3_close(db_);
        }
    }

    bool Database::init()
    {
        int rc = sqlite3_open(db_path_.c_str(), &db_);
        if (rc != SQLITE_OK)
        {
            std::cerr << "[Database] Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }

        std::string migration_error;
        if (!migrate_core_schema(db_, &migration_error))
        {
            std::cerr << "[Database] Migration error: " << migration_error << std::endl;
            return false;
        }

        Logger::info("Database initialized at " + db_path_);
        return true;
    }

    void Database::save_message(const Message &msg)
    {
        if (!db_)
            return;

        const char *sql = R"(
        INSERT INTO messages (from_user, to_user, room_name, content, message_type, is_read, timestamp)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
            return;
        }

        sqlite3_bind_text(stmt, 1, msg.from.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, msg.to.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, msg.room.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, msg.content.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, msg.type.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 6, msg.is_read ? 1 : 0);
        sqlite3_bind_int64(stmt, 7, msg.timestamp);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            Logger::error("Failed to save message: " + std::string(sqlite3_errmsg(db_)));
        }

        sqlite3_finalize(stmt);
    }

    void Database::save_moderation_event(const std::string &actor,
                                         const std::string &target,
                                         const std::string &action,
                                         const std::string &reason)
    {
        if (!db_)
            return;

        const char *sql = R"(
        INSERT INTO moderation_events (actor, target, action, reason, created_at)
        VALUES (?, ?, ?, ?, ?)
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare moderation event statement: " + std::string(sqlite3_errmsg(db_)));
            return;
        }

        const std::time_t now = std::time(nullptr);
        sqlite3_bind_text(stmt, 1, actor.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, target.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, action.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, reason.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(now));

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            Logger::error("Failed to save moderation event: " + std::string(sqlite3_errmsg(db_)));
        }

        sqlite3_finalize(stmt);
    }

    std::vector<ModerationEvent> Database::get_recent_moderation_events(int limit)
    {
        std::vector<ModerationEvent> events;

        if (!db_)
            return events;

        const char *sql = R"(
        SELECT actor, target, action, reason, created_at
        FROM moderation_events
        ORDER BY created_at DESC, id DESC
        LIMIT ?
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare moderation event query: " + std::string(sqlite3_errmsg(db_)));
            return events;
        }

        sqlite3_bind_int(stmt, 1, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ModerationEvent event;
            event.actor = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            event.target = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            event.action = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            event.reason = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            event.created_at = sqlite3_column_int64(stmt, 4);
            events.push_back(event);
        }

        sqlite3_finalize(stmt);
        return events;
    }

    std::vector<Message> Database::get_messages(const std::string &user, int limit)
    {
        std::vector<Message> messages;

        if (!db_)
            return messages;

        const char *sql = R"(
        SELECT from_user, to_user, content, message_type, is_read, timestamp
        FROM messages
        WHERE from_user = ? OR to_user = ?
        ORDER BY timestamp DESC
        LIMIT ?
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare statement");
            return messages;
        }

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, user.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            Message msg;
            msg.from = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            msg.to = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            msg.content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            msg.type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            msg.is_read = sqlite3_column_int(stmt, 4) != 0;
            msg.timestamp = sqlite3_column_int64(stmt, 5);
            messages.push_back(msg);
        }

        sqlite3_finalize(stmt);
        return messages;
    }

    void Database::mark_messages_read(const std::string &from_user, const std::string &to_user)
    {
        if (!db_)
            return;

        const char *sql = R"(
        UPDATE messages SET is_read = 1
        WHERE from_user = ? AND to_user = ? AND is_read = 0
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare mark read statement");
            return;
        }

        sqlite3_bind_text(stmt, 1, from_user.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, to_user.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            Logger::error("Failed to mark messages as read");
        }

        sqlite3_finalize(stmt);
        Logger::debug("Marked messages from " + from_user + " to " + to_user + " as read");
    }

    int Database::get_unread_count(const std::string &user)
    {
        if (!db_)
            return 0;

        const char *sql = R"(
        SELECT COUNT(*) FROM messages
        WHERE to_user = ? AND is_read = 0
    )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare unread count statement");
            return 0;
        }

        sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);

        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return count;
    }

} // namespace chat
