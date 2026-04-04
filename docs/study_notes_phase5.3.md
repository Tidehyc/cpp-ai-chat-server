# 学习笔记 - 阶段 5.3：前端 Bug 修复与优化

_版本：v0.5.3_
_最后更新：2026-03-30_

---

## 📋 本次学习内容

### 1. React Hooks 闭包陷阱

#### 问题描述

在 `useEffect` 中使用的状态变量会被"捕获"，导致总是使用旧值。

#### 实际案例

```typescript
// ❌ 错误示例
useEffect(() => {
  const handleMessage = (event: MessageEvent) => {
    // 这里的 currentRoom 是 useEffect 第一次渲染时的值
    // 即使 currentRoom 改变了，这里还是旧值
    let room = currentRoom || '#general'
  }
}, []) // 空依赖数组，只运行一次
```

#### 解决方案

**方案 1：使用 Store 的 getState 方法**
```typescript
// ✅ 正确示例
useEffect(() => {
  const handleMessage = (event: MessageEvent) => {
    // 实时从 store 获取最新值
    const actualRoom = useRoomStore.getState().currentRoom || '#general'
  }
}, [])
```

**方案 2：添加依赖项**
```typescript
// ✅ 但会导致重复执行
useEffect(() => {
  const handleMessage = (event: MessageEvent) => {
    let room = currentRoom || '#general'
  }
}, [currentRoom]) // currentRoom 改变时重新创建处理器
```

#### 学习要点

- `useEffect` 的依赖数组为空 `[]` 时，内部变量会被"冻结"
- Zustand store 的 `getState()` 可以绕过闭包问题
- 需要权衡：实时性 vs 性能

---

### 2. 登录状态持久化

#### 问题描述

刷新页面后登录状态丢失，跳回登录页。

#### 原因分析

```typescript
// ❌ 问题配置
export const useUserStore = create<UserState>()(
  persist(
    (set) => ({...}),
    {
      name: 'user-storage',
      skipHydration: true, // ← 这导致刷新后不自动恢复数据
    }
  )
)
```

#### 解决方案

**方案 1：移除 skipHydration**
```typescript
// ✅ 正确配置
export const useUserStore = create<UserState>()(
  persist(
    (set) => ({...}),
    {
      name: 'user-storage',
      // 移除 skipHydration，让 Zustand 自动恢复
    }
  )
)
```

**方案 2：手动检查登录状态**
```typescript
// ✅ 在 ProtectedRoute 中
function ProtectedRoute({ children }) {
  const { isLoggedIn, username } = useUserStore()
  
  // 检查 localStorage 中是否有数据
  if (!isLoggedIn && !username) {
    return <Navigate to="/login" />
  }
  
  return <>{children}</>
}
```

#### 学习要点

- `persist` 中间件默认会从 localStorage 恢复数据
- `skipHydration: true` 会阻止自动恢复
- 可以结合使用 `isLoggedIn` 和 `username` 判断登录状态

---

### 3. 避免重复登录

#### 问题描述

每次页面渲染都会自动登录，重置用户状态为 `online`。

#### 原因分析

```typescript
// ❌ 问题代码
useEffect(() => {
  if (existingWS) {
    // 每次渲染都会发送登录命令
    ws.send(`/login ${username} ${password}`)
  }
}, []) // 虽然只执行一次，但多次渲染会创建多个连接
```

#### 解决方案

**使用全局标志**
```typescript
// ✅ 正确做法
useEffect(() => {
  const hasLoggedIn = (window as any).chatHasLoggedIn
  
  if (!hasLoggedIn && existingWS) {
    ws.send(`/login ${username} ${password}`)
    ;(window as any).chatHasLoggedIn = true // 标记已登录
  }
}, [])
```

#### 学习要点

- 使用 `window` 对象存储全局状态
- 登录命令会重置用户状态，避免重复发送
- 可以在多个页面间共享登录状态

---

### 4. 组件布局优化

#### 问题描述

每个页面都有自己的侧边栏代码，导致代码重复且侧边栏会消失。

#### 解决方案

