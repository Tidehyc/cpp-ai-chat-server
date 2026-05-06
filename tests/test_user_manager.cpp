#include <gtest/gtest.h>
#include "user/user_manager.h"
#include "storage/database.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <ctime>

using namespace chat;

namespace
{

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

static std::string legacy_sha256(const std::string &text)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(text.c_str()), text.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

// 测试夹具
class UserManagerTest : public ::testing::Test
{
protected:
    std::unique_ptr<Database> db_;
    std::unique_ptr<UserManager> user_manager_;

    void SetUp() override
    {
        // 创建临时数据库
        db_ = std::make_unique<Database>(":memory:");
        db_->init();
        user_manager_ = std::make_unique<UserManager>(db_.get());
    }

    void TearDown() override
    {
        user_manager_.reset();
        db_.reset();
    }
};

// 测试用户注册
TEST_F(UserManagerTest, RegisterUser)
{
    // 成功注册
    EXPECT_TRUE(user_manager_->register_user("testuser", "123456"));

    // 重复注册失败
    EXPECT_FALSE(user_manager_->register_user("testuser", "123456"));

    // 空用户名失败
    EXPECT_FALSE(user_manager_->register_user("", "123456"));

    // 空密码失败
    EXPECT_FALSE(user_manager_->register_user("testuser2", ""));
}

// 测试用户登录
TEST_F(UserManagerTest, UserLogin)
{
    // 先注册
    user_manager_->register_user("testuser", "123456");

    User user;
    // 正确密码登录成功
    LoginResult result1 = user_manager_->login("testuser", "123456", user);
    EXPECT_EQ(result1, LoginResult::SUCCESS);
    EXPECT_EQ(user.username, "testuser");

    // 错误密码登录失败
    LoginResult result2 = user_manager_->login("testuser", "wrong_password", user);
    EXPECT_EQ(result2, LoginResult::WRONG_PASSWORD);

    // 不存在的用户登录失败
    LoginResult result3 = user_manager_->login("nonexistent", "123456", user);
    EXPECT_EQ(result3, LoginResult::USER_NOT_FOUND);
}

TEST_F(UserManagerTest, BannedUserCannotLogin)
{
    user_manager_->register_user("testuser", "123456");

    User user = user_manager_->get_user_by_name("testuser");
    ASSERT_TRUE(user.is_valid());
    ASSERT_TRUE(user_manager_->update_status(user.id, "banned"));

    User login_user;
    LoginResult result = user_manager_->login("testuser", "123456", login_user);
    EXPECT_EQ(result, LoginResult::BANNED);
}

TEST_F(UserManagerTest, AdminUserGetsAdminRole)
{
    ASSERT_TRUE(user_manager_->register_user("admin", "123456"));

    User admin = user_manager_->get_user_by_name("admin");
    ASSERT_TRUE(admin.is_valid());
    EXPECT_EQ(admin.role, "admin");
    EXPECT_TRUE(user_manager_->is_admin_user("admin"));
    EXPECT_EQ(read_schema_version(db_->get_db(), "users_schema_version"), 2);
    EXPECT_EQ(count_column(db_->get_db(), "users", "role"), 1);
}

// 测试用户状态更新
TEST_F(UserManagerTest, UpdateStatus)
{
    user_manager_->register_user("testuser", "123456");

    User user;
    user_manager_->login("testuser", "123456", user);

    // 更新状态
    EXPECT_TRUE(user_manager_->update_status(user.id, "busy"));

    // 获取用户验证状态
    User updated_user = user_manager_->get_user(user.id);
    EXPECT_EQ(updated_user.status, "busy");
}

// 测试在线状态
TEST_F(UserManagerTest, OnlineStatus)
{
    user_manager_->register_user("testuser", "123456");

    User user;
    user_manager_->login("testuser", "123456", user);

    // 设置在线
    user_manager_->set_user_online(user.id, true);
    EXPECT_TRUE(user_manager_->is_online(user.id));

    // 设置离线
    user_manager_->set_user_online(user.id, false);
    EXPECT_FALSE(user_manager_->is_online(user.id));
}

// 测试获取所有用户
TEST_F(UserManagerTest, GetAllUsers)
{
    user_manager_->register_user("user1", "123456");
    user_manager_->register_user("user2", "123456");
    user_manager_->register_user("user3", "123456");

    auto users = user_manager_->get_all_users();
    EXPECT_EQ(users.size(), 3);
}

// 测试预设更新
TEST_F(UserManagerTest, UpdatePreset)
{
    user_manager_->register_user("testuser", "123456");

    User user;
    user_manager_->login("testuser", "123456", user);

    // 更新预设
    EXPECT_TRUE(user_manager_->update_preset(user.id, "teacher"));

    User updated_user = user_manager_->get_user(user.id);
    EXPECT_EQ(updated_user.current_preset, "teacher");
}

// 测试旧版本密码哈希兼容与自动升级
TEST_F(UserManagerTest, LegacyPasswordCompatibleAndUpgrade)
{
    const std::string username = "legacy_user";
    const std::string password = "123456";
    const std::string legacy_hash = legacy_sha256(password);

    const char *insert_sql = R"(
        INSERT INTO users (username, password_hash, status, created_at)
        VALUES (?, ?, 'offline', ?)
    )";

    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(db_->get_db(), insert_sql, -1, &stmt, nullptr), SQLITE_OK);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, legacy_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, std::time(nullptr));
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);

    User user;
    LoginResult result = user_manager_->login(username, password, user);
    EXPECT_EQ(result, LoginResult::SUCCESS);

    const char *query_sql = "SELECT password_hash FROM users WHERE username = ?";
    stmt = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(db_->get_db(), query_sql, -1, &stmt, nullptr), SQLITE_OK);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);

    const unsigned char *upgraded_ptr = sqlite3_column_text(stmt, 0);
    ASSERT_NE(upgraded_ptr, nullptr);
    std::string upgraded = reinterpret_cast<const char *>(upgraded_ptr);
    sqlite3_finalize(stmt);

    EXPECT_NE(upgraded, legacy_hash);
    EXPECT_TRUE(upgraded.rfind("v2$", 0) == 0);
}

// 测试损坏的 v2 密码记录不会导致崩溃，且登录失败
TEST_F(UserManagerTest, CorruptedV2PasswordRecordShouldFailSafely)
{
    const std::string username = "bad_v2_user";
    const std::string corrupted_hash = "v2$incomplete";

    const char *insert_sql = R"(
        INSERT INTO users (username, password_hash, status, created_at)
        VALUES (?, ?, 'offline', ?)
    )";

    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(db_->get_db(), insert_sql, -1, &stmt, nullptr), SQLITE_OK);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, corrupted_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, std::time(nullptr));
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);

    User user;
    LoginResult result = user_manager_->login(username, "123456", user);
    EXPECT_EQ(result, LoginResult::WRONG_PASSWORD);
}
