#include <gtest/gtest.h>
#include <ctime>
#include <chrono>
#include <iostream>
#include <memory>
#include "history/message_history.h"
#include "storage/database.h"

using namespace chat;

class MessageHistoryTest : public ::testing::Test
{
protected:
    std::unique_ptr<Database> db_;
    std::unique_ptr<MessageHistoryManager> history_manager_;

    void SetUp() override
    {
        db_ = std::make_unique<Database>(":memory:");
        db_->init();
        history_manager_ = std::make_unique<MessageHistoryManager>(db_.get());
        InsertTestData();
    }

    void TearDown() override
    {
        history_manager_.reset();
        db_.reset();
    }

    void InsertTestData()
    {
        const char *sql = R"(
            INSERT INTO messages (from_user, to_user, room_name, content, message_type, timestamp)
            VALUES (?, ?, ?, ?, ?, ?)
        )";

        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
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

    void BulkInsertMessages(int count)
    {
        ASSERT_NE(db_->get_db(), nullptr);
        char *err_msg = nullptr;
        ASSERT_EQ(sqlite3_exec(db_->get_db(), "BEGIN TRANSACTION", nullptr, nullptr, &err_msg), SQLITE_OK)
            << (err_msg ? err_msg : "begin transaction failed");
        if (err_msg)
        {
            sqlite3_free(err_msg);
            err_msg = nullptr;
        }

        const char *sql = R"(
            INSERT INTO messages (from_user, to_user, room_name, content, message_type, timestamp)
            VALUES (?, ?, ?, ?, ?, ?)
        )";

        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr), SQLITE_OK);

        const std::time_t base_ts = std::time(nullptr) - count;
        for (int i = 0; i < count; ++i)
        {
            const std::string from_user = (i % 3 == 0) ? "user1" : ((i % 3 == 1) ? "user2" : "user3");
            const std::string to_user = (i % 5 == 0) ? "AI" : "general";
            const std::string room_name = (i % 2 == 0) ? "general" : "code";
            const std::string content = "perf_message_" + std::to_string(i);

            sqlite3_bind_text(stmt, 1, from_user.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, to_user.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, room_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, content.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, "text", -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 6, static_cast<sqlite3_int64>(base_ts + i));

            ASSERT_EQ(sqlite3_step(stmt), SQLITE_DONE);
            ASSERT_EQ(sqlite3_reset(stmt), SQLITE_OK);
            ASSERT_EQ(sqlite3_clear_bindings(stmt), SQLITE_OK);
        }

        sqlite3_finalize(stmt);

        ASSERT_EQ(sqlite3_exec(db_->get_db(), "COMMIT", nullptr, nullptr, &err_msg), SQLITE_OK)
            << (err_msg ? err_msg : "commit failed");
        if (err_msg)
        {
            sqlite3_free(err_msg);
        }
    }

    double MeasureSearchLatencyMs(int rounds)
    {
        SearchCriteria criteria;
        criteria.room_name = "general";
        criteria.start_time = std::time(nullptr) - 20000;
        criteria.end_time = std::time(nullptr) + 60;
        criteria.limit = 50;

        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < rounds; ++i)
        {
            auto messages = history_manager_->search(criteria);
            if (messages.empty())
            {
                ADD_FAILURE() << "search returned empty result during performance measurement";
                return 1e12;
            }
        }
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - start).count();
    }
};

// 测试获取 AI 历史
TEST_F(MessageHistoryTest, GetAIHistory)
{
    auto messages = history_manager_->get_history("@ai", false, 10);
    EXPECT_GT(messages.size(), 0);
}

// 测试搜索消息
TEST_F(MessageHistoryTest, SearchMessages)
{
    SearchCriteria criteria;
    criteria.keyword = "你好";
    criteria.limit = 10;

    auto messages = history_manager_->search(criteria);
    EXPECT_GT(messages.size(), 0);
}

// 测试搜索关键词不会触发 SQL 注入
TEST_F(MessageHistoryTest, SearchKeywordInjectionSafe)
{
    SearchCriteria criteria;
    criteria.keyword = "' OR 1=1 --";
    criteria.limit = 10;

    auto messages = history_manager_->search(criteria);
    EXPECT_EQ(messages.size(), 0);
}

TEST_F(MessageHistoryTest, SearchEmptyResult)
{
    SearchCriteria criteria;
    criteria.keyword = "不存在的关键词";
    criteria.limit = 10;

    auto messages = history_manager_->search(criteria);
    EXPECT_TRUE(messages.empty());
}

TEST_F(MessageHistoryTest, SearchLimitFallbackWhenNonPositive)
{
    SearchCriteria criteria;
    criteria.keyword = "你好";
    criteria.limit = 0; // 应回退到默认值而不是返回空

    auto messages = history_manager_->search(criteria);
    EXPECT_FALSE(messages.empty());
}

// 测试导出功能
TEST_F(MessageHistoryTest, ExportMessages)
{
    std::string txt_export = history_manager_->export_messages("@ai", false, "txt");
    EXPECT_FALSE(txt_export.empty());
}

TEST_F(MessageHistoryTest, SearchPerformanceBaselineWithIndex)
{
    BulkInsertMessages(10000);

    const double total_ms = MeasureSearchLatencyMs(40);
    const double avg_ms = total_ms / 40.0;
    std::cout << "[perf] indexed_total_ms=" << total_ms << ", indexed_avg_ms=" << avg_ms << std::endl;

    EXPECT_LT(avg_ms, 8.0) << "Average indexed search latency too high: " << avg_ms << " ms";
}

TEST_F(MessageHistoryTest, SearchPerformanceRegressionIndexedShouldNotBeWorseThanNoIndex)
{
    BulkInsertMessages(10000);

    const double indexed_ms = MeasureSearchLatencyMs(40);

    char *err_msg = nullptr;
    ASSERT_EQ(sqlite3_exec(db_->get_db(), "DROP INDEX IF EXISTS idx_room_name_timestamp", nullptr, nullptr, &err_msg), SQLITE_OK)
        << (err_msg ? err_msg : "drop idx_room_name_timestamp failed");
    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = nullptr;
    }

    ASSERT_EQ(sqlite3_exec(db_->get_db(), "DROP INDEX IF EXISTS idx_timestamp", nullptr, nullptr, &err_msg), SQLITE_OK)
        << (err_msg ? err_msg : "drop idx_timestamp failed");
    if (err_msg)
    {
        sqlite3_free(err_msg);
    }

    const double no_index_ms = MeasureSearchLatencyMs(40);
    std::cout << "[perf] indexed_total_ms=" << indexed_ms << ", no_index_total_ms=" << no_index_ms << std::endl;

    EXPECT_LE(indexed_ms, no_index_ms * 1.20)
        << "Indexed search regressed. indexed_ms=" << indexed_ms << " no_index_ms=" << no_index_ms;
}
