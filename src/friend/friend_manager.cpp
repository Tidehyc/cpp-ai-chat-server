#include "friend_manager.h"
#include "storage/database.h"
#include "utils/logger.h"
#include <sqlite3.h>

namespace chat {

FriendManager::FriendManager(Database* db) : db_(db) {
    // 初始化好友表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS friendships (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            friend_id INTEGER NOT NULL,
            created_at INTEGER NOT NULL,
            UNIQUE(user_id, friend_id)
        );
        CREATE INDEX IF NOT EXISTS idx_user_id ON friendships(user_id);
        CREATE INDEX IF NOT EXISTS idx_friend_id ON friendships(friend_id);
        
        CREATE TABLE IF NOT EXISTS friend_requests (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender_id INTEGER NOT NULL,
            receiver_id INTEGER NOT NULL,
            status TEXT DEFAULT 'pending',  -- pending, accepted, rejected
            created_at INTEGER NOT NULL,
            UNIQUE(sender_id, receiver_id)
        );
        CREATE INDEX IF NOT EXISTS idx_sender ON friend_requests(sender_id);
        CREATE INDEX IF NOT EXISTS idx_receiver ON friend_requests(receiver_id);
        CREATE INDEX IF NOT EXISTS idx_status ON friend_requests(status);
        
        CREATE TABLE IF NOT EXISTS private_messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_user TEXT NOT NULL,
            to_user TEXT NOT NULL,
            content TEXT NOT NULL,
            is_read INTEGER DEFAULT 0,
            timestamp INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_pm_from ON private_messages(from_user);
        CREATE INDEX IF NOT EXISTS idx_pm_to ON private_messages(to_user);
        CREATE INDEX IF NOT EXISTS idx_pm_timestamp ON private_messages(timestamp)
    )";

    char* err_msg = nullptr;
    if (sqlite3_exec(db_->get_db(), sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        Logger::error("Failed to create friendship tables: " + std::string(err_msg));
        sqlite3_free(err_msg);
    } else {
        Logger::info("Friendship tables initialized");
    }
}

int FriendManager::get_user_id_by_name(const std::string& username) {
    const char* sql = "SELECT id FROM users WHERE username = ?";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int user_id = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return user_id;
        }
        sqlite3_finalize(stmt);
    }
    return -1;
}

std::string FriendManager::get_username_by_id(int user_id) {
    const char* sql = "SELECT username FROM users WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* username_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string username = username_cstr ? username_cstr : "";
            sqlite3_finalize(stmt);
            return username;
        }
        sqlite3_finalize(stmt);
    }
    return "";
}

bool FriendManager::add_friend(int user_id, int friend_id) {
    if (user_id == friend_id) {
        Logger::error("Cannot add yourself as friend");
        return false;
    }
    
    // 检查是否已经是好友
    if (is_friend(user_id, friend_id)) {
        Logger::error("Already friends");
        return false;
    }
    
    const char* sql = R"(
        INSERT OR IGNORE INTO friendships (user_id, friend_id, created_at)
        VALUES (?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_int(stmt, 2, friend_id);
        sqlite3_bind_int64(stmt, 3, std::time(nullptr));
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc == SQLITE_DONE) {
            Logger::info("Added friend: user " + std::to_string(user_id) + " -> " + std::to_string(friend_id));
            return true;
        }
    }
    
    return false;
}

bool FriendManager::remove_friend(int user_id, int friend_id) {
    // 删除双向好友关系
    // 1. user_id -> friend_id
    // 2. friend_id -> user_id
    const char* sql = "DELETE FROM friendships WHERE (user_id = ? AND friend_id = ?) OR (user_id = ? AND friend_id = ?)";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_int(stmt, 2, friend_id);
        sqlite3_bind_int(stmt, 3, friend_id);
        sqlite3_bind_int(stmt, 4, user_id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE) {
            Logger::info("Removed friend (both directions): user " + std::to_string(user_id) + " <-> " + std::to_string(friend_id));
            return true;
        }
    }

    return false;
}

