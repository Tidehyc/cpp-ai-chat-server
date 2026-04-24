#include "message_history.h"
#include "storage/database.h"
#include "utils/logger.h"
#include <sstream>
#include <algorithm>

namespace chat
{

    MessageHistoryManager::MessageHistoryManager(Database *db) : db_(db)
    {
        // 消息表已存在，无需创建
    }

    std::vector<MessageRecord> MessageHistoryManager::get_history(
        const std::string &target,
        bool is_room,
        int limit)
    {
        std::vector<MessageRecord> messages;

        std::string sql;

        // 特殊处理：查询与 AI 的对话
        if (target == "@ai" || target == "@AI" || target == "ai" || target == "AI")
        {
            sql = R"(
            SELECT id, from_user, to_user, room_name, content, message_type, timestamp
            FROM messages
            WHERE from_user = 'AI' OR to_user = 'AI'
            ORDER BY timestamp DESC
            LIMIT ?
        )";

            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db_->get_db(), sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmt, 1, limit);

                while (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    MessageRecord msg;
                    msg.id = sqlite3_column_int64(stmt, 0);
                    msg.from_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                    msg.to_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                    msg.room_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                    msg.content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
                    msg.message_type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
                    msg.timestamp = sqlite3_column_int64(stmt, 6);
                    messages.push_back(msg);
                }
                sqlite3_finalize(stmt);
            }
        }
        else if (is_room)
        {
            // 去掉 # 前缀
            std::string room = target;
            if (room[0] == '#')
            {
                room = room.substr(1);
            }

            sql = R"(
            SELECT id, from_user, to_user, room_name, content, message_type, timestamp
            FROM messages
            WHERE room_name = ?
            ORDER BY timestamp DESC
            LIMIT ?
        )";

            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db_->get_db(), sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_text(stmt, 1, room.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 2, limit);

                while (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    MessageRecord msg;
                    msg.id = sqlite3_column_int64(stmt, 0);
                    msg.from_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                    msg.to_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                    msg.room_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                    msg.content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
                    msg.message_type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
                    msg.timestamp = sqlite3_column_int64(stmt, 6);
                    messages.push_back(msg);
                }
                sqlite3_finalize(stmt);
            }
        }
        else
        {
            // 查询与特定用户的私聊
            sql = R"(
            SELECT id, from_user, to_user, room_name, content, message_type, timestamp
            FROM messages
            WHERE (from_user = ? OR to_user = ?) AND (from_user != 'AI' AND to_user != 'AI')
            ORDER BY timestamp DESC
            LIMIT ?
        )";

            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db_->get_db(), sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_text(stmt, 1, target.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, target.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 3, limit);

                while (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    MessageRecord msg;
                    msg.id = sqlite3_column_int64(stmt, 0);
                    msg.from_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                    msg.to_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                    msg.room_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                    msg.content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
                    msg.message_type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
                    msg.timestamp = sqlite3_column_int64(stmt, 6);
                    messages.push_back(msg);
                }
                sqlite3_finalize(stmt);
            }
        }

        // 反转顺序（从旧到新）
        std::reverse(messages.begin(), messages.end());
        return messages;
    }

    std::vector<MessageRecord> MessageHistoryManager::search(const SearchCriteria &criteria)
    {
        std::vector<MessageRecord> messages;

        std::string sql = R"(
        SELECT id, from_user, to_user, room_name, content, message_type, timestamp
        FROM messages
        WHERE 1=1
    )";

        if (criteria.has_keyword())
        {
            sql += " AND content LIKE ?";
        }
        if (criteria.has_from_user())
        {
            sql += " AND from_user = ?";
        }
        if (criteria.has_room())
        {
            sql += " AND room_name = ?";
        }
        if (criteria.start_time > 0)
        {
            sql += " AND timestamp >= ?";
        }
        if (criteria.end_time > 0)
        {
            sql += " AND timestamp <= ?";
        }

        sql += " ORDER BY timestamp DESC LIMIT ?";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
        {
            int bind_index = 1;
            if (criteria.has_keyword())
            {
                std::string keyword_pattern = "%" + criteria.keyword + "%";
                sqlite3_bind_text(stmt, bind_index++, keyword_pattern.c_str(), -1, SQLITE_TRANSIENT);
            }
            if (criteria.has_from_user())
            {
                sqlite3_bind_text(stmt, bind_index++, criteria.from_user.c_str(), -1, SQLITE_TRANSIENT);
            }
            if (criteria.has_room())
            {
                sqlite3_bind_text(stmt, bind_index++, criteria.room_name.c_str(), -1, SQLITE_TRANSIENT);
            }
            if (criteria.start_time > 0)
            {
                sqlite3_bind_int64(stmt, bind_index++, criteria.start_time);
            }
            if (criteria.end_time > 0)
            {
                sqlite3_bind_int64(stmt, bind_index++, criteria.end_time);
            }

            const int safe_limit = criteria.limit > 0 ? criteria.limit : 20;
            sqlite3_bind_int(stmt, bind_index, safe_limit);

            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                MessageRecord msg;
                msg.id = sqlite3_column_int64(stmt, 0);
                msg.from_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                msg.to_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                msg.room_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                msg.content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
                msg.message_type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
                msg.timestamp = sqlite3_column_int64(stmt, 6);
                messages.push_back(msg);
            }
            sqlite3_finalize(stmt);
        }

        return messages;
    }

    std::string MessageHistoryManager::export_messages(
        const std::string &target,
        bool is_room,
        const std::string &format)
    {
        auto messages = get_history(target, is_room, 1000); // 最多导出 1000 条

        if (format == "json")
        {
            std::stringstream ss;
            ss << "{\n  \"target\": \"" << target << "\",\n";
            ss << "  \"is_room\": " << (is_room ? "true" : "false") << ",\n";
            ss << "  \"messages\": [\n";

            for (size_t i = 0; i < messages.size(); ++i)
            {
                const auto &msg = messages[i];
                ss << "    {\n";
                ss << "      \"from\": \"" << msg.from_user << "\",\n";
                ss << "      \"to\": \"" << msg.to_user << "\",\n";
                ss << "      \"content\": \"" << msg.content << "\",\n";
                ss << "      \"timestamp\": " << msg.timestamp << "\n";
                ss << "    }";
                if (i < messages.size() - 1)
                    ss << ",";
                ss << "\n";
            }

            ss << "  ]\n}";
            return ss.str();
        }
        else
        {
            // 文本格式
            std::stringstream ss;
            ss << "聊天记录导出\n";
            ss << "目标：" << target << (is_room ? " (房间)" : " (用户)") << "\n";
            ss << "时间：" << std::time(nullptr) << "\n";
            ss << "====================\n\n";

            for (const auto &msg : messages)
            {
                char time_buf[64];
                std::time_t t = msg.timestamp;
                std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

                ss << "[" << time_buf << "] ";
                if (!msg.room_name.empty())
                {
                    ss << "#" << msg.room_name << " ";
                }
                ss << msg.from_user << ": " << msg.content << "\n";
            }

            return ss.str();
        }
    }

} // namespace chat
