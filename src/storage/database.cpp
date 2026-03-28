#include "database.h"
#include "utils/logger.h"
#include <sstream>
#include <iostream>

namespace chat {

Database::Database(const std::string& db_path) : db_(nullptr), db_path_(db_path) {}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::init() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    // 创建消息表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_user TEXT NOT NULL,
            to_user TEXT NOT NULL,
            room_name TEXT,
            content TEXT NOT NULL,
            message_type TEXT DEFAULT 'text',
            timestamp INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_from_user ON messages(from_user);
        CREATE INDEX IF NOT EXISTS idx_to_user ON messages(to_user);
        CREATE INDEX IF NOT EXISTS idx_room_name ON messages(room_name);
        CREATE INDEX IF NOT EXISTS idx_timestamp ON messages(timestamp)
    )";

    char* err_msg = nullptr;
    rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    Logger::info("Database initialized at " + db_path_);
    return true;
}

void Database::save_message(const Message& msg) {
    if (!db_) return;

    const char* sql = R"(
        INSERT INTO messages (from_user, to_user, room_name, content, message_type, timestamp)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        return;
    }

    sqlite3_bind_text(stmt, 1, msg.from.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, msg.to.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, msg.room.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, msg.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, msg.type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 6, msg.timestamp);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Logger::error("Failed to save message: " + std::string(sqlite3_errmsg(db_)));
    }

    sqlite3_finalize(stmt);
}

std::vector<Message> Database::get_messages(const std::string& user, int limit) {
    std::vector<Message> messages;
    
    if (!db_) return messages;

    const char* sql = R"(
        SELECT from_user, to_user, content, message_type, timestamp
        FROM messages
        WHERE from_user = ? OR to_user = ?
        ORDER BY timestamp DESC
        LIMIT ?
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::error("Failed to prepare statement");
        return messages;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message msg;
        msg.from = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        msg.to = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        msg.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        msg.timestamp = sqlite3_column_int64(stmt, 4);
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

} // namespace chat
