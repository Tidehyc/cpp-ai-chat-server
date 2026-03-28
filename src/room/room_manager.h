#pragma once

#include <string>
#include <vector>
#include <set>
#include <memory>

namespace chat {

// 房间结构
struct Room {
    int id = 0;
    std::string name;
    std::string ai_prompt;      // AI 系统提示词
    double ai_temperature = 0.7;
    int max_context = 10;
    std::set<int> member_ids;   // 成员 ID 列表
    int64_t created_at = 0;
    
    bool is_valid() const { return id > 0; }
    
    void join(int user_id) { member_ids.insert(user_id); }
    void leave(int user_id) { member_ids.erase(user_id); }
    bool is_member(int user_id) const { return member_ids.find(user_id) != member_ids.end(); }
    size_t member_count() const { return member_ids.size(); }
};

// 房间管理器
class RoomManager {
public:
    RoomManager(class Database* db);
    
    // 房间操作
    bool create_room(const std::string& name, const std::string& ai_prompt, int created_by);
    bool join_room(int user_id, const std::string& room_name);
    bool leave_room(int user_id, const std::string& room_name);
    Room get_room(const std::string& room_name);
    Room get_room_by_id(int room_id);
    std::vector<Room> get_all_rooms();
    std::vector<Room> get_user_rooms(int user_id);
    
    // 获取房间成员
    std::vector<int> get_room_members(const std::string& room_name);
    
    // 初始化内置房间
    void init_builtin_rooms();
    
private:
    class Database* db_;
    
    // 内置房间配置
    struct BuiltinRoom {
        const char* name;
        const char* ai_prompt;
    };
    
    static const BuiltinRoom builtin_rooms_[];
};

} // namespace chat