std::vector<Friendship> FriendManager::get_friends(int user_id) {
    std::vector<Friendship> friends;
    
    // 查询双向好友关系
    // 1. user_id 添加的好友（user_id -> friend_id）
    const char* sql1 = R"(
        SELECT f.id, f.user_id, f.friend_id, u.username, f.created_at
        FROM friendships f
        JOIN users u ON f.friend_id = u.id
        WHERE f.user_id = ?
        ORDER BY u.username
    )";
    
    // 2. 添加 user_id 为好友的（friend_id -> user_id，此时 friend_id 是真正的好友）
    const char* sql2 = R"(
        SELECT f.id, f.friend_id as user_id, f.user_id as friend_id, u.username, f.created_at
        FROM friendships f
        JOIN users u ON f.user_id = u.id
        WHERE f.friend_id = ?
        ORDER BY u.username
    )";
    
    sqlite3_stmt* stmt = nullptr;
    
    // 查询第一类好友
    if (sqlite3_prepare_v2(db_->get_db(), sql1, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Friendship friendship;
            friendship.id = sqlite3_column_int(stmt, 0);
            friendship.user_id = sqlite3_column_int(stmt, 1);
            friendship.friend_id = sqlite3_column_int(stmt, 2);
            friendship.friend_username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            friendship.created_at = sqlite3_column_int64(stmt, 4);
            friends.push_back(friendship);
        }
        
        sqlite3_finalize(stmt);
    }
    
    // 查询第二类好友
    if (sqlite3_prepare_v2(db_->get_db(), sql2, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Friendship friendship;
            friendship.id = sqlite3_column_int(stmt, 0);
            friendship.user_id = sqlite3_column_int(stmt, 1);
            friendship.friend_id = sqlite3_column_int(stmt, 2);
            friendship.friend_username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            friendship.created_at = sqlite3_column_int64(stmt, 4);
            
            // 避免重复
            bool exists = false;
            for (const auto& f : friends) {
                if (f.friend_id == friendship.friend_id) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                friends.push_back(friendship);
            }
        }
        
        sqlite3_finalize(stmt);
    }
    
    Logger::info("Got " + std::to_string(friends.size()) + " friends for user " + std::to_string(user_id));
    return friends;
}

bool FriendManager::is_friend(int user_id, int friend_id) {
    // 检查双向好友关系
    // 1. user_id -> friend_id
    // 2. friend_id -> user_id
    const char* sql = "SELECT COUNT(*) FROM friendships WHERE (user_id = ? AND friend_id = ?) OR (user_id = ? AND friend_id = ?)";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_int(stmt, 2, friend_id);
        sqlite3_bind_int(stmt, 3, friend_id);
        sqlite3_bind_int(stmt, 4, user_id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return count > 0;
        }
        sqlite3_finalize(stmt);
    }
    
    return false;
}

// ========== 好友请求操作 ==========

bool FriendManager::has_pending_request(int sender_id, int receiver_id) {
    const char* sql = "SELECT COUNT(*) FROM friend_requests WHERE sender_id = ? AND receiver_id = ? AND status = 'pending'";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, sender_id);
        sqlite3_bind_int(stmt, 2, receiver_id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return count > 0;
        }
        sqlite3_finalize(stmt);
    }
    
    return false;
}

bool FriendManager::send_request(int sender_id, int receiver_id) {
    Logger::info("Attempting to send friend request: " + std::to_string(sender_id) + " -> " + std::to_string(receiver_id));
    
    if (sender_id == receiver_id) {
        Logger::error("Cannot send friend request to yourself");
        return false;
    }

    // 检查是否已经是好友
    if (is_friend(sender_id, receiver_id)) {
        Logger::info("Already friends, cannot send request");
        return false;
    }

    // 检查是否有待处理的请求
    if (has_pending_request(sender_id, receiver_id)) {
        Logger::info("Friend request already sent");
        return false;
    }

    const char* sql = R"(
        INSERT OR REPLACE INTO friend_requests (sender_id, receiver_id, status, created_at)
        VALUES (?, ?, 'pending', ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, sender_id);
        sqlite3_bind_int(stmt, 2, receiver_id);
        sqlite3_bind_int64(stmt, 3, std::time(nullptr));
        int rc = sqlite3_step(stmt);
        
        if (rc == SQLITE_DONE) {
            Logger::info("Friend request sent successfully: " + std::to_string(sender_id) + " -> " + std::to_string(receiver_id));
            sqlite3_finalize(stmt);
            return true;
        } else {
            const char* errmsg = sqlite3_errmsg(db_->get_db());
            Logger::error("Failed to send friend request, rc=" + std::to_string(rc) + ", error=" + std::string(errmsg));
            sqlite3_finalize(stmt);
            return false;
        }
    } else {
        const char* errmsg = sqlite3_errmsg(db_->get_db());
        Logger::error("Failed to prepare statement: " + std::string(errmsg));
        return false;
    }

    return false;
}

