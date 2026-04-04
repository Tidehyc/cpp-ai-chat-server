#include "connection.h"
#include "protocol/json_parser.h"
#include "storage/database.h"
#include "ai/ai_client.h"
#include "user/user_manager.h"
#include "room/room_manager.h"
#include "preset/preset_manager.h"
#include "conversation/conversation.h"
#include "history/message_history.h"
#include "friend/friend_manager.h"
#include "utils/logger.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <thread>
#include <regex>

namespace chat {

static uint64_t g_connection_id = 0;

// 静态房间列表（阶段 4.1 简单实现）
std::set<std::string> Connection::created_rooms_;

// 静态连接列表（用于广播）
std::set<ConnectionPtr> Connection::all_connections_;

Connection::Connection(boost::asio::ip::tcp::socket socket,
                       Database* db, AIClient* ai_client,
                       UserManager* user_manager, RoomManager* room_manager, PresetManager* preset_manager,
                       ConversationManager* conversation_manager, MessageHistoryManager* history_manager,
                       FriendManager* friend_manager)
    : socket_(std::move(socket))
    , state_(ConnectionState::HANDSHAKE)
    , id_(++g_connection_id)
    , is_writing_(false)
    , db_(db)
    , ai_client_(ai_client)
    , user_manager_(user_manager)
    , room_manager_(room_manager)
    , preset_manager_(preset_manager)
    , conversation_manager_(conversation_manager)
    , history_manager_(history_manager)
    , friend_manager_(friend_manager) {
    // 预分配缓冲区，减少内存分配
    read_buffer_.resize(8192);  // 8KB 缓冲区
}

Connection::~Connection() {
    Logger::debug("Connection " + std::to_string(id_) + " destroyed");
    
    // 如果已登录且状态不是 offline，更新为 offline
    if (session_.is_authenticated()) {
        user_manager_->update_status(session_.user_id, "offline");
    }
}

void Connection::start() {
    Logger::info("Connection " + std::to_string(id_) + " started");
    
    // 添加到连接列表
    all_connections_.insert(shared_from_this());
    
    do_read_handshake();
}

void Connection::send(const std::string& message) {
    if (state_ == ConnectionState::CLOSED) {
        return;
    }
    
    auto frame = WebSocketParser::encode(message);
    write_queue_.push_back(frame);
    
    if (!is_writing_) {
        do_write();
    }
}

void Connection::close() {
    if (state_ == ConnectionState::CLOSED) return;

    state_ = ConnectionState::CLOSING;

    Logger::info("Connection " + std::to_string(id_) + " closing, user_id=" + std::to_string(session_.user_id) + ", username=" + session_.username);

    // 如果已登录，标记为离线并通知好友
    if (session_.is_authenticated()) {
        user_manager_->set_user_online(session_.user_id, false);
        user_manager_->update_status(session_.user_id, "offline");  // 更新数据库状态为 offline

        // 获取用户的好友列表并通知他们
        auto friends = friend_manager_->get_friends(session_.user_id);
        Logger::info("Connection " + std::to_string(id_) + " has " + std::to_string(friends.size()) + " friends to notify");
        
        for (const auto& f : friends) {
            Logger::info("Notifying friend_id=" + std::to_string(f.friend_id) + ", username=" + f.friend_username);
            for (auto& conn : all_connections_) {
                if (conn && conn->session_.user_id == f.friend_id) {
                    // 发送好友下线通知
                    std::string json = "{\"type\":\"friend_offline\",\"username\":\"" + session_.username + "\",\"status\":\"offline\"}";
                    Logger::info("Sending offline notification: " + json);
                    conn->send(json);
                    break;
                }
            }
        }
    }

    // 从连接列表中移除
    all_connections_.erase(shared_from_this());

    auto frame = WebSocketParser::create_close_frame(1000);
    write_queue_.push_back(frame);

    if (!is_writing_) {
        do_write();
    }
}

void Connection::send_system_message(const std::string& content) {
    Message msg;
    msg.type = "system";
    msg.content = content;
    send(JsonParser::stringify(msg));
}

void Connection::do_read_handshake() {
    auto self = shared_from_this();
    boost::asio::async_read_until(
        socket_,
        boost::asio::dynamic_buffer(handshake_buffer_),
        "\r\n\r\n",
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (ec) {
                Logger::error("Connection " + std::to_string(id_) + " handshake read error");
                close();
                return;
            }
            
            if (!WebSocketParser::is_handshake_request(handshake_buffer_)) {
                Logger::error("Connection " + std::to_string(id_) + " not a WebSocket handshake");
                close();
                return;
            }
            
            std::string key = WebSocketParser::extract_websocket_key(handshake_buffer_);
            if (key.empty()) {
                Logger::error("Connection " + std::to_string(id_) + " missing WebSocket key");
                close();
                return;
            }
            
            std::string response = WebSocketParser::create_handshake_response(key);
            
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(response),
                [this, self](boost::system::error_code ec, std::size_t) {
                    if (ec) {
                        Logger::error("Connection " + std::to_string(id_) + " handshake write error");
                        close();
                        return;
                    }
                    
                    Logger::info("Connection " + std::to_string(id_) + " WebSocket handshake complete");
                    state_ = ConnectionState::AUTHENTICATED;
                    handshake_buffer_.clear();
                    
                    // 发送简化的欢迎消息
                    send_system_message("欢迎！输入 /help 查看命令，或 /login <用户名> <密码> 登录。");
                    
                    do_read_websocket();
                }
            );
        }
    );
}

void Connection::do_read_websocket() {
    if (state_ == ConnectionState::CLOSED) return;
    
    auto self = shared_from_this();
    socket_.async_read_some(
        boost::asio::buffer(read_buffer_),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (ec) {
                Logger::debug("Connection " + std::to_string(id_) + " read error: " + ec.message());
                close();
                return;
            }
            
            WebSocketFrame frame;
            std::vector<uint8_t> data(read_buffer_.begin(), read_buffer_.begin() + length);
            
            WSResult result = WebSocketParser::parse(data, frame);
            
            if (result == WSResult::SUCCESS) {
                handle_websocket_frame(frame);
                do_read_websocket();
            } else if (result == WSResult::INCOMPLETE) {
                do_read_websocket();
            } else if (result == WSResult::ERROR_CLOSE) {
                Logger::info("Connection " + std::to_string(id_) + " received close frame");
                close();
            } else {
                Logger::error("Connection " + std::to_string(id_) + " parse error");
                close();
            }
        }
    );
}

