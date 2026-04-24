#include <gtest/gtest.h>

#include "friend/offline_message_delivery.h"
#include "storage/database.h"

#include <sqlite3.h>
#include <memory>
#include <string>

using namespace chat;

class OfflineMessageDeliveryTest : public ::testing::Test
{
protected:
    std::unique_ptr<Database> db_;

    void SetUp() override
    {
        db_ = std::make_unique<Database>(":memory:");
        db_->init();

        const char *schema_sql = R"(
            CREATE TABLE IF NOT EXISTS private_messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                from_user TEXT NOT NULL,
                to_user TEXT NOT NULL,
                content TEXT NOT NULL,
                is_read INTEGER DEFAULT 0,
                timestamp INTEGER NOT NULL
            )
        )";

        char *err = nullptr;
        ASSERT_EQ(sqlite3_exec(db_->get_db(), schema_sql, nullptr, nullptr, &err), SQLITE_OK);
        if (err)
        {
            sqlite3_free(err);
        }
    }

    void TearDown() override
    {
        db_.reset();
    }

    void insert_private_message(const std::string &from,
                                const std::string &to,
                                const std::string &content,
                                int64_t ts)
    {
        const char *sql = "INSERT INTO private_messages (from_user, to_user, content, is_read, timestamp) VALUES (?, ?, ?, 0, ?)";
        sqlite3_stmt *stmt = nullptr;

        ASSERT_EQ(sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr), SQLITE_OK);
        sqlite3_bind_text(stmt, 1, from.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, to.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 4, ts);
        ASSERT_EQ(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    int unread_count_for(const std::string &to)
    {
        const char *sql = "SELECT COUNT(*) FROM private_messages WHERE to_user = ? AND is_read = 0";
        sqlite3_stmt *stmt = nullptr;
        int count = -1;

        if (sqlite3_prepare_v2(db_->get_db(), sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, to.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                count = sqlite3_column_int(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);
        return count;
    }
};

TEST_F(OfflineMessageDeliveryTest, CommitWhenAllDelivered)
{
    insert_private_message("alice", "bob", "m1", 1);
    insert_private_message("alice", "bob", "m2", 2);

    int delivered = 0;
    std::string err;

    const bool ok = deliver_offline_messages_transactional(
        db_->get_db(),
        "bob",
        [](const OfflineMessageRow &)
        { return true; },
        &delivered,
        &err);

    EXPECT_TRUE(ok);
    EXPECT_EQ(delivered, 2);
    EXPECT_TRUE(err.empty());
    EXPECT_EQ(unread_count_for("bob"), 0);
}

TEST_F(OfflineMessageDeliveryTest, RollbackWhenUpdateFails)
{
    insert_private_message("alice", "bob", "m1", 1);
    insert_private_message("alice", "bob", "m2", 2);

    const char *trigger_sql = R"(
        CREATE TRIGGER fail_second_update
        BEFORE UPDATE OF is_read ON private_messages
        WHEN NEW.id = 2
        BEGIN
            SELECT RAISE(ABORT, 'inject update failure');
        END;
    )";

    char *err_msg = nullptr;
    ASSERT_EQ(sqlite3_exec(db_->get_db(), trigger_sql, nullptr, nullptr, &err_msg), SQLITE_OK);
    if (err_msg)
    {
        sqlite3_free(err_msg);
    }

    int delivered = 0;
    std::string err;
    const bool ok = deliver_offline_messages_transactional(
        db_->get_db(),
        "bob",
        [](const OfflineMessageRow &)
        { return true; },
        &delivered,
        &err);

    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.empty());
    EXPECT_EQ(unread_count_for("bob"), 2);
}

TEST_F(OfflineMessageDeliveryTest, RollbackWhenSinkFails)
{
    insert_private_message("alice", "bob", "m1", 1);
    insert_private_message("alice", "bob", "m2", 2);

    int delivered = 0;
    std::string err;

    const bool ok = deliver_offline_messages_transactional(
        db_->get_db(),
        "bob",
        [](const OfflineMessageRow &row)
        { return row.id == 1; },
        &delivered,
        &err);

    EXPECT_FALSE(ok);
    EXPECT_EQ(delivered, 0);
    EXPECT_EQ(err, "sink failed");
    EXPECT_EQ(unread_count_for("bob"), 2);
}
