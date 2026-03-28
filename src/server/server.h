#pragma once

#include "connection.h"
#include <boost/asio.hpp>
#include <memory>
#include <set>

namespace chat {

class Database;
class AIClient;
class UserManager;
class RoomManager;
class PresetManager;

class Server {
public:
    Server(boost::asio::io_context& io_context, short port,
           Database* db, AIClient* ai_client,
           UserManager* user_manager, RoomManager* room_manager, PresetManager* preset_manager,
           ConversationManager* conversation_manager, MessageHistoryManager* history_manager);
    void start();

private:
    void do_accept();

    boost::asio::ip::tcp::acceptor acceptor_;
    std::set<ConnectionPtr> connections_;

    Database* db_;
    AIClient* ai_client_;
    UserManager* user_manager_;
    RoomManager* room_manager_;
    PresetManager* preset_manager_;
    ConversationManager* conversation_manager_;
    MessageHistoryManager* history_manager_;
};

} // namespace chat