void Connection::do_write() {
    if (is_writing_ || write_queue_.empty()) return;

    is_writing_ = true;
    auto self = shared_from_this();

    const auto& data = write_queue_[0];
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(data),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec) {
                Logger::error("Connection " + std::to_string(id_) + " write error");
                close();
                return;
            }

            write_queue_.erase(write_queue_.begin());
            is_writing_ = false;

            if (state_ == ConnectionState::CLOSING) {
                state_ = ConnectionState::CLOSED;
                socket_.close();
            } else if (!write_queue_.empty()) {
                do_write();
            }
        }
    );
}

void Connection::handle_websocket_frame(const WebSocketFrame& frame) {
    switch (frame.opcode) {
        case Opcode::TEXT: {
            std::string message = WebSocketParser::decode(frame);
            Logger::debug("Connection " + std::to_string(id_) + " received: " + message);
            
            // 直接处理为命令/消息
            handle_command(message);
            break;
        }
        
        case Opcode::PING: {
            auto pong = WebSocketParser::create_pong_frame();
            write_queue_.push_back(pong);
            if (!is_writing_) {
                do_write();
            }
            break;
        }
        
        case Opcode::PONG:
            break;
            
        default:
            Logger::debug("Connection " + std::to_string(id_) + " unknown opcode");
            break;
    }
}

void Connection::on_message(const Message& msg) {
    Logger::info("Connection " + std::to_string(id_) + " processing message type: " + msg.type);
    
    // 简单回复确认
    Message response;
    response.type = "system";
    response.content = "Message received: " + msg.content;
    send(JsonParser::stringify(response));
}

void Connection::handle_command(const std::string& input) {
    Logger::info("Connection " + std::to_string(id_) + " command: " + input);

    if (input.empty()) return;

    // 尝试解析 JSON
    try {
        Message msg = JsonParser::parse(input);
        // 如果是 JSON 消息，提取 content 作为命令
        if (!msg.content.empty()) {
            handle_command(msg.content);  // 递归处理 content
            return;
        }
    } catch (...) {
        // 不是 JSON，继续处理为命令
    }

    if (input[0] == '/') {
        // 解析命令
        size_t space_pos = input.find(' ');
        std::string cmd_name = (space_pos != std::string::npos) ? input.substr(1, space_pos - 1) : input.substr(1);
        std::string args_str = (space_pos != std::string::npos) ? input.substr(space_pos + 1) : "";

        // 分割参数
        std::vector<std::string> args_vec;
        if (!args_str.empty()) {
            std::istringstream iss(args_str);
            std::string arg;
            while (iss >> arg) {
                args_vec.push_back(arg);
            }
        }

        // 处理命令
        if (cmd_name == "register" || cmd_name == "reg") {
            cmd_register(args_vec);
        } else if (cmd_name == "login" || cmd_name == "log") {
            cmd_login(args_vec);
        } else if (cmd_name == "users") {
            cmd_users(args_vec);
        } else if (cmd_name == "status") {
            cmd_status(args_vec);
        } else if (cmd_name == "rooms") {
            cmd_rooms(args_vec);
        } else if (cmd_name == "rooms_json") {
            send_room_list_json();
        } else if (cmd_name == "join") {
            cmd_join(args_vec);
        } else if (cmd_name == "leave") {
            cmd_leave(args_vec);
        } else if (cmd_name == "create") {
            cmd_create(args_vec);
        } else if (cmd_name == "list") {
            cmd_list(args_vec);
        } else if (cmd_name == "msg") {
            cmd_msg(args_vec);
        } else if (cmd_name == "preset") {
            cmd_preset(args_vec);
        } else if (cmd_name == "clear") {
            cmd_clear(args_vec);
        } else if (cmd_name == "context") {
            cmd_context(args_vec);
        } else if (cmd_name == "history") {
            cmd_history(args_vec);
        } else if (cmd_name == "search") {
            cmd_search(args_vec);
        } else if (cmd_name == "export") {
            cmd_export(args_vec);
        } else if (cmd_name == "friend") {
            cmd_friend(args_vec);
        } else if (cmd_name == "pm") {
            cmd_private_message(args_vec);
        } else if (cmd_name == "help" || cmd_name == "h") {
            cmd_help(args_vec);
        } else if (cmd_name == "quit" || cmd_name == "exit") {
            // 退出前更新状态
            if (session_.is_authenticated()) {
                user_manager_->update_status(session_.user_id, "offline");
            }
            close();
        } else {
            // 未知命令，当作聊天消息处理
            handle_chat_message(input);
        }
    } else {
        // 不是命令，当作聊天消息
        handle_chat_message(input);
    }
}

void Connection::handle_chat_message(const std::string& content) {
    Logger::info("handle_chat_message called, user: " + session_.username + ", room: " + session_.current_room + ", content: " + content);
    
    if (!session_.is_authenticated()) {
        Logger::info("User not authenticated, sending login prompt");
        send_system_message("请先登录。使用 /register <用户名> <密码> 注册，或 /login <用户名> <密码> 登录。");
        return;
    }

    // 检查是否包含 @AI
    if (content.find("@AI") != std::string::npos || content.find("@ai") != std::string::npos) {
        // 提取 @AI 后面的内容
        std::string ai_content = content;
        size_t pos = content.find("@AI");
        if (pos == std::string::npos) {
            pos = content.find("@ai");
        }
        if (pos != std::string::npos) {
            ai_content = content.substr(pos + 3);
            // 去除前导空格
            while (!ai_content.empty() && ai_content[0] == ' ') {
                ai_content = ai_content.substr(1);
            }
        }

        std::string ai_prompt = get_preset_ai_prompt(session_.current_preset);
        
        // 获取对话上下文
        std::vector<AIMessage> context;
        if (conversation_manager_) {
            auto& ctx = conversation_manager_->get_context(session_.current_room, std::to_string(session_.user_id));
            for (const auto& msg : ctx.get_recent_messages(ctx.max_history)) {
                context.push_back(AIMessage{msg.role, msg.content});
            }
            // 保存用户消息到上下文
            conversation_manager_->add_message(session_.current_room, std::to_string(session_.user_id), "user", ai_content);
        }
        
        send_ai_response(ai_prompt, ai_content, context);
        
        // 保存用户消息到数据库
        if (db_) {
            Message user_msg;
            user_msg.type = "text";
            user_msg.from = session_.username;
            user_msg.to = "AI";
            user_msg.room = session_.current_room;
            user_msg.content = ai_content;
            user_msg.timestamp = std::time(nullptr);
            db_->save_message(user_msg);
        }
        
        return;
    }

    // 普通消息：广播给同一房间的其他用户
    broadcast_to_room(content);
    
    // 保存普通消息到数据库
    if (db_) {
        Message msg;
        msg.type = "text";
        msg.from = session_.username;
        msg.to = session_.current_room;
        msg.room = session_.current_room;
        msg.content = content;
        msg.timestamp = std::time(nullptr);
        db_->save_message(msg);
    }
}

