# 学习笔记 - 阶段 5.2：单元测试

_版本：v0.5.2_
_最后更新：2026-03-28_

---

## 📚 什么是单元测试？

### 通俗解释

**单元测试**就像是给每个零件做"体检"。

想象你在造一辆汽车：
- **发动机** - 需要单独测试能否启动
- **刹车** - 需要单独测试能否刹住
- **方向盘** - 需要单独测试能否转向

如果等整车装好了再测试，出了问题很难找到是哪个零件的问题。

**在软件中**：
- **UserManager**（用户管理器）- 测试注册、登录是否正常
- **RoomManager**（房间管理器）- 测试创建房间、加入房间是否正常
- **MessageHistory**（消息历史）- 测试查询历史记录是否正常
- **Conversation**（对话上下文）- 测试多轮对话记忆是否正常

---

## 🔧 GoogleTest 框架

### 什么是 GoogleTest？

GoogleTest 是一个 C++ 单元测试框架，提供：
- **出题功能**（定义测试）
- **评分功能**（判断对错）
- **成绩单**（测试报告）

### 基本用法

```cpp
#include <gtest/gtest.h>

// 定义一个简单测试
TEST(TestSuite, TestName) {
    // 出题
    int result = 2 + 2;
    
    // 评分
    EXPECT_EQ(result, 4);  // 如果 result == 4，通过；否则失败
}
```

### 测试夹具（Test Fixture）

**为什么需要测试夹具**？
- 多个测试需要相同的环境
- 避免重复代码
- 自动清理资源

**如何使用**：
```cpp
// 1. 定义测试夹具
class UserManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<UserManager> user_manager_;

    void SetUp() override {
        // 每个测试前执行
        user_manager_ = std::make_unique<UserManager>(db);
    }

    void TearDown() override {
        // 每个测试后执行（自动清理）
        user_manager_.reset();
    }
};

// 2. 使用测试夹具
TEST_F(UserManagerTest, RegisterUser) {
    EXPECT_TRUE(user_manager_->register_user("testuser", "123456"));
}
```

---

## 📦 测试内容详解

### 1. UserManager 测试（6 个测试）

#### RegisterUser - 测试用户注册

```cpp
TEST_F(UserManagerTest, RegisterUser) {
    // 成功注册
    EXPECT_TRUE(user_manager_->register_user("testuser", "123456"));
    
    // 重复注册失败
    EXPECT_FALSE(user_manager_->register_user("testuser", "123456"));
    
    // 空用户名失败
    EXPECT_FALSE(user_manager_->register_user("", "123456"));
}
```

**测试场景**：
- ✅ 正常注册
- ✅ 重复注册
- ✅ 空用户名

#### UserLogin - 测试用户登录

```cpp
TEST_F(UserManagerTest, UserLogin) {
    user_manager_->register_user("testuser", "123456");
    
    User user;
    // 正确密码
    EXPECT_TRUE(user_manager_->login("testuser", "123456", user));
    
    // 错误密码
    EXPECT_FALSE(user_manager_->login("testuser", "wrong", user));
    
    // 不存在的用户
    EXPECT_FALSE(user_manager_->login("nonexistent", "123456", user));
}
```

**测试场景**：
- ✅ 正确密码登录
- ✅ 错误密码登录
- ✅ 不存在用户

---

### 2. RoomManager 测试（5 个测试）

#### BuiltinRooms - 测试内置房间

```cpp
TEST_F(RoomManagerTest, BuiltinRooms) {
    auto rooms = room_manager_->get_all_rooms();
    
    // 应该有 6 个内置房间
    EXPECT_EQ(rooms.size(), 6);
    
    // 检查房间名称
    std::set<std::string> room_names;
    for (const auto& room : rooms) {
        room_names.insert(room.name);
    }
    
    EXPECT_TRUE(room_names.count("general"));
    EXPECT_TRUE(room_names.count("code"));
    // ... 其他房间
}
```

**测试场景**：
- ✅ 房间数量正确
- ✅ 房间名称正确

---

### 3. MessageHistory 测试（3 个测试）

#### GetAIHistory - 测试获取 AI 历史

```cpp
TEST_F(MessageHistoryTest, GetAIHistory) {
    auto messages = history_manager_->get_history("@ai", false, 10);
    
    // 应该有 AI 相关的消息
    EXPECT_GT(messages.size(), 0);
}
```

**测试场景**：
- ✅ 能获取 AI 对话历史

---

### 4. Conversation 测试（6 个测试）

#### MultiTurnConversation - 测试多轮对话

```cpp
TEST_F(ConversationTest, MultiTurnConversation) {
    conv_manager_->add_message("general", "user1", "user", "我叫小明");
    conv_manager_->add_message("general", "user1", "assistant", "你好小明");
    
    auto& ctx = conv_manager_->get_context("general", "user1");
    EXPECT_EQ(ctx.history.size(), 2);
}
```

**测试场景**：
- ✅ 能记住多轮对话
- ✅ 消息顺序正确

#### RoomIsolation - 测试房间隔离

```cpp
TEST_F(ConversationTest, RoomIsolation) {
    conv_manager_->add_message("general", "user1", "user", "general 消息");
    conv_manager_->add_message("code", "user1", "user", "code 消息");
    
    auto& general_ctx = conv_manager_->get_context("general", "user1");
    auto& code_ctx = conv_manager_->get_context("code", "user1");
    
    EXPECT_EQ(general_ctx.history.size(), 1);
    EXPECT_EQ(code_ctx.history.size(), 1);
    EXPECT_NE(general_ctx.history[0].content, code_ctx.history[0].content);
}
```

**测试场景**：
- ✅ 不同房间对话不混淆

---

## 🏗️ 测试基础设施

### 目录结构

