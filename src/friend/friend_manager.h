#pragma once

#include <string>
#include <vector>

namespace chat {

// 好友关系结构
struct Friendship {
    int id = 0;
    int user_id = 0;
    int friend_id = 0;
    std::string friend_username;
    int64_t created_at = 0;
    
    bool is_valid() const { return id > 0; }
};

// 好友请求结构
struct FriendRequest {
    int id = 0;
    int sender_id = 0;
    std::string sender_username;
    int receiver_id = 0;
    std::string status;  // pending, accepted, rejected
    int64_t created_at = 0;
    
    bool is_valid() const { return id > 0; }
};

// 好友管理器
class FriendManager {
public:
    FriendManager(class Database* db);
    
    // 好友操作
    bool add_friend(int user_id, int friend_id);
    bool remove_friend(int user_id, int friend_id);
    std::vector<Friendship> get_friends(int user_id);
    bool is_friend(int user_id, int friend_id);
    
    // 好友请求操作
    bool send_request(int sender_id, int receiver_id);
    std::vector<FriendRequest> get_received_requests(int receiver_id);
    bool accept_request(int sender_id, int receiver_id);
    bool reject_request(int sender_id, int receiver_id);
    bool has_pending_request(int sender_id, int receiver_id);
    
    // 获取用户 ID
    int get_user_id_by_name(const std::string& username);
    
    // 获取标准用户名（从数据库中获取正确的用户名格式）
    std::string get_username_by_id(int user_id);

private:
    class Database* db_;
};

} // namespace chat