void Connection::broadcast_to_room(const std::string& message) {
    Logger::info("=== Broadcast Debug ===");
    Logger::info("Current user: " + session_.username + ", current_room: " + session_.current_room);
    Logger::info("Message: " + message);
    Logger::info("Total connections: " + std::to_string(all_connections_.size()));
    
    // 构建 JSON 格式的房间消息
    std::string json = "{\"type\":\"room_message\",\"from\":\"" + session_.username + "\",\"room\":\"" + session_.current_room + "\",\"content\":\"" + message + "\",\"timestamp\":" + std::to_string(std::time(nullptr)) + "}";
    
    Logger::info("Broadcast JSON: " + json);

    int sent_count = 0;
    int total_count = 0;
    // 广播给同一房间的所有其他用户
    for (auto& conn : all_connections_) {
        total_count++;
        Logger::info("Checking connection " + std::to_string(total_count) + ": user=" + conn->session_.username + ", room=" + conn->session_.current_room + ", authenticated=" + std::to_string(conn->session_.is_authenticated()));
        
        if (conn &&
            conn.get() != this &&
            conn->session_.current_room == session_.current_room &&
            conn->session_.is_authenticated()) {
            Logger::info(">>> Sending to user: " + conn->session_.username);
            conn->send(json);
            sent_count++;
        } else {
            Logger::info(">>> Skipping user: " + conn->session_.username + " (room mismatch or not authenticated or self)");
        }
    }
    
    Logger::info("Broadcast sent to " + std::to_string(sent_count) + " / " + std::to_string(total_count - 1) + " other users");
    Logger::info("======================");
    
    // 自己也收到消息
    send_system_message("[你]: " + message);
}

// ========== 用户命令 ==========

void Connection::cmd_register(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        send_system_message("用法：/register <用户名> <密码>");
        return;
    }
    
    std::string username = args[0];
    std::string password = args[1];
    
    if (password.length() < 6) {
        send_system_message("密码长度至少为 6 位");
        return;
    }
    
    if (user_manager_->register_user(username, password)) {
        send_system_message("注册成功！请使用 /login " + username + " <密码> 登录。");
    } else {
        send_system_message("注册失败：用户名可能已存在。");
    }
}

void Connection::cmd_login(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        send_system_message("用法：/login <用户名> <密码>");
        return;
    }
    
    std::string username = args[0];
    std::string password = args[1];

    User user;
    LoginResult result = user_manager_->login(username, password, user);
    
    if (result == LoginResult::SUCCESS) {
        // 检查用户是否已经在其他地方登录
        bool was_online = user_manager_->is_online(user.id);

        session_.user_id = user.id;
        session_.username = user.username;
        session_.current_preset = user.current_preset;
        session_.status = "online";
        state_ = ConnectionState::ACTIVE;
        user_manager_->set_user_online(user.id, true);
        user_manager_->update_status(user.id, "online");

        if (was_online) {
            send_system_message("提示：该账号之前已在线，现在已更新为新会话。");
        }

        send_system_message("登录成功！欢迎 " + username + "。输入 /help 查看命令。");

        // 自动获取好友列表
        cmd_friend({"list"});

        // 检查并推送待处理的好友请求
        auto pending_requests = friend_manager_->get_received_requests(user.id);
        if (!pending_requests.empty()) {
            send_system_message("📬 你有 " + std::to_string(pending_requests.size()) + " 条待处理的好友请求。使用 /friend requests 查看。");
            for (const auto& req : pending_requests) {
                send_system_message("📬 " + req.sender_username + " 向你发送了好友请求。使用 /friend accept " + req.sender_username + " 同意。");
            }
        }

        // 推送离线私聊消息
        send_offline_messages();

        // 通知好友上线
        auto friends = friend_manager_->get_friends(user.id);
        for (const auto& f : friends) {
            for (auto& conn : all_connections_) {
                if (conn && conn->session_.user_id == f.friend_id) {
                    std::string json = "{\"type\":\"friend_online\",\"username\":\"" + user.username + "\",\"status\":\"online\"}";
                    conn->send(json);
                    break;
                }
            }
        }
    } else if (result == LoginResult::USER_NOT_FOUND) {
        send_system_message("登录失败：用户不存在。");
    } else {
        send_system_message("登录失败：密码错误。");
    }
}

void Connection::cmd_users(const std::vector<std::string>& /*args*/) {
    auto users = user_manager_->get_all_users();
    
    if (users.empty()) {
        send_system_message("暂无用户。");
        return;
    }
    
    std::string result = "用户列表：" + std::to_string(users.size()) + " 人\n";
    for (const auto& u : users) {
        std::string status_icon;
        if (u.status == "online") {
            status_icon = "🟢";  // 在线
        } else if (u.status == "busy") {
            status_icon = "🔴";  // 忙碌
        } else if (u.status == "away") {
            status_icon = "🟡";  // 离开
        } else {
            status_icon = "⚪";  // 离线
        }
        result += "  " + status_icon + " " + u.username + " [" + u.status + "]\n";
    }
    send_system_message(result);
}

void Connection::cmd_status(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }
    
    if (args.empty()) {
        // 查看当前状态
        send_system_message("当前状态：" + session_.status);
        send_system_message("可用状态：online, offline, busy, away");
    } else {
        // 设置状态
        std::string new_status = args[0];
        if (new_status == "online" || new_status == "offline" || 
            new_status == "busy" || new_status == "away") {
            session_.status = new_status;
            user_manager_->update_status(session_.user_id, new_status);
            send_system_message("状态已更新为：" + new_status);
        } else {
            send_system_message("无效状态。可用状态：online, offline, busy, away");
        }
    }
}

