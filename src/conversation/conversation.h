#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <ctime>

namespace chat {

// 对话消息
struct ConversationMessage {
    std::string role;       // "user" 或 "assistant"
    std::string content;    // 消息内容
    int64_t timestamp;      // 时间戳
    
    ConversationMessage() : timestamp(0) {}
    ConversationMessage(const std::string& r, const std::string& c)
        : role(r), content(c), timestamp(std::time(nullptr)) {}
};

// 对话上下文
struct ConversationContext {
    std::string room_name;          // 房间名
    std::string user_id;            // 用户 ID（私聊时）
    std::vector<ConversationMessage> history;  // 对话历史
    int max_history = 10;           // 最大保留轮数
    
    // 添加消息
    void add_message(const std::string& role, const std::string& content);
    
    // 获取最近 N 轮对话
    std::vector<ConversationMessage> get_recent_messages(int count) const;
    
    // 清空历史
    void clear();
    
    // 生成键名
    std::string get_key() const;
};

// 对话上下文管理器
class ConversationManager {
public:
    ConversationManager(class Database* db);
    
    // 获取上下文
    ConversationContext& get_context(const std::string& room_name, const std::string& user_id = "");
    
    // 添加消息
    void add_message(const std::string& room_name, const std::string& user_id,
                     const std::string& role, const std::string& content);
    
    // 清空上下文
    void clear_context(const std::string& room_name, const std::string& user_id = "");
    
    // 设置最大历史数
    void set_max_history(int max_history);
    
    // 从数据库加载上下文
    void load_context(const std::string& room_name, const std::string& user_id);
    
    // 保存上下文到数据库
    void save_context(const std::string& room_name, const std::string& user_id);
    
private:
    class Database* db_;
    std::map<std::string, ConversationContext> contexts_;  // key: room_name:user_id
    int default_max_history_ = 10;
};

} // namespace chat
