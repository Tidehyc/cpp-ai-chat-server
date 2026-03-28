#include "connection.h"
#include "protocol/json_parser.h"
#include "storage/database.h"
#include "ai/ai_client.h"
#include "user/user_manager.h"
#include "room/room_manager.h"
#include "preset/preset_manager.h"
#include "conversation/conversation.h"
#include "history/message_history.h"
#include "utils/logger.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <thread>

namespace chat {

static uint64_t g_connection_id = 0;

// 静态房间列表（阶段 4.1 简单实现）
std::set<std::string> Connection::created_rooms_;

// 静态连接列表（用于广播）
std::set<ConnectionPtr> Connection::all_connections_;

Connection::Connection(boost::asio::ip::tcp::socket socket,
                       Database* db, AIClient* ai_client,
                       UserManager* user_manager, RoomManager* room_manager, PresetManager* preset_manager,
                       ConversationManager* conversation_manager, MessageHistoryManager* history_manager)
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
    , history_manager_(history_manager) {
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
    
    // 如果已登录，标记为离线
    if (session_.is_authenticated()) {
        user_manager_->set_user_online(session_.user_id, false);
        user_manager_->update_status(session_.user_id, "offline");  // 更新数据库状态为 offline
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
    
    // 批处理：如果有多条消息，合并发送
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
    if (!session_.is_authenticated()) {
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
    // 构建广播消息
    std::string broadcast_msg = "[" + session_.username + "]: " + message;
    
    // 广播给同一房间的所有其他用户
    for (auto& conn : all_connections_) {
        if (conn && 
            conn.get() != this && 
            conn->session_.current_room == session_.current_room &&
            conn->session_.is_authenticated()) {
            conn->send_system_message(broadcast_msg);
        }
    }
    
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
    if (user_manager_->login(username, password, user)) {
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
    } else {
        send_system_message("登录失败：用户名或密码错误。");
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
    // 简单实现：直接加入房间（不检查是否存在）
    session_.current_room = room_name;
    send_system_message("已加入房间 #" + room_name);
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
    // 添加到房间列表
    created_rooms_.insert(room_name);
    // 加入房间
    session_.current_room = room_name;
    send_system_message("已创建并加入房间 #" + room_name);
    send_system_message("提示：使用 /list 查看房间，/leave 离开房间");
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

std::string Connection::get_preset_ai_prompt(const std::string& preset_name) {
    return preset_manager_->get_ai_prompt(session_.user_id, preset_name);
}

void Connection::send_ai_response(const std::string& prompt, const std::string& user_message,
                                   const std::vector<AIMessage>& context) {
    // 发送"正在思考"提示
    send_system_message("AI 思考中...");

    // 使用异步 AI 调用（不阻塞连接）
    auto self = shared_from_this();
    ai_client_->chat_async(user_message, context, [self, prompt](const std::string& reply) {
        // AI 回复回调（在后台线程中执行）
        if (!self) return;
        
        // 发送 AI 回复
        Message response;
        response.type = "ai_response";
        response.from = "AI";
        response.to = self->session_.username;
        response.room = self->session_.current_room;
        response.content = reply;
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
                reply
            );
            self->conversation_manager_->save_context(
                self->session_.current_room, 
                std::to_string(self->session_.user_id)
            );
        }
    });
}

} // namespace chat