// ========== 房间命令 ==========

void Connection::cmd_rooms(const std::vector<std::string>& /*args*/) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }

    send_system_message("=== AI 房间 ===");
    send_system_message("#general - 友好助手");
    send_system_message("#code - 编程助手");
    send_system_message("#translate - 翻译");
    send_system_message("#learn - 学习");
    send_system_message("#write - 写作");
    send_system_message("#debug - 调试");
    send_system_message("");
    send_system_message("提示：使用 /join <房间名> 加入 AI 房间");
}

void Connection::cmd_join(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }

    if (args.empty()) {
        send_system_message("用法：/join <房间名>");
        return;
    }

    std::string room_name = args[0];
    // 确保房间名以 # 开头，保持全局一致性
    if (room_name.empty() || room_name[0] != '#') {
        room_name = "#" + room_name;
    }

    Logger::info("User " + session_.username + " joining room: " + room_name);
    session_.current_room = room_name;
    Logger::info("User " + session_.username + " current_room set to: " + session_.current_room);

    send_system_message("已加入房间 " + room_name);
}

void Connection::cmd_leave(const std::vector<std::string>& /*args*/) {
    session_.current_room = "general";
    send_system_message("已离开当前房间，返回 #general。");
}

// ========== 群聊命令 ==========

void Connection::cmd_create(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }

    if (args.empty()) {
        send_system_message("用法：/create room <房间名>");
        return;
    }

    if (args[0] != "room") {
        send_system_message("用法：/create room <房间名>");
        return;
    }

    if (args.size() < 2) {
        send_system_message("用法：/create room <房间名>");
        return;
    }

    std::string room_name = args[1];
    // 确保房间名以 # 开头，保持全局一致性
    if (room_name.empty() || room_name[0] != '#') {
        room_name = "#" + room_name;
    }
    
    // 添加到房间列表
    created_rooms_.insert(room_name);
    // 加入房间
    session_.current_room = room_name;
    send_system_message("已创建并加入房间 " + room_name);
    send_system_message("提示：使用 /list 查看房间，/leave 离开房间");
    
    // 广播房间列表更新给所有连接
    broadcast_room_list();
}

void Connection::send_room_list_json() {
    std::string json = "{\"type\":\"room_list_update\",\"rooms\":[";
    bool first = true;
    
    // 添加预定义房间
    std::vector<std::pair<std::string, std::string>> default_rooms = {
        {"#general", "🏠"}, {"#code", "💻"}, {"#gaming", "🎮"}, 
        {"#music", "🎵"}, {"#movie", "🎬"}
    };
    
    for (const auto& room : default_rooms) {
        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"" + room.first + "\",\"icon\":\"" + room.second + "\",\"description\":\"预定义房间\",\"isCustom\":false}";
    }
    
    // 添加用户创建的房间 (room 已经包含 #)
    for (const auto& room : created_rooms_) {
        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"" + room + "\",\"icon\":\"💬\",\"description\":\"用户创建\",\"isCustom\":true}";
    }
    
    json += "]}";
    send(json);
}

void Connection::broadcast_room_list() {
    Logger::info("=== Broadcast Room List ===");
    Logger::info("Total connections: " + std::to_string(all_connections_.size()));
    
    std::string json = "{\"type\":\"room_list_update\",\"rooms\":[";
    bool first = true;
    
    // 添加预定义房间
    std::vector<std::pair<std::string, std::string>> default_rooms = {
        {"#general", "🏠"}, {"#code", "💻"}, {"#gaming", "🎮"}, 
        {"#music", "🎵"}, {"#movie", "🎬"}
    };
    
    for (const auto& room : default_rooms) {
        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"" + room.first + "\",\"icon\":\"" + room.second + "\",\"description\":\"预定义房间\",\"isCustom\":false}";
    }
    
    // 添加用户创建的房间 (room 已经包含 #)
    for (const auto& room : created_rooms_) {
        if (!first) json += ",";
        first = false;
        json += "{\"name\":\"" + room + "\",\"icon\":\"💬\",\"description\":\"用户创建\",\"isCustom\":true}";
    }
    
    json += "]}";
    
    int sent_count = 0;
    // 广播给所有连接
    for (auto& conn : all_connections_) {
        if (conn && conn->session_.is_authenticated()) {
            Logger::info("Sending room list to user: " + conn->session_.username);
            conn->send(json);
            sent_count++;
        }
    }
    Logger::info("Room list sent to " + std::to_string(sent_count) + " users.");
    Logger::info("===========================");
}

void Connection::cmd_list(const std::vector<std::string>& /*args*/) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }

    send_system_message("=== 群聊房间 ===");

    // 显示用户创建的房间
    if (!created_rooms_.empty()) {
        for (const auto& room : created_rooms_) {
            std::string marker = (room == session_.current_room) ? " [当前]" : "";
            send_system_message("#" + room + marker);
        }
    } else {
        send_system_message("（暂无用户创建的房间）");
    }

    send_system_message("");
    send_system_message("当前房间：" + session_.current_room);
    send_system_message("");
    send_system_message("提示：");
    send_system_message("/create room <名称> - 创建房间");
    send_system_message("/join <房间名>      - 加入房间");
    send_system_message("/rooms             - 查看 AI 房间");
}

// ========== AI 命令 ==========

void Connection::cmd_msg(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        send_system_message("用法：/msg ai <消息内容>");
        return;
    }
    
    if (args[0] != "ai") {
        send_system_message("目前只支持 /msg ai <消息>");
        return;
    }
    
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }
    
    // 拼接消息内容
    std::string message = args[1];
    for (size_t i = 2; i < args.size(); ++i) {
        message += " " + args[i];
    }

    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }

    std::string ai_prompt = get_preset_ai_prompt(session_.current_preset);
    
    // 获取对话上下文
    std::vector<AIMessage> context;
    if (conversation_manager_) {
        auto& ctx = conversation_manager_->get_context(session_.current_room, std::to_string(session_.user_id));
        for (const auto& msg : ctx.get_recent_messages(ctx.max_history)) {
            context.push_back(AIMessage{msg.role, msg.content});
        }
        // 保存用户消息到上下文
        conversation_manager_->add_message(session_.current_room, std::to_string(session_.user_id), "user", message);
    }
    
    send_ai_response(ai_prompt, message, context);
}

