#include <gtest/gtest.h>
#include "history/message_history.h"
#include "storage/database.h"

using namespace chat;

class MessageHistoryTest : public ::testing::Test {
protected:
    std::unique_ptr<Database> db_;
    std::unique_ptr<MessageHistoryManager> history_manager_;

    void SetUp() override {
        db_ = std::make_unique<Database>(":memory:");
        db_->init();
        history_manager_ = std::make_unique<MessageHistoryManager>(db_.get());
        InsertTestData();
    }

    void TearDown() override {
        history_manager_.reset();
        db_.reset();
    }

    void InsertTestData() {
        const char* sql = R"(
            INSERT INTO messages (from_user, to_user, room_name, content, message_type, timestamp)
            VALUES (?, ?, ?, ?, ?, ?)
        )";
        
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
            // 插入测试消息
            sqlite3_bind_text(stmt, 1, "user1", -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, "AI", -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, "general", -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, "你好", -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, "text", -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 6, std::time(nullptr));
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
};

// 测试获取 AI 历史
TEST_F(MessageHistoryTest, GetAIHistory) {
    auto messages = history_manager_->get_history("@ai", false, 10);
    EXPECT_GT(messages.size(), 0);
}

// 测试搜索消息
TEST_F(MessageHistoryTest, SearchMessages) {
    SearchCriteria criteria;
    criteria.keyword = "你好";
    criteria.limit = 10;
    
    auto messages = history_manager_->search(criteria);
    EXPECT_GT(messages.size(), 0);
}

// 测试导出功能
TEST_F(MessageHistoryTest, ExportMessages) {
    std::string txt_export = history_manager_->export_messages("@ai", false, "txt");
    EXPECT_FALSE(txt_export.empty());
}
