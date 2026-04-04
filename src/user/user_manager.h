#pragma once

#include <string>
#include <vector>
#include <set>

namespace chat {

// 用户结构
struct User {
    int id = 0;
    std::string username;
    std::string password_hash;
    std::string current_preset = "friend";
    std::string status = "online";
    int64_t created_at = 0;

    bool is_valid() const { return id > 0; }
};

// 登录结果枚举
enum class LoginResult {
    SUCCESS = 0,        // 登录成功
    USER_NOT_FOUND,     // 用户不存在
    WRONG_PASSWORD      // 密码错误
};

// 用户管理器
class UserManager {
public:
    UserManager(class Database* db);

    // 用户操作
    bool register_user(const std::string& username, const std::string& password);
    LoginResult login(const std::string& username, const std::string& password, User& user);
    User get_user(int user_id);
    User get_user_by_name(const std::string& username);
    bool update_status(int user_id, const std::string& status);
    bool update_preset(int user_id, const std::string& preset);

    // 会话管理
    std::string generate_session_token();
    bool set_session_token(int user_id, const std::string& token);
    bool verify_session_token(int user_id, const std::string& token);
    void remove_session_token(int user_id);

    // 用户列表
    std::vector<User> get_all_users();
    std::vector<User> get_online_users();
    void set_user_online(int user_id, bool online);
    bool is_online(int user_id);
    std::string get_user_status(int user_id);

    // 服务器启动时重置所有用户状态
    void reset_all_users_offline();

private:
    class Database* db_;
    std::set<int> online_users_;
};

} // namespace chat