void Connection::cmd_preset(const std::vector<std::string>& args) {
    if (args.empty()) {
        send_system_message("用法：/preset list | /preset use <名称>");
        return;
    }

    if (args[0] == "list") {
        auto presets = preset_manager_->get_all_presets(session_.user_id);
        send_system_message("可用预设：");
        for (const auto& p : presets) {
            std::string marker = (p.name == session_.current_preset) ? " [当前]" : "";
            send_system_message("  - " + p.name + marker);
        }
    } else if (args[0] == "use") {
        if (args.size() < 2) {
            send_system_message("用法：/preset use <名称>");
            return;
        }
        std::string preset_name = args[1];
        Preset preset = preset_manager_->get_preset(session_.user_id, preset_name);
        if (preset.is_valid()) {
            session_.current_preset = preset_name;
            user_manager_->update_preset(session_.user_id, preset_name);
            send_system_message("已切换预设为：" + preset_name);
        } else {
            send_system_message("预设不存在：" + preset_name);
        }
    } else {
        send_system_message("未知命令。用法：/preset list | /preset use <名称>");
    }
}

// ========== 系统命令 ==========

void Connection::cmd_help(const std::vector<std::string>& /*args*/) {
    // 分条发送帮助信息
    send_system_message("=== 用户命令 ===");
    send_system_message("/register <用户名> <密码> - 注册账号");
    send_system_message("/login <用户名> <密码>   - 登录");
    send_system_message("/users                 - 查看所有用户");
    send_system_message("/status [状态]          - 设置/查看状态");
    
    send_system_message("=== AI 房间 ===");
    send_system_message("/rooms                - 列出 AI 房间");
    send_system_message("/join <房间名>         - 加入房间");
    send_system_message("/leave                - 离开房间");
    
    send_system_message("=== 群聊房间 ===");
    send_system_message("/list                 - 列出群聊房间");
    send_system_message("/create room <名称>    - 创建房间");
    
    send_system_message("=== AI 对话 ===");
    send_system_message("/msg ai <消息>       - 私聊 AI");
    send_system_message("/preset list          - 列出预设");
    send_system_message("/preset use <名称>     - 使用预设");
    send_system_message("/clear                - 清空对话历史");
    send_system_message("/context              - 查看上下文");
    send_system_message("/context set <轮数>    - 设置上下文大小");
    send_system_message("@AI <消息>            - 在聊天中@AI");
    
    send_system_message("=== 历史记录 ===");
    send_system_message("/history <目标> [数量]  - 查看历史消息");
    send_system_message("/search <关键词>       - 搜索消息");
    send_system_message("/export <目标>         - 导出聊天记录");

    send_system_message("=== 系统命令 ===");
    send_system_message("/help                 - 显示帮助");
    send_system_message("/quit                 - 退出");
}

// ========== 上下文命令 ==========

void Connection::cmd_clear(const std::vector<std::string>& /*args*/) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }
    
    if (conversation_manager_) {
        conversation_manager_->clear_context(session_.current_room, std::to_string(session_.user_id));
        send_system_message("对话历史已清空。");
    }
}

void Connection::cmd_context(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }
    
    if (conversation_manager_) {
        auto& ctx = conversation_manager_->get_context(session_.current_room, std::to_string(session_.user_id));
        
        if (args.empty()) {
            // 查看当前上下文大小
            send_system_message("当前上下文大小：" + std::to_string(ctx.history.size()) + " 条消息");
            send_system_message("最大历史：" + std::to_string(ctx.max_history) + " 轮");
        } else if (args[0] == "set") {
            // 设置上下文大小
            if (args.size() < 2) {
                send_system_message("用法：/context set <轮数>");
                return;
            }
            try {
                int new_max = std::stoi(args[1]);
                if (new_max > 0 && new_max <= 50) {
                    ctx.max_history = new_max;
                    send_system_message("已设置最大历史为 " + std::to_string(new_max) + " 轮");
                } else {
                    send_system_message("轮数必须在 1-50 之间");
                }
            } catch (...) {
                send_system_message("无效的轮数");
            }
        } else {
            send_system_message("用法：/context [set <轮数>]");
        }
    }
}

// ========== 历史命令 ==========

void Connection::cmd_history(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }
    
    if (args.empty()) {
        send_system_message("用法：/history <目标> [数量]");
        send_system_message("示例：/history @ai 10  查看与 AI 的最近 10 条消息");
        send_system_message("       /history #code 20  查看 code 房间最近 20 条消息");
        return;
    }
    
    std::string target = args[0];
    int limit = 20;
    if (args.size() > 1) {
        try {
            limit = std::stoi(args[1]);
            if (limit <= 0 || limit > 100) {
                send_system_message("数量必须在 1-100 之间");
                return;
            }
        } catch (...) {
            send_system_message("无效的数量");
            return;
        }
    }
    
    bool is_room = (target[0] == '#');
    
    if (history_manager_) {
        auto messages = history_manager_->get_history(target, is_room, limit);
        
        if (messages.empty()) {
            send_system_message("没有找到历史记录。");
            return;
        }
        
        send_system_message("=== 历史记录：" + target + " ===");
        for (const auto& msg : messages) {
            char time_buf[64];
            std::time_t t = msg.timestamp;
            std::strftime(time_buf, sizeof(time_buf), "%m-%d %H:%M", std::localtime(&t));
            std::string content = msg.content.substr(0, 50);
            if (msg.content.length() > 50) content += "...";
            
            // 显示发送者和房间
            std::string prefix = msg.from_user;
            if (!msg.room_name.empty()) {
                prefix = "#" + msg.room_name + " " + prefix;
            }
            
            send_system_message(std::string("[") + time_buf + "] " + prefix + ": " + content);
        }
    }
}

