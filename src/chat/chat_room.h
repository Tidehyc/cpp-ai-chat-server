#pragma once

#include <string>
#include <set>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>

namespace chat {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

// 聊天室
class ChatRoom {
public:
    ChatRoom(const std::string& name);
    
    // 成员管理
    void join(ConnectionPtr conn);
    void leave(ConnectionPtr conn);
    bool is_member(ConnectionPtr conn) const;
    size_t member_count() const;
    
    // 消息广播
    void broadcast(const std::string& message, ConnectionPtr exclude = nullptr);
    void broadcast_to_ai(const std::string& message, ConnectionPtr from);
    
    // 获取信息
    std::string get_name() const { return name_; }
    const std::set<ConnectionPtr>& get_members() const { return members_; }
    
private:
    std::string name_;
    std::set<ConnectionPtr> members_;
};

// 聊天室管理器
class ChatRoomManager {
public:
    ChatRoomManager();
    
    // 聊天室操作
    bool create_room(const std::string& name);
    bool join_room(ConnectionPtr conn, const std::string& room_name);
    bool leave_room(ConnectionPtr conn, const std::string& room_name);
    void leave_all_rooms(ConnectionPtr conn);
    
    // 获取聊天室
    ChatRoom* get_room(const std::string& room_name);
    std::vector<ChatRoom*> get_user_rooms(ConnectionPtr conn);
    std::vector<ChatRoom*> get_all_rooms();
    
    // 消息广播
    void broadcast_to_room(const std::string& room_name, const std::string& message, ConnectionPtr from);
    
private:
    std::map<std::string, std::unique_ptr<ChatRoom>> rooms_;
    std::map<ConnectionPtr, std::set<std::string>> user_rooms_;  // 用户所在的房间
};

} // namespace chat
