cmake_minimum_required(VERSION 3.16)
project(cpp_ai_chat_server VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Boost
find_package(Boost REQUIRED COMPONENTS system)

# 查找 OpenSSL (用于 HTTPS)
find_package(OpenSSL REQUIRED)

# 查找 SQLite3
find_package(SQLite3 REQUIRED)

# 查找或下载 nlohmann/json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(json)
endif()

# 查找 CURL
find_package(CURL REQUIRED)

# 查找或下载 spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        URL https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/server/server.cpp
    src/server/connection.cpp
    src/protocol/message.cpp
    src/protocol/json_parser.cpp
    src/protocol/websocket.cpp
    src/ai/ai_client.cpp
    src/storage/database.cpp
    src/utils/logger.cpp
    src/user/user_manager.cpp
    src/room/room_manager.cpp
    src/preset/preset_manager.cpp
    src/command/command_parser.cpp
    src/chat/chat_room.cpp
    src/conversation/conversation.cpp
    src/history/message_history.cpp
)

# 头文件
set(HEADERS
    src/server/server.h
    src/server/connection.h
    src/protocol/message.h
    src/protocol/json_parser.h
    src/protocol/websocket.h
    src/ai/ai_client.h
    src/ai/api_config.h
    src/storage/database.h
    src/utils/logger.h
    src/user/user_manager.h
    src/room/room_manager.h
    src/preset/preset_manager.h
    src/command/command_parser.h
    src/chat/chat_room.h
    src/conversation/conversation.h
    src/history/message_history.h
)

# 可执行文件
add_executable(chat_server ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(chat_server PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    src
)

# 链接库
target_link_libraries(chat_server PRIVATE
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    SQLite::SQLite3
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    pthread
)

# 编译选项
target_compile_options(chat_server PRIVATE -Wall -Wextra -O2)

# 测试客户端
add_executable(ws_client scripts/ws_client.cpp)
target_link_libraries(ws_client PRIVATE ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} pthread)

# ========== 单元测试 ==========
enable_testing()

# 下载 GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

# 测试可执行文件
add_executable(chat_server_test
    tests/test_user_manager.cpp
    tests/test_room_manager.cpp
    tests/test_message_history.cpp
    tests/test_conversation.cpp
    src/storage/database.cpp
    src/user/user_manager.cpp
    src/room/room_manager.cpp
    src/utils/logger.cpp
    src/history/message_history.cpp
    src/conversation/conversation.cpp
    src/protocol/json_parser.cpp
)

target_link_libraries(chat_server_test PRIVATE
    GTest::gtest_main
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    SQLite::SQLite3
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
    pthread
)

target_include_directories(chat_server_test PRIVATE
    src
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# 启用测试
include(GoogleTest)
gtest_discover_tests(chat_server_test)