void Connection::cmd_search(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }
    
    if (args.empty()) {
        send_system_message("用法：/search <关键词> [from:用户] [in:房间]");
        send_system_message("示例：/search 快速排序");
        send_system_message("       /search from:alice");
        send_system_message("       /search 快速排序 in:#code");
        return;
    }
    
    SearchCriteria criteria;
    criteria.keyword = args[0];
    criteria.limit = 20;
    
    // 解析额外参数
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i].substr(0, 5) == "from:") {
            criteria.from_user = args[i].substr(5);
        } else if (args[i].substr(0, 3) == "in:") {
            criteria.room_name = args[i].substr(3);
        }
    }
    
    if (history_manager_) {
        auto messages = history_manager_->search(criteria);

        if (messages.empty()) {
            send_system_message("没有找到匹配的消息。");
            return;
        }

        send_system_message("=== 搜索结果：" + criteria.keyword + " ===");
        send_system_message("找到 " + std::to_string(messages.size()) + " 条消息：");
        for (const auto& msg : messages) {
            char time_buf[64];
            std::time_t t = msg.timestamp;
            std::strftime(time_buf, sizeof(time_buf), "%m-%d %H:%M", std::localtime(&t));
            
            // 严格过滤内容，只保留安全的可打印字符
            std::string safe_content;
            for (unsigned char c : msg.content) {
                // 保留可打印 ASCII (32-126) 和常见中文字符范围
                if (c >= 32 && c < 127) {
                    safe_content += c;
                } else if (c >= 0x80) {
                    // UTF-8 字符，直接保留
                    safe_content += c;
                }
                // 其他字符（换行、制表符等）替换为空格
                if (safe_content.length() >= 60) {
                    safe_content += "...";
                    break;
                }
            }
            
            send_system_message("[" + std::string(time_buf) + "] #" + msg.room_name + " " + msg.from_user + ": " + safe_content);
        }
    }
}

void Connection::cmd_export(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }
    
    if (args.empty()) {
        send_system_message("用法：/export <目标> [format=txt|json]");
        send_system_message("示例：/export @ai");
        send_system_message("       /export #code format=json");
        return;
    }
    
    std::string target = args[0];
    std::string format = "txt";
    
    // 解析格式参数
    for (const auto& arg : args) {
        if (arg.substr(0, 7) == "format=") {
            format = arg.substr(7);
        }
    }
    
    if (format != "txt" && format != "json") {
        send_system_message("格式必须是 txt 或 json");
        return;
    }
    
    bool is_room = (target[0] == '#');
    
    if (history_manager_) {
        std::string content = history_manager_->export_messages(target, is_room, format);
        
        // 分条发送（避免消息太长）
        size_t pos = 0;
        size_t max_len = 400;
        while (pos < content.length()) {
            std::string chunk = content.substr(pos, std::min(max_len, content.length() - pos));
            send_system_message(chunk);
            pos += max_len;
            if (pos < content.length()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }
}

// ========== 工具方法 ==========

std::string Connection::get_room_ai_prompt(const std::string& room_name) {
    Room room = room_manager_->get_room(room_name);
    if (room.is_valid()) {
        return room.ai_prompt;
    }
    return "你是友好的 AI 助手。";
}

// ========== 好友命令 ==========

void Connection::cmd_friend(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }

    if (args.empty()) {
        send_system_message("用法：/friend <list|request|accept|reject|del> [用户名]");
        send_system_message("  /friend list              - 查看好友列表");
        send_system_message("  /friend request <用户名>   - 发送好友请求");
        send_system_message("  /friend requests          - 查看收到的好友请求");
        send_system_message("  /friend accept <用户名>   - 同意好友请求");
        send_system_message("  /friend reject <用户名>   - 拒绝好友请求");
        send_system_message("  /friend del <用户名>      - 删除好友");
        return;
    }

    std::string subcmd = args[0];

    if (subcmd == "list") {
        // 查看好友列表
        auto friends = friend_manager_->get_friends(session_.user_id);
        
        if (friends.empty()) {
            send_system_message("暂无好友。使用 /friend request <用户名> 添加好友。");
            return;
        }

        std::string result = "好友列表：" + std::to_string(friends.size()) + " 人\n";
        for (const auto& f : friends) {
            // 获取好友状态
            int friend_user_id = f.friend_id;
            bool is_online = user_manager_->is_online(friend_user_id);
            std::string status = is_online ? user_manager_->get_user_status(friend_user_id) : "offline";
            std::string status_icon = "⚪";
            if (status == "online") status_icon = "🟢";
            else if (status == "busy") status_icon = "🔴";
            else if (status == "away") status_icon = "🟡";
            
            result += "  " + status_icon + " " + f.friend_username + " [" + status + "]\n";
        }
        send_system_message(result);

        // 同时发送 JSON 格式的好友列表给前端
        std::string json = "{\"type\":\"friend_list\",\"friends\":[";
        bool first = true;
        for (const auto& f : friends) {
            if (!first) json += ",";
            first = false;
            int friend_user_id = f.friend_id;
            bool is_online = user_manager_->is_online(friend_user_id);
            std::string status = is_online ? user_manager_->get_user_status(friend_user_id) : "offline";
            json += "{\"username\":\"" + f.friend_username + "\",\"status\":\"" + status + "\"}";
        }
        json += "]}";
        send(json);

    } else if (subcmd == "request") {
        // 发送好友请求
        if (args.size() < 2) {
            send_system_message("用法：/friend request <用户名>");
            return;
        }

        std::string target_username = args[1];
        int target_user_id = friend_manager_->get_user_id_by_name(target_username);

        if (target_user_id < 0) {
            send_system_message("用户不存在：" + target_username);
            return;
        }

        if (target_user_id == session_.user_id) {
            send_system_message("不能添加自己为好友。");
            return;
        }

        // 检查是否已经是好友
        if (friend_manager_->is_friend(session_.user_id, target_user_id)) {
            send_system_message("你们已经是好友了。");
            return;
        }

        // 发送好友请求
        if (friend_manager_->send_request(session_.user_id, target_user_id)) {
            send_system_message("好友请求已发送给 " + target_username + "。");

            // 通知目标用户
            for (auto& conn : all_connections_) {
                if (conn && conn->session_.user_id == target_user_id) {
                    conn->send_system_message("📬 " + session_.username + " 向你发送了好友请求。使用 /friend requests 查看。");
                    break;
                }
            }
        } else {
            send_system_message("发送失败，你可能已经发送过好友请求。");
        }

    } else if (subcmd == "requests") {
        // 查看收到的好友请求
        auto requests = friend_manager_->get_received_requests(session_.user_id);

        if (requests.empty()) {
            send_system_message("暂无待处理的好友请求。");
            return;
        }

        std::string result = "待处理好友请求：" + std::to_string(requests.size()) + " 条\n";
        for (const auto& r : requests) {
            result += "  " + r.sender_username + " 请求添加你为好友\n";
        }
        result += "\n使用 /friend accept <用户名> 同意，/friend reject <用户名> 拒绝";
        send_system_message(result);

    } else if (subcmd == "accept") {
        // 同意好友请求
        if (args.size() < 2) {
            send_system_message("用法：/friend accept <用户名>");
            return;
        }

        std::string sender_username = args[1];
        int sender_user_id = friend_manager_->get_user_id_by_name(sender_username);

        if (sender_user_id < 0) {
            send_system_message("用户不存在：" + sender_username);
            return;
        }

        if (friend_manager_->accept_request(sender_user_id, session_.user_id)) {
            send_system_message("你已同意 " + sender_username + " 的好友请求。你们现在是好友了！");

            // 通知请求方
            for (auto& conn : all_connections_) {
                if (conn && conn->session_.user_id == sender_user_id) {
                    conn->send_system_message("✅ " + session_.username + " 接受了你的好友请求。你们现在是好友了！");
                    break;
                }
            }

            // 刷新双方好友列表
            cmd_friend({"list"});
            
            // 通知请求方刷新好友列表
            for (auto& conn : all_connections_) {
                if (conn && conn->session_.user_id == sender_user_id) {
                    conn->cmd_friend({"list"});
                    break;
                }
            }
        } else {
            send_system_message("操作失败，可能没有来自该用户的好友请求。");
        }

    } else if (subcmd == "reject") {
        // 拒绝好友请求
        if (args.size() < 2) {
            send_system_message("用法：/friend reject <用户名>");
            return;
        }

        std::string sender_username = args[1];
        int sender_user_id = friend_manager_->get_user_id_by_name(sender_username);

        if (sender_user_id < 0) {
            send_system_message("用户不存在：" + sender_username);
            return;
        }

        if (friend_manager_->reject_request(sender_user_id, session_.user_id)) {
            send_system_message("你已拒绝 " + sender_username + " 的好友请求。");
        } else {
            send_system_message("操作失败，可能没有来自该用户的好友请求。");
        }

    } else if (subcmd == "del") {
        // 删除好友
        if (args.size() < 2) {
            send_system_message("用法：/friend del <用户名>");
            return;
        }

        std::string target_username = args[1];
        int target_user_id = friend_manager_->get_user_id_by_name(target_username);

        if (target_user_id < 0) {
            send_system_message("用户不存在：" + target_username);
            return;
        }

        if (friend_manager_->remove_friend(session_.user_id, target_user_id)) {
            send_system_message("已删除好友：" + target_username);
            
            // 刷新好友列表
            cmd_friend({"list"});
        } else {
            send_system_message("操作失败，可能不是好友。");
        }

    } else {
        send_system_message("未知命令。用法：/friend <list|request|accept|reject|del> [用户名]");
    }
}

