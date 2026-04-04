#include "server.h"
#include <iostream>

namespace chat {

Server::Server(boost::asio::io_context& io_context, short port,
               Database* db, AIClient* ai_client,
               UserManager* user_manager, RoomManager* room_manager, PresetManager* preset_manager,
               ConversationManager* conversation_manager, MessageHistoryManager* history_manager,
               FriendManager* friend_manager)
    : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , db_(db)
    , ai_client_(ai_client)
    , user_manager_(user_manager)
    , room_manager_(room_manager)
    , preset_manager_(preset_manager)
    , conversation_manager_(conversation_manager)
    , history_manager_(history_manager)
    , friend_manager_(friend_manager) {
    std::cout << "[Server] Listening on port " << port << std::endl;
}

void Server::start() {
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                auto conn = std::make_shared<Connection>(
                    std::move(socket),
                    db_, ai_client_,
                    user_manager_, room_manager_, preset_manager_,
                    conversation_manager_, history_manager_, friend_manager_
                );
                connections_.insert(conn);
                conn->start();
            }
            do_accept();
        }
    );
}

} // namespace chat
