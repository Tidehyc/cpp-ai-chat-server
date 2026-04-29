#include <gtest/gtest.h>

#include "storage/database.h"

#include <cstdio>
#include <ctime>
#include <memory>
#include <string>

using namespace chat;

namespace
{

    std::string make_temp_db_path()
    {
        const std::string path = "/tmp/chat_migration_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(std::rand()) + ".db";
        std::remove(path.c_str());
        return path;
    }

    int read_schema_version(sqlite3 *db, const char *key)
    {
        const char *sql = "SELECT value FROM schema_meta WHERE key = ?";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return -1;
        }

        sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

        int version = -1;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            version = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return version;
    }

    int count_column(sqlite3 *db, const std::string &table_name, const std::string &column_name)
    {
        const std::string sql = "PRAGMA table_info(" + table_name + ")";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return 0;
        }

        int count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            if (name && column_name == name)
            {
                ++count;
            }
        }

        sqlite3_finalize(stmt);
        return count;
    }

    int count_rows(sqlite3 *db, const std::string &table_name)
    {
        const std::string sql = "SELECT COUNT(*) FROM " + table_name;
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return -1;
        }

        int count = -1;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return count;
    }

    int count_text_matches(sqlite3 *db, const std::string &table_name, const std::string &column_name, const std::string &value)
    {
        const std::string sql = "SELECT COUNT(*) FROM " + table_name + " WHERE " + column_name + " = ?";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return -1;
        }

        sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_STATIC);

        int count = -1;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return count;
    }

    std::string read_text_column(sqlite3 *db, const std::string &table_name, const std::string &column_name)
    {
        const std::string sql = "SELECT " + column_name + " FROM " + table_name + " ORDER BY id DESC LIMIT 1";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return "";
        }

        std::string value;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char *text = sqlite3_column_text(stmt, 0);
            value = text ? reinterpret_cast<const char *>(text) : "";
        }

        sqlite3_finalize(stmt);
        return value;
    }

} // namespace

TEST(DatabaseMigrationTest, InitCreatesCoreSchemaVersion)
{
    const std::string db_path = make_temp_db_path();

    Database db(db_path);
    ASSERT_TRUE(db.init());

    EXPECT_EQ(read_schema_version(db.get_db(), "core_schema_version"), 4);

    std::remove(db_path.c_str());
}

TEST(DatabaseMigrationTest, InitIsIdempotentForExistingDatabase)
{
    const std::string db_path = make_temp_db_path();

    {
        Database db(db_path);
        ASSERT_TRUE(db.init());
    }

    {
        Database db(db_path);
        ASSERT_TRUE(db.init());
        EXPECT_EQ(read_schema_version(db.get_db(), "core_schema_version"), 4);
        EXPECT_EQ(count_column(db.get_db(), "messages", "is_read"), 1);
        EXPECT_EQ(count_column(db.get_db(), "moderation_events", "actor"), 1);
    }

    std::remove(db_path.c_str());
}

TEST(DatabaseMigrationTest, SaveModerationEventPersistsRow)
{
    const std::string db_path = make_temp_db_path();

    Database db(db_path);
    ASSERT_TRUE(db.init());

    db.save_moderation_event("admin", "alice", "ban", "manual ban");

    EXPECT_EQ(count_rows(db.get_db(), "moderation_events"), 1);
    EXPECT_EQ(read_text_column(db.get_db(), "moderation_events", "actor"), "admin");
    EXPECT_EQ(read_text_column(db.get_db(), "moderation_events", "target"), "alice");
    EXPECT_EQ(read_text_column(db.get_db(), "moderation_events", "action"), "ban");

    std::remove(db_path.c_str());
}

TEST(DatabaseMigrationTest, RecentModerationEventsAreQueryable)
{
    const std::string db_path = make_temp_db_path();

    Database db(db_path);
    ASSERT_TRUE(db.init());

    db.save_moderation_event("admin", "alice", "ban", "manual ban");
    db.save_moderation_event("admin", "alice", "unban", "manual unban");

    auto events = db.get_recent_moderation_events(10);
    ASSERT_EQ(events.size(), 2);
    EXPECT_EQ(events[0].action, "unban");
    EXPECT_EQ(events[0].target, "alice");
    EXPECT_EQ(events[1].action, "ban");
    EXPECT_EQ(count_text_matches(db.get_db(), "moderation_events", "actor", "admin"), 2);

    std::remove(db_path.c_str());
}

TEST(DatabaseMigrationTest, ReportEventPersistsRow)
{
    const std::string db_path = make_temp_db_path();

    Database db(db_path);
    ASSERT_TRUE(db.init());

    db.save_moderation_event("alice", "bob", "report", "spam in chat");

    auto events = db.get_recent_moderation_events(1);
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].actor, "alice");
    EXPECT_EQ(events[0].target, "bob");
    EXPECT_EQ(events[0].action, "report");
    EXPECT_EQ(events[0].reason, "spam in chat");

    std::remove(db_path.c_str());
}

TEST(DatabaseMigrationTest, AnnounceEventPersistsRow)
{
    const std::string db_path = make_temp_db_path();

    Database db(db_path);
    ASSERT_TRUE(db.init());

    db.save_moderation_event("admin", "all_users", "announce", "service maintenance tonight");

    auto events = db.get_recent_moderation_events(1);
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].actor, "admin");
    EXPECT_EQ(events[0].target, "all_users");
    EXPECT_EQ(events[0].action, "announce");
    EXPECT_EQ(events[0].reason, "service maintenance tonight");

    std::remove(db_path.c_str());
}