// ========== 私聊命令 ==========

void Connection::cmd_private_message(const std::vector<std::string>& args) {
    if (!session_.is_authenticated()) {
        send_system_message("请先登录。");
        return;
    }

    if (args.size() < 2) {
        send_system_message("用法：/pm <用户名> <消息内容>");
        return;
    }

    std::string to_user = args[0];
    std::string content;
    
    // 提取消息内容（可能包含空格）
    size_t pos = 0;
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) pos += args[i-1].length() + 1;
        if (i >= 1) {
            content += (i > 1 ? " " : "") + args[i];
        }
    }

    // 检查是否是好友
    int to_user_id = friend_manager_->get_user_id_by_name(to_user);
    if (to_user_id < 0) {
        send_system_message("用户不存在：" + to_user);
        return;
    }

    if (!friend_manager_->is_friend(session_.user_id, to_user_id)) {
        send_system_message("你们不是好友，无法发送私聊消息。");
        return;
    }

    // 获取标准用户名（从数据库中获取正确的用户名格式）
    std::string canonical_to_user = friend_manager_->get_username_by_id(to_user_id);
    if (canonical_to_user.empty()) {
        send_system_message("无法获取用户名");
        return;
    }

    // 保存消息到数据库
    Logger::info("💾 正在保存私聊消息到数据库: " + session_.username + " -> " + canonical_to_user);
    if (db_) {
        Message msg;
        msg.type = "private";
        msg.from = session_.username;
        msg.to = canonical_to_user;
        msg.content = content;
        msg.timestamp = std::time(nullptr);
        db_->save_message(msg);

        // 额外存入 private_messages 表用于离线推送
        const char* sql = "INSERT INTO private_messages (from_user, to_user, content, is_read, timestamp) VALUES (?, ?, ?, 0, ?)";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, msg.from.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, msg.to.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, msg.content.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 4, msg.timestamp);
            int rc = sqlite3_step(stmt);
            if (rc == SQLITE_DONE) {
                Logger::info("✅ 成功保存离线消息到 private_messages 表");
            } else {
                const char* err = sqlite3_errmsg(db_->get_db());
                Logger::error("❌ 保存离线消息失败: " + std::string(err));
            }
            sqlite3_finalize(stmt);
        } else {
            Logger::error("❌ SQL 预处理失败: " + std::string(sqlite3_errmsg(db_->get_db())));
        }
    }

    // 发送消息给接收方（如果在线）
    bool sent = false;
    for (auto& conn : all_connections_) {
        if (conn && conn->session_.user_id == to_user_id) {
            std::string json = "{\"type\":\"private_message\",\"from\":\"" + session_.username + "\",\"to\":\"" + canonical_to_user + "\",\"content\":\"" + content + "\",\"timestamp\":" + std::to_string(std::time(nullptr)) + "}";
            conn->send(json);
            sent = true;
            break;
        }
    }

    // 发送确认给发送方
    if (sent) {
        std::string json = "{\"type\":\"sent_message\",\"from\":\"" + session_.username + "\",\"to\":\"" + canonical_to_user + "\",\"content\":\"" + content + "\",\"timestamp\":" + std::to_string(std::time(nullptr)) + "}";
        send(json);
    } else {
        send_system_message("消息已发送（对方不在线）");
    }
}

std::string Connection::get_preset_ai_prompt(const std::string& preset_name) {
    return preset_manager_->get_ai_prompt(session_.user_id, preset_name);
}

