#include "server/server.h"
#include "storage/database.h"
#include "ai/ai_client.h"
#include "ai/api_config.h"
#include "user/user_manager.h"
#include "room/room_manager.h"
#include "preset/preset_manager.h"
#include "conversation/conversation.h"
#include "history/message_history.h"
#include "friend/friend_manager.h"
#include "utils/logger.h"
#include <boost/asio.hpp>
#include <iostream>
#include <cstdlib>

using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    try {
        // 配置
        short port = 8080;
        std::string db_path = "./chat.db";
        
        // 从环境变量读取 AI API 配置
        // 国家超算互联网 API: https://api.scnet.cn/api/llm/v1
        std::string ai_api_key = std::getenv("AI_API_KEY") ? std::getenv("AI_API_KEY") : "";
        std::string ai_api_url = std::getenv("AI_API_URL") ? std::getenv("AI_API_URL") : "https://api.scnet.cn/api/llm/v1/chat/completions";
        std::string ai_model = std::getenv("AI_MODEL") ? std::getenv("AI_MODEL") : "MiniMax-M2.5";

        if (argc > 1) {
            port = std::atoi(argv[1]);
        }

        // 初始化日志系统
        chat::Logger::init("chat_server.log", spdlog::level::info);
        chat::Logger::info("========================================");
        chat::Logger::info("  C++ AI Chat Server v0.3.0");
        chat::Logger::info("========================================");

        std::cout << "AI Provider: 国家超算互联网\n";
        std::cout << "AI API URL: " << ai_api_url << std::endl;
        std::cout << "AI Model: " << ai_model << std::endl;
        std::cout << (ai_api_key.empty() || ai_api_key == "your_api_key_here" 
            ? "⚠️  AI_API_KEY 未设置，AI 功能将返回模拟回复" 
            : "✅ AI API Key 已配置") << std::endl;
        std::cout << std::endl;

        // 初始化数据库
        chat::Database db(db_path);
        if (!db.init()) {
            std::cerr << "Failed to initialize database" << std::endl;
            return 1;
        }

        // 初始化 AI 客户端
        chat::AIClient ai_client(ai_api_key, ai_api_url);
        ai_client.set_model(ai_model);

        // 初始化用户管理器
        chat::UserManager user_manager(&db);

        // 初始化房间管理器
        chat::RoomManager room_manager(&db);

        // 初始化预设管理器
        chat::PresetManager preset_manager(&db);

        // 初始化对话上下文管理器
        chat::ConversationManager conversation_manager(&db);

        // 初始化消息历史管理器
        chat::MessageHistoryManager history_manager(&db);

        // 初始化好友管理器
        chat::FriendManager friend_manager(&db);

        // 启动服务器
        boost::asio::io_context io_context;
        chat::Server server(io_context, port, &db, &ai_client, &user_manager, &room_manager, &preset_manager, &conversation_manager, &history_manager, &friend_manager);
        server.start();  // 开始接受连接

        std::cout << "Server is running on port " << port << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;

        io_context.run();

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
