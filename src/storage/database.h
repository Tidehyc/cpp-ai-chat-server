#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include "protocol/message.h"

namespace chat {

class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    bool init();
    void save_message(const Message& msg);
    std::vector<Message> get_messages(const std::string& user, int limit = 50);
    
    // 获取 SQLite 数据库句柄
    sqlite3* get_db() { return db_; }

private:
    sqlite3* db_;
    std::string db_path_;
};

} // namespace chat