// 清理 Markdown 格式，转换为纯文本
static std::string strip_markdown(const std::string& text) {
    std::string result = text;
    
    // 移除标题符号 (# ## ### 等)
    std::regex header_regex(R"(^#{1,6}\s*)", std::regex::multiline);
    result = std::regex_replace(result, header_regex, "");
    
    // 移除粗体和斜体符号 (**text** 或 *text*)
    std::regex bold_regex(R"(\*\*(.+?)\*\*)");
    result = std::regex_replace(result, bold_regex, "$1");
    
    std::regex italic_regex(R"(\*(.+?)\*)");
    result = std::regex_replace(result, italic_regex, "$1");
    
    // 移除水平线 (--- 或 ***)
    std::regex hr_regex(R"(^[-*]{3,}\s*$)", std::regex::multiline);
    result = std::regex_replace(result, hr_regex, "");
    
    // 移除列表符号 (- 或 * 开头)
    std::regex list_regex(R"(^[\s]*[-*]\s+)", std::regex::multiline);
    result = std::regex_replace(result, list_regex, "");
    
    // 移除多余的空行（连续两个以上换行符替换为两个）
    std::regex extra_newlines_regex(R"(\n{3,})");
    result = std::regex_replace(result, extra_newlines_regex, "\n\n");
    
    // 移除首尾空白
    result = std::regex_replace(result, std::regex(R"(^\s+|\s+$)"), "");
    
    return result;
}

void Connection::send_ai_response(const std::string& prompt, const std::string& user_message,
                                   const std::vector<AIMessage>& context) {
    // 发送"正在思考"提示
    send_system_message("AI 思考中...");

    // 将预设 prompt 作为第一个消息插入到上下文中
    std::vector<AIMessage> context_with_prompt;
    context_with_prompt.push_back(AIMessage{"system", prompt});
    context_with_prompt.insert(context_with_prompt.end(), context.begin(), context.end());

    Logger::info("Sending AI response with prompt: " + prompt.substr(0, 100) + "...");

    // 使用异步 AI 调用（不阻塞连接）
    auto self = shared_from_this();
    ai_client_->chat_async(user_message, context_with_prompt, [self, prompt](const std::string& reply) {
        // AI 回复回调（在后台线程中执行）
        if (!self) return;

        // 清理 Markdown 格式
        std::string clean_reply = strip_markdown(reply);

        // 发送 AI 回复
        Message response;
        response.type = "ai_response";
        response.from = "AI";
        response.to = self->session_.username;
        response.room = self->session_.current_room;
        response.content = clean_reply;
        response.timestamp = std::time(nullptr);

        self->send(JsonParser::stringify(response));

        // 保存到数据库
        if (self->db_) {
            self->db_->save_message(response);
        }

        // 保存 AI 回复到上下文
        if (self->conversation_manager_) {
            self->conversation_manager_->add_message(
                self->session_.current_room,
                std::to_string(self->session_.user_id),
                "assistant",
                clean_reply
            );
            self->conversation_manager_->save_context(
                self->session_.current_room,
                std::to_string(self->session_.user_id)
            );
        }
    });
}

// 推送离线消息
void Connection::send_offline_messages() {
    Logger::info("[离线消息] 开始检查用户 " + session_.username + " (ID: " + std::to_string(session_.user_id) + ") 的离线消息");

    if (!session_.is_authenticated()) {
        Logger::warn("[离线消息] 用户未认证，跳过");
        return;
    }
    if (!db_) {
        Logger::warn("[离线消息] 数据库指针为空，跳过");
        return;
    }

    sqlite3* db = db_->get_db();
    const char* sql = "SELECT id, from_user, content, timestamp FROM private_messages WHERE to_user = ? COLLATE NOCASE AND is_read = 0 ORDER BY timestamp ASC";
    sqlite3_stmt* stmt = nullptr;

    Logger::info("[离线消息] 执行 SQL 查询: " + std::string(sql));
    Logger::info("[离线消息] 查询参数 (to_user): " + session_.username);

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, session_.username.c_str(), -1, SQLITE_STATIC);

        int count = 0;
        int step_result = 0;
        while ((step_result = sqlite3_step(stmt)) == SQLITE_ROW) {
            int msg_id = sqlite3_column_int(stmt, 0);
            const char* from_user_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* content_cstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            int64_t timestamp = sqlite3_column_int64(stmt, 3);

            std::string from_user = from_user_cstr ? from_user_cstr : "";
            std::string content = content_cstr ? content_cstr : "";

            Logger::info("[离线消息] 找到消息 #" + std::to_string(msg_id) + " 来自 " + from_user + ": " + content);

            // 简单转义防止 JSON 破坏
            std::string safe_content;
            for (char c : content) {
                if (c == '"') safe_content += "\\\"";
                else if (c == '\\') safe_content += "\\\\";
                else safe_content += c;
            }

            std::string json = "{\"type\":\"private_message\",\"from\":\"" + from_user + "\",\"to\":\"" + session_.username + "\",\"content\":\"" + safe_content + "\",\"timestamp\":" + std::to_string(timestamp) + "}";

            Logger::info("[离线消息] 推送消息 #" + std::to_string(msg_id) + " 给 " + session_.username + ": " + content);
            this->send(json);
            count++;

            // 标记为已读
            const char* update_sql = "UPDATE private_messages SET is_read = 1 WHERE id = ?";
            sqlite3_stmt* update_stmt = nullptr;
            if (sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(update_stmt, 1, msg_id);
                int rc = sqlite3_step(update_stmt);
                Logger::info("[离线消息] 标记消息 #" + std::to_string(msg_id) + " 为已读，结果: " + std::to_string(rc));
                sqlite3_finalize(update_stmt);
            } else {
                Logger::error("[离线消息] 更新语句预处理失败");
            }
        }
        
        Logger::info("[离线消息] sqlite3_step 最终返回: " + std::to_string(step_result) + " (SQLITE_DONE=" + std::to_string(SQLITE_DONE) + ")");
        
        sqlite3_finalize(stmt);

        if (count > 0) {
            Logger::info("✅ 推送了 " + std::to_string(count) + " 条离线消息给 " + session_.username);
        } else {
            Logger::info("[离线消息] 没有未读消息");
        }
    } else {
        Logger::error("[离线消息] SQL 查询预处理失败: " + std::string(sqlite3_errmsg(db)));
    }
}

} // namespace chat
