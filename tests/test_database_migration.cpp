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

} // namespace

TEST(DatabaseMigrationTest, InitCreatesCoreSchemaVersion)
{
    const std::string db_path = make_temp_db_path();

    Database db(db_path);
    ASSERT_TRUE(db.init());

    EXPECT_EQ(read_schema_version(db.get_db(), "core_schema_version"), 3);

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
        EXPECT_EQ(read_schema_version(db.get_db(), "core_schema_version"), 3);
        EXPECT_EQ(count_column(db.get_db(), "messages", "is_read"), 1);
    }

    std::remove(db_path.c_str());
}
