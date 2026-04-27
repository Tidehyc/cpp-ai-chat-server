# C++ AI Chat Frontend

基于 React + TypeScript 的 Web 前端，为 C++ AI Chat Server 提供类似微信/QQ 的聊天界面。

## 🚀 快速开始

### 安装依赖

```bash
npm install
```

### 启动开发服务器

```bash
npm run dev
```

访问 http://localhost:5173

### 生产构建

```bash
npm run build
```

## 📁 项目结构

```
frontend/
├── src/
│   ├── components/         # 可复用组件
│   │   ├── Sidebar.tsx     # 侧边栏
│   │   ├── ChatList.tsx    # 会话列表
│   │   └── ChatArea.tsx    # 聊天区域
│   ├── pages/              # 页面组件
│   │   ├── Login.tsx       # 登录页
│   │   ├── Chat.tsx        # 聊天页
│   │   ├── Contact.tsx     # 联系人页
│   │   ├── AIAssistant.tsx # AI 助手页
│   │   └── Settings.tsx    # 设置页
│   ├── stores/             # Zustand 状态管理
│   │   ├── userStore.ts    # 用户状态
│   │   ├── messageStore.ts # 消息状态
│   │   └── roomStore.ts    # 房间状态
│   ├── services/           # API 服务
│   │   ├── websocket.ts    # WebSocket 服务
│   │   └── api.ts          # HTTP API
│   ├── hooks/              # 自定义 Hooks
│   │   └── useWebSocket.ts # WebSocket Hook
│   ├── types/              # TypeScript 类型
│   └── App.tsx             # 根组件
└── package.json
```

## 🛠️ 技术栈

| 技术 | 用途 |
|------|------|
| React 18 | UI 框架 |
| TypeScript | 类型系统 |
| Vite | 构建工具 |
| Tailwind CSS | 样式 |
| Zustand | 状态管理 |
| React Router | 路由 |
| React Markdown | Markdown 渲染 |
| Syntax Highlighter | 代码高亮 |

## 🎨 图标规范（已锁定）

前端已统一使用 `lucide-react` 线性图标，禁止在 UI 文案中继续引入 emoji 作为功能图标。

- 规范文档：`frontend/ICON_GUIDELINES.md`
- 适用范围：`src/pages/`、`src/components/`、`src/stores/` 中所有会渲染到 UI 的图标或状态标识
- 原则：统一尺寸、统一线宽、统一颜色语义、统一交互反馈

## 📋 功能列表

### 第一阶段 (MVP) ✅

- [x] 项目脚手架搭建
- [x] 登录/注册页面
- [x] 聊天主界面
- [x] 侧边栏导航
- [x] 会话列表
- [x] WebSocket 服务
- [x] 状态管理

### 第二阶段 (开发中)

- [ ] WebSocket 消息收发
- [ ] 实时消息显示
- [ ] @AI 功能
- [ ] Markdown 渲染
- [ ] 代码高亮

### 第三阶段 (计划)

- [ ] 深色主题切换
- [ ] 联系人列表
- [ ] 消息搜索
- [ ] 浏览器通知

## 🔗 后端连接

默认情况下，前端连接同源的 `/ws`。在 HTTPS 页面中会自动切换为 WSS。

如果你需要手动指定后端地址，可以设置 `VITE_WS_URL`，例如：

```bash
VITE_WS_URL=ws://localhost:8080/ws
```

或在公网部署时使用：

```bash
VITE_WS_URL=wss://your-domain.com/ws
```

```bash
# 启动后端
cd ../build
./chat_server
```

## 📄 License

MIT
