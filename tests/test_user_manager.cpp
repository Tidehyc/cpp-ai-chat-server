#include <gtest/gtest.h>
#include "user/user_manager.h"
#include "storage/database.h"
#include <cstdlib>
#include <ctime>

using namespace chat;

// 测试夹具
class UserManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<Database> db_;
    std::unique_ptr<UserManager> user_manager_;

    void SetUp() override {
        // 创建临时数据库
        db_ = std::make_unique<Database>(":memory:");
        db_->init();
        user_manager_ = std::make_unique<UserManager>(db_.get());
    }

    void TearDown() override {
        user_manager_.reset();
        db_.reset();
    }
};

// 测试用户注册
TEST_F(UserManagerTest, RegisterUser) {
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
TEST_F(UserManagerTest, UserLogin) {
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

// 测试用户状态更新
TEST_F(UserManagerTest, UpdateStatus) {
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
TEST_F(UserManagerTest, OnlineStatus) {
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
TEST_F(UserManagerTest, GetAllUsers) {
    user_manager_->register_user("user1", "123456");
    user_manager_->register_user("user2", "123456");
    user_manager_->register_user("user3", "123456");
    
    auto users = user_manager_->get_all_users();
    EXPECT_EQ(users.size(), 3);
}

// 测试预设更新
TEST_F(UserManagerTest, UpdatePreset) {
    user_manager_->register_user("testuser", "123456");
    
    User user;
    user_manager_->login("testuser", "123456", user);
    
    // 更新预设
    EXPECT_TRUE(user_manager_->update_preset(user.id, "teacher"));
    
    User updated_user = user_manager_->get_user(user.id);
    EXPECT_EQ(updated_user.current_preset, "teacher");
}