std::vector<FriendRequest> FriendManager::get_received_requests(int receiver_id) {
    Logger::info("Getting received friend requests for user " + std::to_string(receiver_id));
    
    std::vector<FriendRequest> requests;

    const char* sql = R"(
        SELECT r.id, r.sender_id, u.username, r.receiver_id, r.status, r.created_at
        FROM friend_requests r
        JOIN users u ON r.sender_id = u.id
        WHERE r.receiver_id = ? AND r.status = 'pending'
        ORDER BY r.created_at DESC
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, receiver_id);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            FriendRequest request;
            request.id = sqlite3_column_int(stmt, 0);
            request.sender_id = sqlite3_column_int(stmt, 1);
            request.sender_username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            request.receiver_id = sqlite3_column_int(stmt, 3);
            request.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            request.created_at = sqlite3_column_int64(stmt, 5);
            requests.push_back(request);
        }

        sqlite3_finalize(stmt);
    }

    Logger::info("Got " + std::to_string(requests.size()) + " pending friend requests for user " + std::to_string(receiver_id));
    return requests;
}

bool FriendManager::accept_request(int sender_id, int receiver_id) {
    // 首先检查是否存在待处理的请求
    if (!has_pending_request(sender_id, receiver_id)) {
        Logger::info("No pending friend request from " + std::to_string(sender_id) + " to " + std::to_string(receiver_id));
        return false;
    }

    Logger::info("Accepting friend request: " + std::to_string(sender_id) + " -> " + std::to_string(receiver_id));

    // 开始事务
    sqlite3_exec(db_->get_db(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    // 更新请求状态为 accepted
    const char* update_sql = "UPDATE friend_requests SET status = 'accepted' WHERE sender_id = ? AND receiver_id = ? AND status = 'pending'";
    sqlite3_stmt* stmt = nullptr;

    bool success = false;
    if (sqlite3_prepare_v2(db_->get_db(), update_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, sender_id);
        sqlite3_bind_int(stmt, 2, receiver_id);
        int rc = sqlite3_step(stmt);
        Logger::info("Update request status result: " + std::to_string(rc));
        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE) {
            // 添加双向好友关系
            const char* insert_sql = R"(
                INSERT OR IGNORE INTO friendships (user_id, friend_id, created_at)
                VALUES (?, ?, ?)
            )";

            if (sqlite3_prepare_v2(db_->get_db(), insert_sql, -1, &stmt, nullptr) == SQLITE_OK) {
                // sender -> receiver
                sqlite3_bind_int(stmt, 1, sender_id);
                sqlite3_bind_int(stmt, 2, receiver_id);
                sqlite3_bind_int64(stmt, 3, std::time(nullptr));
                int rc1 = sqlite3_step(stmt);
                Logger::info("Insert friendship 1 (sender->receiver) rc: " + std::to_string(rc1));

                // 重新准备语句
                sqlite3_reset(stmt);
                
                // receiver -> sender
                sqlite3_bind_int(stmt, 1, receiver_id);
                sqlite3_bind_int(stmt, 2, sender_id);
                int rc2 = sqlite3_step(stmt);
                Logger::info("Insert friendship 2 (receiver->sender) rc: " + std::to_string(rc2));

                sqlite3_finalize(stmt);

                // SQLITE_DONE=1 (插入成功), SQLITE_IGNORE=101 (已存在，忽略)
                // 只要不是错误都算成功
                if (rc1 == SQLITE_DONE || rc1 == SQLITE_IGNORE) {
                    if (rc2 == SQLITE_DONE || rc2 == SQLITE_IGNORE) {
                        sqlite3_exec(db_->get_db(), "COMMIT", nullptr, nullptr, nullptr);
                        success = true;
                        Logger::info("Friend request accepted: " + std::to_string(sender_id) + " <-> " + std::to_string(receiver_id));
                    } else {
                        sqlite3_exec(db_->get_db(), "ROLLBACK", nullptr, nullptr, nullptr);
                        Logger::error("Failed to insert friendship 2, rc=" + std::to_string(rc2));
                    }
                } else {
                    sqlite3_exec(db_->get_db(), "ROLLBACK", nullptr, nullptr, nullptr);
                    Logger::error("Failed to insert friendship 1, rc=" + std::to_string(rc1));
                }
            }
        }
    }

    if (!success) {
        sqlite3_exec(db_->get_db(), "ROLLBACK", nullptr, nullptr, nullptr);
    }

    return success;
}

bool FriendManager::reject_request(int sender_id, int receiver_id) {
    const char* sql = "UPDATE friend_requests SET status = 'rejected' WHERE sender_id = ? AND receiver_id = ? AND status = 'pending'";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, sender_id);
        sqlite3_bind_int(stmt, 2, receiver_id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc == SQLITE_DONE) {
            Logger::info("Friend request rejected: " + std::to_string(sender_id) + " -> " + std::to_string(receiver_id));
            return true;
        }
    }
    
    return false;
}

} // namespace chat
