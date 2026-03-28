#include "chat_room.h"
#include "server/connection.h"
#include "utils/logger.h"
#include <algorithm>

namespace chat {

// ========== ChatRoom 实现 ==========

ChatRoom::ChatRoom(const std::string& name) : name_(name) {}

void ChatRoom::join(ConnectionPtr conn) {
    members_.insert(conn);
    Logger::info("User joined room: " + name_);
}

void ChatRoom::leave(ConnectionPtr conn) {
    members_.erase(conn);
    Logger::info("User left room: " + name_);
}

bool ChatRoom::is_member(ConnectionPtr conn) const {
    return members_.find(conn) != members_.end();
}

size_t ChatRoom::member_count() const {
    return members_.size();
}

void ChatRoom::broadcast(const std::string& message, ConnectionPtr exclude) {
    for (auto& member : members_) {
        if (member != exclude && member.use_count() > 0) {
            member->send(message);
        }
    }
}

void ChatRoom::broadcast_to_ai(const std::string& message, ConnectionPtr from) {
    // 在群聊中，只有 @AI 或特定格式才触发 AI
    // 这里简单实现：广播消息，AI 会收到并处理
    broadcast(message);
}

// ========== ChatRoomManager 实现 ==========

ChatRoomManager::ChatRoomManager() {}

bool ChatRoomManager::create_room(const std::string& name) {
    if (rooms_.find(name) != rooms_.end()) {
        return false;  // 房间已存在
    }
    
    rooms_[name] = std::make_unique<ChatRoom>(name);
    Logger::info("Chat room created: " + name);
    return true;
}

bool ChatRoomManager::join_room(ConnectionPtr conn, const std::string& room_name) {
    auto it = rooms_.find(room_name);
    if (it == rooms_.end()) {
        return false;  // 房间不存在
    }
    
    it->second->join(conn);
    user_rooms_[conn].insert(room_name);
    return true;
}

bool ChatRoomManager::leave_room(ConnectionPtr conn, const std::string& room_name) {
    auto it = rooms_.find(room_name);
    if (it == rooms_.end()) {
        return false;
    }
    
    it->second->leave(conn);
    user_rooms_[conn].erase(room_name);
    
    // 如果用户不在任何房间，删除记录
    if (user_rooms_[conn].empty()) {
        user_rooms_.erase(conn);
    }
    
    return true;
}

void ChatRoomManager::leave_all_rooms(ConnectionPtr conn) {
    auto it = user_rooms_.find(conn);
    if (it != user_rooms_.end()) {
        for (const auto& room_name : it->second) {
            auto room_it = rooms_.find(room_name);
            if (room_it != rooms_.end()) {
                room_it->second->leave(conn);
            }
        }
        user_rooms_.erase(it);
    }
}

ChatRoom* ChatRoomManager::get_room(const std::string& room_name) {
    auto it = rooms_.find(room_name);
    if (it != rooms_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<ChatRoom*> ChatRoomManager::get_user_rooms(ConnectionPtr conn) {
    std::vector<ChatRoom*> result;
    auto it = user_rooms_.find(conn);
    if (it != user_rooms_.end()) {
        for (const auto& room_name : it->second) {
            auto room_it = rooms_.find(room_name);
            if (room_it != rooms_.end()) {
                result.push_back(room_it->second.get());
            }
        }
    }
    return result;
}

std::vector<ChatRoom*> ChatRoomManager::get_all_rooms() {
    std::vector<ChatRoom*> result;
    for (auto& pair : rooms_) {
        result.push_back(pair.second.get());
    }
    return result;
}

void ChatRoomManager::broadcast_to_room(const std::string& room_name, const std::string& message, ConnectionPtr from) {
    auto it = rooms_.find(room_name);
    if (it != rooms_.end()) {
        it->second->broadcast(message, from);
    }
}

} // namespace chat
