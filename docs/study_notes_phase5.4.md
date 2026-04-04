# 学习笔记 - 阶段 5.4：防止重复登录

_版本：v0.5.4_
_最后更新：2026-03-31_

---

## 📚 什么是 Session Token 机制？

### 通俗解释

**Session Token** 就像是用户的"临时身份证"。

想象你去游乐园：
- **门票** = 用户名 + 密码（登录时验证）
- **手环** = Session Token（入园后佩戴）
- **检查点** = 每次请求（验证手环）

如果有人在另一个入口用你的身份买了新手环，你的手环就失效了。

**在软件中**：
- **登录** = 验证用户名密码，生成 token
- **每次请求** = 验证 token 是否有效
- **新登录** = 生成新 token，旧 token 失效

---

## 🔧 实现方案

### 1. 数据库设计

```sql
ALTER TABLE users ADD COLUMN session_token TEXT DEFAULT NULL;
```

- `session_token`: 存储当前活跃会话的 token
- 登录时更新，退出时清除

### 2. 登录流程

```cpp
bool UserManager::login(username, password, user) {
    // 1. 验证用户名密码
    if (!verify_password(username, password)) {
        return false;
    }
    
    // 2. 生成新 token
    std::string new_token = generate_session_token();
    
    // 3. 保存到数据库
    set_session_token(user.id, new_token);
    
    // 4. 返回给用户
    return true;
}
```

### 3. Token 生成

```cpp
std::string UserManager::generate_session_token() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);  // 32 位十六进制随机数
    }
    return ss.str();
}
```

**特点**：
- 32 位十六进制（128 位熵）
- 使用 `std::random_device` 真随机
- 碰撞概率极低

### 4. Token 验证

```cpp
bool UserManager::verify_session_token(user_id, token) {
    // 从数据库查询保存的 token
    std::string db_token = query_token_from_db(user_id);
    
    // 比较 token
    return (token == db_token);
}
```

**验证时机**：
- 每次发送聊天消息
- 执行命令（除登录/注册外）

### 5. 主动通知旧连接

```cpp
void Connection::cmd_login(args) {
    // 登录成功
    if (user_manager_->login(username, password, user)) {
        // 检查是否已在线
        bool was_online = user_manager_->is_online(user.id);
        
        if (was_online) {
            // 查找旧连接
            for (auto& conn : all_connections_) {
                if (conn->session_.user_id == user.id && conn != this) {
                    // 发送通知
                    conn->send_system_message("⚠️ 账号在其他地方登录...");
                    // 延迟关闭
                    std::thread([conn]() {
                        sleep(100ms);
                        conn->close();
                    }).detach();
                }
            }
        }
    }
}
```

---

## 🎨 前端实现

### 1. 检测失效消息

```typescript
// Chat.tsx
if (msgType === 'system') {
  // 精确匹配，避免误判
  if (content.includes('账号在其他地方登录') && 
      content.includes('会话已失效')) {
    setIsSessionExpiredOpen(true)
    return
  }
}
```

### 2. 弹窗组件

```typescript
// SessionExpiredModal.tsx
export default function SessionExpiredModal({ isVisible, onClose }) {
  useEffect(() => {
    if (isVisible) {
      // 清除登录状态
      localStorage.removeItem('user-storage')
      ;(window as any).chatHasLoggedIn = false
    }
  }, [isVisible])

  if (!isVisible) return null

  return (
    <div className="fixed inset-0 bg-black/70 ...">
      <div className="bg-white dark:bg-dark-card ...">
        <h2>账号在其他地方登录</h2>
        <p>您的账号已在其他设备或浏览器窗口登录...</p>
        <button onClick={onClose}>返回登录页面</button>
      </div>
    </div>
  )
}
```

### 3. 防止重复监听器

```typescript
// Chat.tsx
const hasSetupListener = useRef(false)

useEffect(() => {
  // 防止重复设置监听器
  if (hasSetupListener.current) {
    return
  }
  hasSetupListener.current = true
  
  // 设置 WebSocket 监听器
  ws.addEventListener('message', handleMessage)
}, [])
```

---

## 🔍 调试技巧

### 后端日志

```cpp
// 登录时
Logger::info("Generated new token for user " + username + ": " + new_token);
Logger::info("Set session token result: " + std::string(success ? "SUCCESS" : "FAILED"));

// 验证时
Logger::info("Verifying token for user " + user_id + ": " + token);
Logger::info("DB token: " + db_token);
Logger::info("Token match: " + std::string(match ? "YES" : "NO"));
```

### 前端日志

```typescript
console.log('[Chat] Session expired, showing modal')
console.log('[WS] Close event received')
```

---

## ⚠️ 常见问题

### 问题 1：登录后就有弹窗

**原因**：前端检测条件太宽泛

```typescript
// ❌ 错误：使用 || 导致误判
if (content.includes('账号在其他地方登录') || 
    content.includes('会话已失效')) {

// ✅ 正确：使用 && 精确匹配
if (content.includes('账号在其他地方登录') && 
    content.includes('会话已失效')) {
```

### 问题 2：两个账号同时登录没弹窗

**原因**：后端没有主动通知旧连接

**解决**：遍历 `all_connections_` 查找并通知

### 问题 3：发送消息就被弹出

**原因**：token 验证逻辑返回错误

```cpp
// ❌ 错误：指针比较
return db_token != nullptr && token == db_token;

// ✅ 正确：字符串比较
std::string db_token_str = db_token ? db_token : "";
return (token == db_token_str);
```

### 问题 4：消息显示两条

**原因**：重复注册 WebSocket 监听器

**解决**：添加 `hasSetupListener` 标志防止重复

---

## 📊 性能考虑

### Token 生成频率
- 每次登录生成一次
- 不影响日常操作性能

### Token 验证频率
- 每次聊天消息验证一次
- 使用数据库索引优化查询

### 数据库优化
```sql
-- 添加索引加速查询
CREATE INDEX idx_users_session_token ON users(session_token);
```

---

## 🎯 安全考虑

### Token 安全
- ✅ 32 位十六进制（128 位熵）
- ✅ 使用真随机数生成器
- ✅ 碰撞概率极低（≈ 2^-128）

### 传输安全
- ⚠️ 当前使用 WebSocket 明文传输
- ✅ 建议生产环境使用 WSS（WebSocket Secure）

### 存储安全
- ⚠️ 当前 token 明文存储在数据库
- ✅ 可以考虑哈希存储（如 SHA-256）

---

## 📚 参考资料

- [Session Token 最佳实践](https://owasp.org/www-community/Session_Management_Cheat_Sheet)
- [C++ 随机数生成](https://en.cppreference.com/w/cpp/numeric/random)
- [WebSocket 安全](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers#security)

---

_阶段 5.4 完成：防止重复登录_
_下一阶段：性能优化与移动端适配_
