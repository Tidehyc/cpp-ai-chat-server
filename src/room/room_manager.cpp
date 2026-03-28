#include "room_manager.h"
#include "storage/database.h"
#include "utils/logger.h"

namespace chat {

RoomManager::RoomManager(Database* db) : db_(db) {
    // 初始化房间表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS rooms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            ai_prompt TEXT NOT NULL,
            ai_temperature REAL DEFAULT 0.7,
            max_context INTEGER DEFAULT 10,
            created_by INTEGER,
            created_at INTEGER NOT NULL
        )
    )";

    char* err_msg = nullptr;
    if (sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        Logger::error("Failed to create rooms table: " + std::string(err_msg));
        sqlite3_free(err_msg);
    }

    // 初始化内置房间
    init_builtin_rooms();
}

void RoomManager::init_builtin_rooms() {
    const BuiltinRoom builtin_rooms[] = {
        {"general", "你是友好的 AI 助手，热情地帮助用户解答各种问题。"},
        {"code", "你是资深程序员，擅长代码编写、Debug、代码审查。你提供的代码简洁高效，并附带详细解释。"},
        {"translate", "你是专业翻译，精通多国语言。你只输出翻译结果，不做额外解释。"},
        {"learn", "你是耐心的老师，擅长用通俗易懂的方式讲解知识，会用生活中的例子帮助理解。"},
        {"write", "你是专业作家，擅长各种文体写作，包括文章、故事、诗歌、公文等。"},
        {"debug", "你是 Debug 专家，擅长定位和解决问题。你会逐步分析问题，提供清晰的解决方案。"},
    };

    for (const auto& builtin : builtin_rooms) {
        create_room(builtin.name, builtin.ai_prompt, 0);
    }
    Logger::info("Built-in rooms initialized");
}

bool RoomManager::create_room(const std::string& name, const std::string& ai_prompt, int created_by) {
    // 检查房间是否已存在
    Room existing = get_room(name);
    if (existing.is_valid()) {
        return false;
    }

    const char* sql = R"(
        INSERT INTO rooms (name, ai_prompt, created_by, created_at)
        VALUES (?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::error("Failed to prepare create room statement");
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ai_prompt.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, created_by);
    sqlite3_bind_int64(stmt, 4, std::time(nullptr));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        Logger::info("Room created: " + name);
        return true;
    }

    return false;
}

bool RoomManager::join_room(int user_id, const std::string& room_name) {
    Room room = get_room(room_name);
    if (!room.is_valid()) {
        return false;
    }

    room.join(user_id);
    Logger::info("User " + std::to_string(user_id) + " joined room " + room_name);
    return true;
}

bool RoomManager::leave_room(int user_id, const std::string& room_name) {
    Room room = get_room(room_name);
    if (!room.is_valid()) {
        return false;
    }

    room.leave(user_id);
    Logger::info("User " + std::to_string(user_id) + " left room " + room_name);
    return true;
}

Room RoomManager::get_room(const std::string& room_name) {
    Room room;
    const char* sql = R"(
        SELECT id, name, ai_prompt, ai_temperature, max_context, created_at
        FROM rooms
        WHERE name = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            room.id = sqlite3_column_int(stmt, 0);
            room.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            room.ai_prompt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            room.ai_temperature = sqlite3_column_double(stmt, 3);
            room.max_context = sqlite3_column_int(stmt, 4);
            room.created_at = sqlite3_column_int64(stmt, 5);
        }
        sqlite3_finalize(stmt);
    }

    return room;
}

std::vector<Room> RoomManager::get_all_rooms() {
    std::vector<Room> rooms;
    const char* sql = R"(
        SELECT id, name, ai_prompt, ai_temperature, max_context, created_at
        FROM rooms
        ORDER BY name
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Room room;
            room.id = sqlite3_column_int(stmt, 0);
            room.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            room.ai_prompt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            room.ai_temperature = sqlite3_column_double(stmt, 3);
            room.max_context = sqlite3_column_int(stmt, 4);
            room.created_at = sqlite3_column_int64(stmt, 5);
            rooms.push_back(room);
        }
        sqlite3_finalize(stmt);
    }

    return rooms;
}

std::vector<Room> RoomManager::get_user_rooms(int user_id) {
    return get_all_rooms();
}

std::vector<int> RoomManager::get_room_members(const std::string& room_name) {
    Room room = get_room(room_name);
    if (!room.is_valid()) {
        return {};
    }
    return std::vector<int>(room.member_ids.begin(), room.member_ids.end());
}

} // namespace chat
