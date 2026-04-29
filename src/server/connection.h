#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include "protocol/websocket.h"
#include "protocol/message.h"
#include "ai/ai_client.h"
#include "conversation/conversation.h"
#include "friend/friend_manager.h"
#include "command/command_dispatcher.h"

namespace chat
{

    class Database;
    class AIClient;
    class UserManager;
    class RoomManager;
    class PresetManager;
    class ConversationManager;
    class MessageHistoryManager;
    class FriendManager;

    // 连接状态
    enum class ConnectionState
    {
        HANDSHAKE,     // 等待 WebSocket 握手
        AUTHENTICATED, // 已认证（登录）
        ACTIVE,        // 活跃状态
        CLOSING,       // 关闭中
        CLOSED         // 已关闭
    };

    // 前向声明和类型别名
    class Connection;
    using ConnectionPtr = std::shared_ptr<Connection>;

    // 会话数据
    struct Session
    {
        int user_id = 0;
        std::string username;
        std::string role = "user";
        std::string current_room = "general";  // 当前所在房间
        std::string current_preset = "friend"; // 当前 AI 预设
        std::string status = "online";         // 在线状态
        std::string session_token;             // 会话 token

        bool is_authenticated() const { return user_id > 0; }
    };

    class Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        explicit Connection(boost::asio::ip::tcp::socket socket,
                            Database *db, AIClient *ai_client,
                            UserManager *user_manager, RoomManager *room_manager, PresetManager *preset_manager,
                            ConversationManager *conversation_manager, MessageHistoryManager *history_manager,
                            FriendManager *friend_manager);
        ~Connection();

        void start();
        void send(const std::string &message);
        void close();

        void set_id(uint64_t id) { id_ = id; }
        uint64_t get_id() const { return id_; }
        ConnectionState get_state() const { return state_; }

        // 聊天室支持
        void set_chat_room(const std::string &room_name) { current_chat_room_ = room_name; }
        std::string get_chat_room() const { return current_chat_room_; }

    private:
        static std::set<ConnectionPtr> snapshot_all_connections();
        static std::set<std::string> snapshot_created_rooms();
        static ConnectionPtr find_connection_by_user_id(int user_id);
        static ConnectionPtr find_connection_by_username(const std::string &username);

        void do_read_handshake();
        void do_read_websocket();
        void do_write();
        void on_message(const Message &msg);
        void handle_websocket_frame(const WebSocketFrame &frame);
        std::string get_remote_ip() const;
        bool allow_command_rate(const std::string &input, const std::string &request_id);
        bool try_handle_http_healthcheck();

        // 命令处理
        void handle_command(const std::string &input);
        void handle_chat_message(const std::string &content);
        void execute_command(const std::string &cmd_name, const std::vector<std::string> &args);
        void broadcast_to_room(const std::string &message);
        void send_ai_response(const std::string &prompt, const std::string &user_message,
                              const std::vector<AIMessage> &context = {});

        // 用户命令
        void cmd_register(const std::vector<std::string> &args);
        void cmd_login(const std::vector<std::string> &args);
        void cmd_users(const std::vector<std::string> &args);
        void cmd_status(const std::vector<std::string> &args);
        void cmd_ban(const std::vector<std::string> &args);
        void cmd_unban(const std::vector<std::string> &args);
        void cmd_modlog(const std::vector<std::string> &args);
        void cmd_report(const std::vector<std::string> &args);
        void cmd_announce(const std::vector<std::string> &args);
        void cmd_friend(const std::vector<std::string> &args);
        void cmd_friend_list();
        void cmd_friend_request(const std::vector<std::string> &args);
        void cmd_friend_requests();
        void cmd_friend_accept(const std::vector<std::string> &args);
        void cmd_friend_reject(const std::vector<std::string> &args);
        void cmd_friend_delete(const std::vector<std::string> &args);
        void cmd_private_message(const std::vector<std::string> &args);
        void broadcast_online_users();
        void send_room_list();

        static std::string get_status_icon(const std::string &status);
        static std::string build_status_line(const std::string &username, const std::string &status);
        std::string get_friend_status(int friend_user_id) const;
        std::string build_friend_list_text(const std::vector<Friendship> &friends) const;
        std::string build_friend_list_json(const std::vector<Friendship> &friends) const;
        std::string build_room_list_json() const;

        // 房间命令
        void cmd_rooms(const std::vector<std::string> &args);
        void cmd_join(const std::vector<std::string> &args);
        void cmd_leave(const std::vector<std::string> &args);

        // 群聊命令
        void cmd_create(const std::vector<std::string> &args);
        void cmd_list(const std::vector<std::string> &args);

        // AI 命令
        void cmd_msg(const std::vector<std::string> &args);
        void cmd_preset(const std::vector<std::string> &args);

        // 上下文命令
        void cmd_clear(const std::vector<std::string> &args);
        void cmd_context(const std::vector<std::string> &args);

        // 历史命令
        void cmd_history(const std::vector<std::string> &args);
        void cmd_search(const std::vector<std::string> &args);
        void cmd_export(const std::vector<std::string> &args);

        // 已读状态命令
        void cmd_mark_read(const std::vector<std::string> &args);
        void cmd_unread_count(const std::vector<std::string> &args);

        // 系统命令
        void cmd_help(const std::vector<std::string> &args);

        // 工具方法
        void send_system_message(const std::string &content);
        void send_error_message(const std::string &error_code,
                                const std::string &content,
                                const std::string &request_id,
                                const std::string &module);
        void send_room_list_json();
        void broadcast_room_list();
        void notify_friend_online(int user_id, const std::string &username);
        void send_offline_messages();
        std::string get_room_ai_prompt(const std::string &room_name);
        std::string get_preset_ai_prompt(const std::string &preset_name);

        boost::asio::ip::tcp::socket socket_;
        ConnectionState state_;
        uint64_t id_;
        Session session_;

        // 读取缓冲
        std::vector<uint8_t> read_buffer_;
        std::string handshake_buffer_;

        // 写入队列
        std::vector<std::vector<uint8_t>> write_queue_;
        bool is_writing_;
        bool needs_close_; // 标记需要在写入完成后关闭连接

        // 批量发送优化
        std::vector<std::vector<uint8_t>> pending_writes_;
        static constexpr int BATCH_SIZE = 4; // 批处理大小

        // 命令分发器
        CommandDispatcher command_dispatcher_;

        // 外部资源引用
        Database *db_;
        AIClient *ai_client_;
        UserManager *user_manager_;
        RoomManager *room_manager_;
        PresetManager *preset_manager_;
        ConversationManager *conversation_manager_;
        MessageHistoryManager *history_manager_;
        FriendManager *friend_manager_;

        // 聊天室
        std::string current_chat_room_;

        // 静态房间列表（阶段 4.1 简单实现）
        static std::set<std::string> created_rooms_;

        // 静态连接列表（用于广播）
        static std::set<ConnectionPtr> all_connections_;
    };

    using ConnectionPtr = std::shared_ptr<Connection>;

} // namespace chat