```
cpp-ai-chat-server/
├── tests/
│   ├── test_user_manager.cpp      # 用户管理器测试
│   ├── test_room_manager.cpp      # 房间管理器测试
│   ├── test_message_history.cpp   # 消息历史测试
│   └── test_conversation.cpp      # 对话上下文测试
├── CMakeLists.txt                 # 测试配置
└── build/
    └── chat_server_test           # 测试程序
```

### CMake 配置

```cmake
# 下载 GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)

# 创建测试
add_executable(chat_server_test
    tests/test_user_manager.cpp
    tests/test_room_manager.cpp
    ...
)

# 链接 GoogleTest
target_link_libraries(chat_server_test PRIVATE
    GTest::gtest_main
    ...
)
```

---

## 📊 测试结果

```
[==========] Running 20 tests from 4 test suites.
[----------] 6 tests from UserManagerTest
[       OK ] UserManagerTest.RegisterUser (8 ms)
[       OK ] UserManagerTest.UserLogin (2 ms)
[       OK ] UserManagerTest.UpdateStatus (1 ms)
[       OK ] UserManagerTest.OnlineStatus (1 ms)
[       OK ] UserManagerTest.GetAllUsers (2 ms)
[       OK ] UserManagerTest.UpdatePreset (1 ms)
[----------] 5 tests from RoomManagerTest
[       OK ] RoomManagerTest.BuiltinRooms (2 ms)
[       OK ] RoomManagerTest.GetRoom (2 ms)
[       OK ] RoomManagerTest.CreateRoom (2 ms)
[       OK ] RoomManagerTest.JoinRoom (1 ms)
[       OK ] RoomManagerTest.LeaveRoom (1 ms)
[----------] 3 tests from MessageHistoryTest
[       OK ] MessageHistoryTest.GetAIHistory (1 ms)
[       OK ] MessageHistoryTest.SearchMessages (1 ms)
[       OK ] MessageHistoryTest.ExportMessages (1 ms)
[----------] 6 tests from ConversationTest
[       OK ] ConversationTest.GetContext (1 ms)
[       OK ] ConversationTest.AddMessage (1 ms)
[       OK ] ConversationTest.MultiTurnConversation (1 ms)
[       OK ] ConversationTest.ClearContext (1 ms)
[       OK ] ConversationTest.RoomIsolation (1 ms)
[       OK ] ConversationTest.UserIsolation (1 ms)
[==========] 20 tests from 4 test suites ran. (49 ms total)
[  PASSED  ] 20 tests.
```

**通过率**：100% ✅

---

## 🎯 为什么需要单元测试？

### 没有测试时

```
1. 修改代码
2. 启动服务器
3. 手动登录
4. 手动测试注册
5. 手动测试登录
6. 手动测试房间
...
发现问题：登录失败了！
问题：什么时候改坏的？哪里改坏的？
```

### 有测试后

```
1. 修改代码
2. 运行测试（1 秒钟）
3. 立即发现：UserManagerTest.UserLogin 失败！
4. 定位问题：登录功能改坏了
5. 修复，重新测试
```

### 好处

| 好处 | 说明 |
|------|------|
| **快速发现问题** | 1 秒钟跑完 20 个测试 |
| **精确定位** | 知道哪个功能坏了 |
| **防止回归** | 修改后不会破坏旧功能 |
| **文档作用** | 测试代码展示了功能如何使用 |
| **增强信心** | 测试通过 = 功能正常 |

---

## 📝 最佳实践

### 1. 测试命名

```cpp
// 好的命名
TEST_F(UserManagerTest, RegisterUser)      // 清晰明了
TEST_F(UserManagerTest, LoginWithWrongPassword)  // 描述场景

// 不好的命名
TEST_F(UserManagerTest, Test1)  // 不知道测什么
TEST_F(UserManagerTest, TestUserManagerFunction2)  // 太模糊
```

### 2. 测试独立性

```cpp
// 好的测试 - 独立
TEST_F(UserManagerTest, RegisterUser) {
    // 自己创建测试数据
}

TEST_F(UserManagerTest, LoginUser) {
    // 自己创建测试数据，不依赖其他测试
}

// 不好的测试 - 依赖其他测试
TEST_F(UserManagerTest, Test2) {
    // 依赖 Test1 创建的数据 ❌
}
```

### 3. 测试覆盖

```cpp
// 覆盖正常场景
TEST_F(UserManagerTest, RegisterSuccess) {
    EXPECT_TRUE(user_manager_->register_user("user", "123456"));
}

// 覆盖异常场景
TEST_F(UserManagerTest, RegisterEmptyName) {
    EXPECT_FALSE(user_manager_->register_user("", "123456"));
}

TEST_F(UserManagerTest, RegisterEmptyPassword) {
    EXPECT_FALSE(user_manager_->register_user("user", ""));
}
```

---

## 🔧 常用命令

### 运行所有测试
```bash
./chat_server_test
```

### 运行特定测试套件
```bash
./chat_server_test --gtest_filter="UserManagerTest.*"
```

### 运行单个测试
```bash
./chat_server_test --gtest_filter="UserManagerTest.RegisterUser"
```

### 列出所有测试
```bash
./chat_server_test --gtest_list_tests
```

### 显示详细信息
```bash
./chat_server_test --gtest_print_time=1
```

---

## 📈 代码统计

| 文件 | 测试数 | 代码量 |
|------|--------|--------|
| test_user_manager.cpp | 6 | 100+ 行 |
| test_room_manager.cpp | 5 | 100+ 行 |
| test_message_history.cpp | 3 | 60+ 行 |
| test_conversation.cpp | 6 | 80+ 行 |
| **总计** | **20** | **340+ 行** |

---

_最后更新：2026-03-28_
_版本：v0.5.2_
