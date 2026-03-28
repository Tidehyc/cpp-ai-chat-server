#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace chat {

// 消息记录
struct MessageRecord {
    int64_t id;
    std::string from_user;
    std::string to_user;
    std::string room_name;
    std::string content;
    std::string message_type;
    int64_t timestamp;
};

// 搜索条件
struct SearchCriteria {
    std::string keyword;
    std::string from_user;
    std::string room_name;
    int64_t start_time = 0;
    int64_t end_time = 0;
    int limit = 20;
    
    bool has_keyword() const { return !keyword.empty(); }
    bool has_from_user() const { return !from_user.empty(); }
    bool has_room() const { return !room_name.empty(); }
};

// 消息历史管理器
class MessageHistoryManager {
public:
    MessageHistoryManager(class Database* db);
    
    // 获取历史消息
    std::vector<MessageRecord> get_history(
        const std::string& target,  // 用户名或房间名
        bool is_room,
        int limit = 20
    );
    
    // 搜索消息
    std::vector<MessageRecord> search(const SearchCriteria& criteria);
    
    // 导出消息
    std::string export_messages(
        const std::string& target,
        bool is_room,
        const std::string& format = "txt"
    );
    
private:
    class Database* db_;
};

} // namespace chat
