#include "user_manager.h"
#include "storage/database.h"
#include "utils/logger.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <vector>

namespace chat {

// 简单的密码哈希
static std::string hash_password(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

UserManager::UserManager(Database* db) : db_(db) {
    // 初始化用户表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT,
            current_preset TEXT DEFAULT 'friend',
            status TEXT DEFAULT 'online',
            created_at INTEGER NOT NULL
        )
    )";
    
    char* err_msg = nullptr;
    if (sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        Logger::error("Failed to create users table: " + std::string(err_msg));
        sqlite3_free(err_msg);
    }
    
    // 服务器启动时重置所有用户状态为 offline
    reset_all_users_offline();
}

void UserManager::reset_all_users_offline() {
    const char* sql = "UPDATE users SET status = 'offline'";
    char* err_msg = nullptr;
    
    if (sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        Logger::error("Failed to reset user status: " + std::string(err_msg));
        sqlite3_free(err_msg);
    } else {
        Logger::info("All users reset to offline status");
    }
}

bool UserManager::register_user(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        return false;
    }
    
    const char* sql = R"(
        INSERT INTO users (username, password_hash, status, created_at)
        VALUES (?, ?, 'offline', ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::error("Failed to prepare register statement");
        return false;
    }
    
    std::string pwd_hash = hash_password(password);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pwd_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, std::time(nullptr));
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        Logger::info("User registered: " + username + " (status: offline)");
        return true;
    } else if (rc == SQLITE_CONSTRAINT) {
        Logger::info("Username already exists: " + username);
        return false;  // 用户名已存在
    }
    
    return false;
}

bool UserManager::login(const std::string& username, const std::string& password, User& user) {
    const char* sql = R"(
        SELECT id, username, password_hash, current_preset, status, created_at
        FROM users
        WHERE username = ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::error("Failed to prepare login statement");
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string stored_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.current_preset = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user.created_at = sqlite3_column_int64(stmt, 5);
        
        // 验证密码
        if (stored_hash == hash_password(password)) {
            sqlite3_finalize(stmt);
            Logger::info("User logged in: " + username);
            return true;
        }
    }
    
    sqlite3_finalize(stmt);
    Logger::info("Login failed for: " + username);
    return false;
}

User UserManager::get_user(int user_id) {
    User user;
    const char* sql = R"(
        SELECT id, username, current_preset, status, created_at
        FROM users
        WHERE id = ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user.id = sqlite3_column_int(stmt, 0);
            user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user.current_preset = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            user.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            user.created_at = sqlite3_column_int64(stmt, 4);
        }
        sqlite3_finalize(stmt);
    }
    
    return user;
}

User UserManager::get_user_by_name(const std::string& username) {
    User user;
    const char* sql = R"(
        SELECT id, username, current_preset, status, created_at
        FROM users
        WHERE username = ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user.id = sqlite3_column_int(stmt, 0);
            user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user.current_preset = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            user.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            user.created_at = sqlite3_column_int64(stmt, 4);
        }
        sqlite3_finalize(stmt);
    }
    
    return user;
}

bool UserManager::update_status(int user_id, const std::string& status) {
    const char* sql = "UPDATE users SET status = ? WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, user_id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
    
    return false;
}

bool UserManager::update_preset(int user_id, const std::string& preset) {
    const char* sql = "UPDATE users SET current_preset = ? WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, preset.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, user_id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }
    
    return false;
}

std::vector<User> UserManager::get_all_users() {
    std::vector<User> users;
    // 返回所有用户
    const char* sql = R"(
        SELECT id, username, current_preset, status, created_at
        FROM users
        ORDER BY username
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            User user;
            user.id = sqlite3_column_int(stmt, 0);
            user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user.current_preset = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            user.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            user.created_at = sqlite3_column_int64(stmt, 4);
            users.push_back(user);
        }
        sqlite3_finalize(stmt);
    }
    
    return users;
}

std::vector<User> UserManager::get_online_users() {
    std::vector<User> users;
    // 返回所有非 offline 状态的用户
    const char* sql = R"(
        SELECT id, username, current_preset, status, created_at
        FROM users
        WHERE status != 'offline'
        ORDER BY username
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            User user;
            user.id = sqlite3_column_int(stmt, 0);
            user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user.current_preset = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            user.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            user.created_at = sqlite3_column_int64(stmt, 4);
            users.push_back(user);
        }
        sqlite3_finalize(stmt);
    }
    
    return users;
}

void UserManager::set_user_online(int user_id, bool online) {
    if (online) {
        online_users_.insert(user_id);
    } else {
        online_users_.erase(user_id);
    }
}

bool UserManager::is_online(int user_id) {
    return online_users_.find(user_id) != online_users_.end();
}

} // namespace chat