**创建通用布局组件**
```typescript
// components/PageLayout.tsx
export default function PageLayout({ children }) {
  return (
    <div className="flex h-screen bg-gray-100 dark:bg-dark-bg">
      <Sidebar />
      <div className="flex-1 flex flex-col overflow-hidden">
        {children}
      </div>
    </div>
  )
}
```

**在所有页面中使用**
```typescript
// pages/Contact.tsx
export default function Contact() {
  return (
    <PageLayout>
      {/* 页面内容 */}
    </PageLayout>
  )
}
```

#### 学习要点

- 使用组合模式避免代码重复
- 布局组件提供一致的用户体验
- 便于统一修改侧边栏样式

---

### 5. WebSocket 消息处理

#### 问题描述

多个页面都监听 `online_users` 消息，导致重复处理。

#### 解决方案

**统一消息处理逻辑**
```typescript
// ✅ 在 Chat 页面处理
if (data.type === 'online_users' && data.users) {
  setOnlineUsers(data.users)
  
  // 同步更新当前用户状态
  const currentUser = data.users.find(u => u.username === username)
  if (currentUser && currentUser.status !== status) {
    setStatus(currentUser.status as any)
  }
}
```

**Contact 页面只接收**
```typescript
// ✅ Contact 页面
const handleOnlineUsers = (event: MessageEvent) => {
  const data = JSON.parse(event.data)
  if (data.type === 'online_users' && data.users) {
    setOnlineUsers(data.users.map(u => ({
      username: u.username || u.name || 'unknown',
      status: u.status as string
    })))
  }
}
```

#### 学习要点

- 明确各页面的职责
- 避免重复处理同一消息
- 保持状态同步

---

## 🔧 技术要点总结

### React Hooks

1. **useEffect 依赖陷阱**
   - 空依赖数组会"冻结"变量
   - 使用 store 的 `getState()` 获取实时值

2. **useEffect 清理**
   - 及时移除事件监听器
   - 避免内存泄漏

3. **useState vs useStore**
   - 组件内状态用 `useState`
   - 跨组件状态用 `useStore`

### Zustand

1. **persist 中间件**
   - 自动持久化到 localStorage
   - 注意 `skipHydration` 的影响

2. **状态同步**
   - 使用 `getState()` 获取最新状态
   - 使用 `subscribe()` 监听变化

### WebSocket

1. **连接管理**
   - 使用 `window` 对象共享连接
   - 避免重复创建连接

2. **消息处理**
   - 统一消息格式（JSON）
   - 明确各页面处理职责

---

## 📊 性能优化

### 避免重复渲染

```typescript
// ✅ 使用 useMemo 缓存计算结果
const groupedUsers = useMemo(() => ({
  online: users.filter(u => u.status === 'online'),
  busy: users.filter(u => u.status === 'busy'),
  away: users.filter(u => u.status === 'away'),
  offline: users.filter(u => u.status === 'offline'),
}), [users])
```

### 避免重复请求

```typescript
// ✅ 只在挂载时请求一次
useEffect(() => {
  ws.send('/users')
}, []) // 空依赖数组
```

---

## 🎯 收获与感悟

### 1. 闭包陷阱是常见 Bug 源

- React Hooks 的闭包特性容易导致使用旧值
- 需要特别注意 `useEffect` 中的变量引用
- Zustand 的 `getState()` 是好用的解决方案

### 2. 登录状态管理要谨慎

- 登录命令会重置状态，避免重复发送
- 使用全局标志管理登录状态
- 持久化配置要正确

### 3. 代码复用很重要

- 通用布局组件减少代码重复
- 统一的消息处理逻辑
- 便于维护和修改

### 4. 调试日志帮助定位问题

- 添加详细日志追踪状态变化
- 使用浏览器开发者工具
- 后端日志配合前端调试

---

## 📚 参考资料

- [React Hooks 官方文档](https://react.dev/reference/react)
- [Zustand 文档](https://github.com/pmndrs/zustand)
- [React 闭包陷阱](https://dmitripavlutin.com/react-hooks-closure-trap/)
- [WebSocket API](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket)

---

_阶段 5.3 完成：前端 Bug 修复与优化_
_下一阶段：功能完善与性能优化_
