#include "conversation.h"
#include "storage/database.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>

namespace chat {

using json = nlohmann::json;

// ========== ConversationContext 实现 ==========

void ConversationContext::add_message(const std::string& role, const std::string& content) {
    history.push_back(ConversationMessage(role, content));
    
    // 保持历史记录在限制范围内
    while (static_cast<int>(history.size()) > max_history * 2) {  // 每轮 2 条（user + assistant）
        history.erase(history.begin());
    }
}

std::vector<ConversationMessage> ConversationContext::get_recent_messages(int count) const {
    if (count >= static_cast<int>(history.size())) {
        return history;
    }
    return std::vector<ConversationMessage>(history.end() - count, history.end());
}

void ConversationContext::clear() {
    history.clear();
}

std::string ConversationContext::get_key() const {
    if (user_id.empty()) {
        return room_name;
    }
    return room_name + ":" + user_id;
}

// ========== ConversationManager 实现 ==========

ConversationManager::ConversationManager(Database* db) : db_(db) {
    // 创建上下文表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS conversations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_name TEXT NOT NULL,
            user_id TEXT,
            history_json TEXT,
            updated_at INTEGER NOT NULL,
            UNIQUE(room_name, user_id)
        )
    )";
    
    char* err_msg = nullptr;
    if (sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        Logger::error("Failed to create conversations table: " + std::string(err_msg));
        sqlite3_free(err_msg);
    }
}

ConversationContext& ConversationManager::get_context(const std::string& room_name, const std::string& user_id) {
    ConversationContext ctx;
    ctx.room_name = room_name;
    ctx.user_id = user_id;
    ctx.max_history = default_max_history_;
    
    std::string key = ctx.get_key();
    
    // 查找或创建上下文
    auto it = contexts_.find(key);
    if (it == contexts_.end()) {
        contexts_[key] = ctx;
    }
    
    return contexts_[key];
}

void ConversationManager::add_message(const std::string& room_name, const std::string& user_id,
                                       const std::string& role, const std::string& content) {
    ConversationContext& ctx = get_context(room_name, user_id);
    ctx.add_message(role, content);
}

void ConversationManager::clear_context(const std::string& room_name, const std::string& user_id) {
    std::string key = room_name + ":" + user_id;
    auto it = contexts_.find(key);
    if (it != contexts_.end()) {
        it->second.clear();
        contexts_.erase(it);
    }
    
    // 从数据库删除
    const char* sql = "DELETE FROM conversations WHERE room_name = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    Logger::info("Conversation context cleared for " + room_name + ":" + user_id);
}

void ConversationManager::set_max_history(int max_history) {
    default_max_history_ = max_history;
    
    // 更新所有现有上下文
    for (auto& pair : contexts_) {
        pair.second.max_history = max_history;
    }
}

void ConversationManager::load_context(const std::string& room_name, const std::string& user_id) {
    const char* sql = R"(
        SELECT history_json FROM conversations
        WHERE room_name = ? AND user_id = ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string json_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            
            try {
                auto j = json::parse(json_str);
                ConversationContext& ctx = get_context(room_name, user_id);
                
                if (j.contains("history") && j["history"].is_array()) {
                    for (const auto& msg : j["history"]) {
                        ctx.history.push_back(ConversationMessage(
                            msg.value("role", "user"),
                            msg.value("content", "")
                        ));
                    }
                }
                
                Logger::info("Loaded conversation context for " + room_name + ":" + user_id);
            } catch (const std::exception& e) {
                Logger::error("Failed to parse conversation history: " + std::string(e.what()));
            }
        }
        sqlite3_finalize(stmt);
    }
}

void ConversationManager::save_context(const std::string& room_name, const std::string& user_id) {
    ConversationContext& ctx = get_context(room_name, user_id);
    
    // 转换为 JSON
    json j;
    j["history"] = json::array();
    for (const auto& msg : ctx.history) {
        j["history"].push_back({
            {"role", msg.role},
            {"content", msg.content},
            {"timestamp", msg.timestamp}
        });
    }
    
    std::string json_str = j.dump();
    
    // 保存到数据库
    const char* sql = R"(
        INSERT OR REPLACE INTO conversations (room_name, user_id, history_json, updated_at)
        VALUES (?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, room_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, json_str.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 4, std::time(nullptr));
        
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Logger::error("Failed to save conversation context");
        }
        sqlite3_finalize(stmt);
    }
}

} // namespace chat
