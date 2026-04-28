#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <set>

#include "protocol/websocket.h"
#include "protocol/message.h"
#include "ai/ai_client.h"
#include "conversation/conversation.h"
#include "friend/friend_manager.h"
#include "command/command_dispatcher.h"

#define private public
#include "server/connection.h"
#undef private

#include "storage/database.h"
#include "user/user_manager.h"

using namespace chat;

namespace
{

    std::string decode_last_frame(Connection &connection)
    {
        if (connection.write_queue_.empty())
        {
            return "";
        }

        WebSocketFrame frame;
        EXPECT_EQ(WebSocketParser::parse(connection.write_queue_.back(), frame), WSResult::SUCCESS);
        return WebSocketParser::decode(frame);
    }

    class ConnectionModerationTest : public ::testing::Test
    {
    protected:
        boost::asio::io_context io_;
        std::unique_ptr<Database> db_;
        std::unique_ptr<UserManager> user_manager_;
        std::shared_ptr<Connection> connection_;

        void SetUp() override
        {
            db_ = std::make_unique<Database>(":memory:");
            ASSERT_TRUE(db_->init());
            user_manager_ = std::make_unique<UserManager>(db_.get());

            boost::asio::ip::tcp::socket socket(io_);
            connection_ = std::make_shared<Connection>(
                std::move(socket),
                db_.get(),
                nullptr,
                user_manager_.get(),
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr);

            connection_->state_ = ConnectionState::ACTIVE;
        }

        void TearDown() override
        {
            connection_.reset();
            user_manager_.reset();
            db_.reset();
        }

        void login_as(const std::string &username)
        {
            User user;
            ASSERT_EQ(user_manager_->login(username, "123456", user), LoginResult::SUCCESS);
            connection_->session_.user_id = user.id;
            connection_->session_.username = user.username;
            connection_->session_.role = user.role;
            connection_->session_.status = user.status;
        }
    };

} // namespace

TEST_F(ConnectionModerationTest, BanRejectsNonAdmin)
{
    ASSERT_TRUE(user_manager_->register_user("alice", "123456"));
    login_as("alice");

    connection_->cmd_ban({"bob"});

    ASSERT_FALSE(connection_->write_queue_.empty());
    const std::string payload = decode_last_frame(*connection_);
    EXPECT_NE(payload.find("仅管理员可封禁账号。"), std::string::npos);
    EXPECT_NE(payload.find("MODERATION_NOT_AUTHORIZED"), std::string::npos);
}

TEST_F(ConnectionModerationTest, UnbanRejectsNonAdmin)
{
    ASSERT_TRUE(user_manager_->register_user("alice", "123456"));
    login_as("alice");

    connection_->cmd_unban({"bob"});

    ASSERT_FALSE(connection_->write_queue_.empty());
    const std::string payload = decode_last_frame(*connection_);
    EXPECT_NE(payload.find("仅管理员可解封账号。"), std::string::npos);
    EXPECT_NE(payload.find("MODERATION_NOT_AUTHORIZED"), std::string::npos);
}

TEST_F(ConnectionModerationTest, ReportRejectsMissingReason)
{
    ASSERT_TRUE(user_manager_->register_user("alice", "123456"));
    login_as("alice");

    connection_->cmd_report({"bob"});

    ASSERT_FALSE(connection_->write_queue_.empty());
    const std::string payload = decode_last_frame(*connection_);
    EXPECT_NE(payload.find("用法：/report <用户名> <原因>"), std::string::npos);
    EXPECT_NE(payload.find("MODERATION_INVALID_ARGS"), std::string::npos);
}

TEST_F(ConnectionModerationTest, AnnounceRejectsNonAdmin)
{
    ASSERT_TRUE(user_manager_->register_user("alice", "123456"));
    login_as("alice");

    connection_->cmd_announce({"系统", "维护"});

    ASSERT_FALSE(connection_->write_queue_.empty());
    const std::string payload = decode_last_frame(*connection_);
    EXPECT_NE(payload.find("仅管理员可发布公告。"), std::string::npos);
    EXPECT_NE(payload.find("MODERATION_NOT_AUTHORIZED"), std::string::npos);
}