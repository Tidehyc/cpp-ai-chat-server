#include <gtest/gtest.h>
#include "conversation/conversation.h"
#include "storage/database.h"

using namespace chat;

class ConversationTest : public ::testing::Test {
protected:
    std::unique_ptr<Database> db_;
    std::unique_ptr<ConversationManager> conv_manager_;

    void SetUp() override {
        db_ = std::make_unique<Database>(":memory:");
        db_->init();
        conv_manager_ = std::make_unique<ConversationManager>(db_.get());
    }

    void TearDown() override {
        conv_manager_.reset();
        db_.reset();
    }
};

// 测试获取上下文
TEST_F(ConversationTest, GetContext) {
    auto& ctx = conv_manager_->get_context("general", "user1");
    
    EXPECT_EQ(ctx.room_name, "general");
    EXPECT_EQ(ctx.user_id, "user1");
    EXPECT_EQ(ctx.max_history, 10);
}

// 测试添加消息
TEST_F(ConversationTest, AddMessage) {
    conv_manager_->add_message("general", "user1", "user", "你好");
    
    auto& ctx = conv_manager_->get_context("general", "user1");
    EXPECT_EQ(ctx.history.size(), 1);
    EXPECT_EQ(ctx.history[0].role, "user");
}

// 测试多轮对话
TEST_F(ConversationTest, MultiTurnConversation) {
    conv_manager_->add_message("general", "user1", "user", "我叫小明");
    conv_manager_->add_message("general", "user1", "assistant", "你好小明");
    
    auto& ctx = conv_manager_->get_context("general", "user1");
    EXPECT_EQ(ctx.history.size(), 2);
}

// 测试清空上下文
TEST_F(ConversationTest, ClearContext) {
    conv_manager_->add_message("general", "user1", "user", "你好");
    conv_manager_->clear_context("general", "user1");
    
    auto& ctx = conv_manager_->get_context("general", "user1");
    EXPECT_EQ(ctx.history.size(), 0);
}

// 测试房间隔离
TEST_F(ConversationTest, RoomIsolation) {
    conv_manager_->add_message("general", "user1", "user", "general 消息");
    conv_manager_->add_message("code", "user1", "user", "code 消息");
    
    auto& general_ctx = conv_manager_->get_context("general", "user1");
    auto& code_ctx = conv_manager_->get_context("code", "user1");
    
    EXPECT_EQ(general_ctx.history.size(), 1);
    EXPECT_EQ(code_ctx.history.size(), 1);
}

// 测试用户隔离
TEST_F(ConversationTest, UserIsolation) {
    conv_manager_->add_message("general", "user1", "user", "user1 消息");
    conv_manager_->add_message("general", "user2", "user", "user2 消息");
    
    auto& user1_ctx = conv_manager_->get_context("general", "user1");
    auto& user2_ctx = conv_manager_->get_context("general", "user2");
    
    EXPECT_EQ(user1_ctx.history.size(), 1);
    EXPECT_EQ(user2_ctx.history.size(), 1);
}
