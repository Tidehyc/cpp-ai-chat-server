import { useState, useEffect, useRef, useCallback } from 'react'
import { useNavigate, Link } from 'react-router-dom'
import { useUserStore } from '../stores/userStore'
import { useFriendStore } from '../stores/friendStore'
import { useMessageStore } from '../stores/messageStore'
import { useRoomStore } from '../stores/roomStore'
import { wsService } from '../services/websocket'
import { ArrowLeft, Bot, Check, CheckCheck, CheckCircle2, ChevronDown, CircleAlert, CircleX, Download, History, Mail, MessageSquare, Plus, Settings, Trash2, User, UserPlus, Users, X } from 'lucide-react'
import type { ConnectionStatus } from '../types'
import type { FriendRequest } from '../types/friend'
import { useSmartScroll } from '../hooks/useSmartScroll'

// 模态框组件
import ContextModal from '../components/ContextModal'
import HistoryModal from '../components/HistoryModal'
import SearchModal from '../components/SearchModal'
import ExportModal from '../components/ExportModal'
import SessionExpiredModal from '../components/SessionExpiredModal'
import MarkdownMessage from '../components/MarkdownMessage'
import MobileTabBar from '../components/MobileTabBar'

export default function Chat() {
  const navigate = useNavigate()

  // Store 状态
  const { username, status, showLogoutConfirm, setShowLogoutConfirm, logout } = useUserStore()
  
  const {
    friends, pendingRequests, selectedFriend, 
    setSelectedFriend, setSearchQuery, getFilteredFriends,
    addPendingRequest, acceptRequest, rejectRequest
  } = useFriendStore()
  const { addMessage, getMessages, setCurrentChatTarget } = useMessageStore()
  const { setCurrentRoom } = useRoomStore()

  // 本地状态
  const [inputMessage, setInputMessage] = useState('')
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('disconnected')
  const [showFriendRequestModal, setShowFriendRequestModal] = useState(false)
  const [newFriendUsername, setNewFriendUsername] = useState('')
  const [friendRequestStatus, setFriendRequestStatus] = useState<'idle' | 'sending' | 'success' | 'error'>('idle')
  const [friendRequestMessage, setFriendRequestMessage] = useState('')
  const [isMobile, setIsMobile] = useState(() => typeof window !== 'undefined' && window.innerWidth < 768)
  
  // 模态框状态
  const [showContextModal, setShowContextModal] = useState(false)
  const [showHistoryModal, setShowHistoryModal] = useState(false)
  const [showSearchModal, setShowSearchModal] = useState(false)
  const [showExportModal, setShowExportModal] = useState(false)
  const [showSessionExpired, setShowSessionExpired] = useState(false)

  // 消息列表 ref
  const inputRef = useRef<HTMLTextAreaElement>(null)
  const selectedFriendRef = useRef<string | null>(null)
  const lastSentTargetRef = useRef<string | null>(null) // 记录最后发送的目标好友
  const {
    containerRef: messageContainerRef,
    endRef: messagesEndRef,
    showScrollButton,
    handleScroll,
    scrollToBottom,
    handleContentChange,
  } = useSmartScroll()

  // 同步 selectedFriend 到 ref
  useEffect(() => {
    selectedFriendRef.current = selectedFriend
  }, [selectedFriend])

  // 好友上下线通知 ref
  const handleFriendOnlineRef = useRef<(msg: any) => void>(() => {})
  const handleFriendOfflineRef = useRef<(msg: any) => void>(() => {})

  // 过滤后的好友列表
  const filteredFriends = getFilteredFriends()

  // 当前聊天消息
  const currentMessages = selectedFriend ? getMessages(selectedFriend) : []

  useEffect(() => {
    if (selectedFriend) {
      scrollToBottom('auto')
    }
  }, [selectedFriend, scrollToBottom])

  useEffect(() => {
    handleContentChange()
  }, [currentMessages, handleContentChange])

  useEffect(() => {
    const onResize = () => setIsMobile(window.innerWidth < 768)
    window.addEventListener('resize', onResize)
    return () => window.removeEventListener('resize', onResize)
  }, [])

  // 处理好友列表 - 使用 useCallback 包装
  const handleFriendList = useCallback((message: any) => {
    const friends = message.raw?.friends || []
    if (friends.length > 0) {
      const friendList = friends.map((f: any) => ({
        id: 0,
        username: f.username,
        status: (f.status === 'online' || f.status === 'busy' || f.status === 'away' || f.status === 'offline') ? f.status : 'online',
        createdAt: new Date().toISOString(),
      }))
      useFriendStore.getState().setFriends(friendList)
    } else {
      useFriendStore.getState().setFriends([])
    }
  }, [])

  // 处理收到的私聊消息
  const handlePrivateMessage = useCallback((message: any) => {
    const fromUser = message.from_user || message.from
    const content = message.content
    const isRead = message.is_read || false

    addMessage(fromUser, {
      type: 'private',
      from_user: fromUser,
      to_user: username || undefined,
      content,
      timestamp: message.timestamp || new Date().toLocaleString('zh-CN'),
      is_own: false,
      is_read: isRead,
    }, false)

    // 如果当前正在与该用户聊天，标记消息为已读（使用防抖）
    if (selectedFriendRef.current === fromUser) {
      // 延迟 100ms 发送，避免频繁调用
      setTimeout(() => {
        // 再次检查当前是否仍在与该用户聊天
        if (selectedFriendRef.current === fromUser) {
          wsService.sendCommand(`/read ${fromUser}`)
        }
      }, 100)
    }
  }, [addMessage, username])

  // 处理发送的消息
  const handleSentMessage = useCallback((message: any) => {
    // sent_message 仅表示服务器已接收，先标记为已送达
    const toUser = message.to
    if (toUser) {
      useMessageStore.getState().markLatestOwnMessageDelivered(toUser)
    }
  }, [])

  // 处理已读确认
  const handleReadConfirm = useCallback((message: any) => {
    const fromUser = message.from_user || message.raw?.from
    console.log(`[Chat] Messages from ${fromUser} marked as read`)
    
    if (fromUser) {
      useMessageStore.getState().markMessagesRead(fromUser)
    }
  }, [])

  // 处理已读通知（对方读了你的消息）
  const handleReadNotify = useCallback((message: any) => {
    const reader = message.from_user || message.raw?.from
    console.log(`[Chat] ${reader} read your messages`)
    
    if (reader) {
      useMessageStore.getState().markMessagesRead(reader)
    }
  }, [])

  // 处理系统消息
  const handleSystemMessage = useCallback((message: any) => {
    const content = message.content || ''

    // 好友请求发送成功
    if (content.includes('好友请求已发送')) {
      setFriendRequestStatus('success')
      setFriendRequestMessage('好友请求已发送，等待对方同意！')

      // 1.5 秒后关闭弹窗
      setTimeout(() => {
        setShowFriendRequestModal(false)
        setNewFriendUsername('')
        setFriendRequestStatus('idle')
        setFriendRequestMessage('')
      }, 1500)
      return
    }

    // 已经是好友
    if (content.includes('已经是好友')) {
      setFriendRequestStatus('error')
      setFriendRequestMessage('你们已经是好友了，无需重复添加')
      return
    }

    // 好友请求发送失败
    if (content.includes('失败') || content.includes('已发送过')) {
      setFriendRequestStatus('error')
      setFriendRequestMessage(content.includes('已发送过') ? '你已经发送过好友请求了' : '发送失败，请稍后重试')
      return
    }

    // 好友请求通知 - 兼容有无前缀图标的文案
    if (content.includes('向你发送了好友请求')) {
      const normalizedContent = content.replace(/^[^\p{L}\p{N}]+\s*/u, '')
      const match = normalizedContent.match(/(\S+) 向你发送了好友请求/)
      if (match) {
        const senderUsername = match[1]
        console.log('[Chat] Received friend request from:', senderUsername)
        addPendingRequest({
          id: Date.now(),
          senderId: 0,
          senderUsername,
          status: 'pending',
          createdAt: new Date().toISOString(),
        })
      }
      return
    }

    // 好友请求被接受
    if (content.includes('接受了') || content.includes('accepted')) {
      const match = content.match(/(\S+) 接受了/)
      if (match) {
        // 添加好友到列表
      }
    }

    // 不是好友，无法发送私聊
    if (content.includes('不是好友') || content.includes('无法发送私聊')) {
      // 使用 lastSentTargetRef 获取发送消息时的目标好友
      const targetFriend = lastSentTargetRef.current || selectedFriendRef.current
      
      addMessage(targetFriend || '', {
        type: 'system',
        from_user: '系统',
        to_user: username || undefined,
        content: '发送失败：' + content,
        timestamp: new Date().toLocaleString('zh-CN'),
        is_own: false,
        is_error: true,
      })
      return
    }

    // 登出提示
    if (content.includes('已退出') || content.includes('logged out')) {
      useUserStore.getState().logout()
      localStorage.removeItem('user-storage')
      navigate('/login')
    }
  }, [selectedFriend, username, addMessage, addPendingRequest, navigate])

  // 初始化 WebSocket 连接
  useEffect(() => {
    console.log('[Chat] useEffect: Setting up WebSocket listeners')
    console.log('[Chat] Current user:', username)

    const initWebSocket = async () => {
      try {
        setConnectionStatus('connecting')
        await wsService.connect()
        setConnectionStatus('connected')

        if (username) {
          console.log('[Chat] User restored from localStorage:', username)
        }
      } catch (err) {
        console.error('[Chat] WebSocket connection failed:', err)
        setConnectionStatus('disconnected')
      }
    }

    initWebSocket()

    // 监听连接状态
    const unsubscribeStatus = wsService.onConnectionStatusChange((status) => {
      setConnectionStatus(status)
    })

    // 监听登录成功
    const checkLoginSuccess = (message: any) => {
      const content = message.content || ''
      if (content.includes('登录成功') || content.includes('Login successful')) {
        console.log('[Chat] Login successful')
      }
      // 登录失败 - 只有在已登录状态下才处理（会话过期情况）
      if ((content.includes('登录失败') || content.includes('Login failed') ||
          content.includes('用户不存在') || content.includes('密码错误')) &&
          useUserStore.getState().isLoggedIn) {
        console.log('[Chat] Login failed while logged in, session expired')
        localStorage.removeItem('user-storage')
        useUserStore.getState().clear()
        navigate('/login')
      }
      // 未登录提示
      if (content.includes('请先登录')) {
        console.log('[Chat] Session expired, redirecting to login')
        localStorage.removeItem('user-storage')
        useUserStore.getState().clear()
        navigate('/login')
      }
    }

    wsService.on('system', checkLoginSuccess)

    // 设置好友上下线处理器
    handleFriendOnlineRef.current = (msg: any) => {
      const username = msg.raw?.username || msg.username
      if (username) {
        useFriendStore.getState().updateFriendStatus(username, 'online')
      }
    }

    handleFriendOfflineRef.current = (msg: any) => {
      const username = msg.raw?.username || msg.username
      if (username) {
        useFriendStore.getState().updateFriendStatus(username, 'offline')
      }
    }

    // 注册监听器
    wsService.on('friend_list', handleFriendList)
    wsService.on('private_message', handlePrivateMessage)
    wsService.on('sent_message', handleSentMessage)
    wsService.on('system', handleSystemMessage)
    wsService.on('read_confirm', handleReadConfirm)
    wsService.on('read_notify', handleReadNotify)
    wsService.on('friend_online', (msg) => handleFriendOnlineRef.current(msg))
    wsService.on('friend_offline', (msg) => handleFriendOfflineRef.current(msg))

    // 主动获取好友列表
    wsService.getFriendList()

    return () => {
      unsubscribeStatus()
      wsService.off('system', checkLoginSuccess)
      wsService.off('friend_list', handleFriendList)
      wsService.off('private_message', handlePrivateMessage)
      wsService.off('sent_message', handleSentMessage)
      wsService.off('system', handleSystemMessage)
      wsService.off('read_confirm', handleReadConfirm)
      wsService.off('read_notify', handleReadNotify)
    }
  }, [])

  // 同意好友请求
  const handleAcceptRequest = (senderUsername: string) => {
    wsService.acceptFriendRequest(senderUsername)
    acceptRequest(senderUsername)
  }

  // 拒绝好友请求
  const handleRejectRequest = (senderUsername: string) => {
    wsService.rejectFriendRequest(senderUsername)
    rejectRequest(senderUsername)
  }

  // 登出
  const handleLogout = () => {
    wsService.logout()
    logout()
    localStorage.removeItem('user-storage')
    navigate('/login')
  }

  // 发送消息
  const handleSendMessage = () => {
    if (!inputMessage.trim() || !selectedFriend) return

    const content = inputMessage.trim()

    // 记录最后发送的目标好友
    lastSentTargetRef.current = selectedFriend

    // 添加自己的消息到 UI
    addMessage(selectedFriend, {
      type: 'private',
      from_user: username || 'me',
      to_user: selectedFriend,
      content,
      timestamp: new Date().toLocaleString('zh-CN'),
      is_own: true,
      is_delivered: false,
      is_read: false,
    }, true) // isOwn = true

    // 发送私聊命令
    wsService.sendPrivateMessage(selectedFriend, content)

    setInputMessage('')
  }

  // 处理键盘事件
  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault()
      handleSendMessage()
    }
  }

  // 选择好友
  const handleSelectFriend = (friendUsername: string) => {
    setSelectedFriend(friendUsername)
    setCurrentChatTarget(friendUsername, 'private')
    setCurrentRoom(null) // 离开群聊房间

    // 标记消息为已读
    wsService.sendCommand(`/read ${friendUsername}`)
  }

  const handleBackToFriendList = () => {
    setSelectedFriend(null)
    setCurrentChatTarget(null)
  }

  // 添加好友
  const handleAddFriend = () => {
    if (!newFriendUsername.trim()) {
      setFriendRequestStatus('error')
      setFriendRequestMessage('请输入用户名')
      return
    }

    setFriendRequestStatus('sending')
    setFriendRequestMessage('')
    wsService.sendFriendRequest(newFriendUsername.trim())
  }

  const getStatusText = () => {
    switch (connectionStatus) {
      case 'connected': return '已连接'
      case 'connecting': return '连接中...'
      case 'disconnected': return '未连接'
    }
  }

  const getStatusColor = () => {
    switch (connectionStatus) {
      case 'connected': return 'text-green-600'
      case 'connecting': return 'text-yellow-600'
      case 'disconnected': return 'text-red-600'
    }
  }

  // 用户状态图标
  const getStatusDotClass = (userStatus: string) => {
    switch (userStatus) {
      case 'online': return 'status-dot status-dot-online'
      case 'busy': return 'status-dot status-dot-busy'
      case 'away': return 'status-dot status-dot-away'
      case 'offline': return 'status-dot status-dot-offline'
      default: return 'status-dot status-dot-offline'
    }
  }

  const showFriendListPanel = !isMobile || !selectedFriend
  const showChatPanel = !isMobile || Boolean(selectedFriend)

  return (
    <div className="chat-shell page-enter h-[100dvh] md:h-screen flex flex-col md:flex-row">
      {/* 左侧边栏 - 好友列表 */}
      <div className={`${showFriendListPanel ? 'flex' : 'hidden md:flex'} chat-sidebar w-full md:w-80 lg:w-96 flex-col h-full`}>
        {/* 标题栏 */}
        <div className="chat-sidebar-header h-16 flex items-center justify-between px-5">
          <h1 className="chat-title text-lg font-semibold">好友聊天</h1>
          <button
            onClick={() => {
              setShowFriendRequestModal(true)
              setFriendRequestStatus('idle')
              setFriendRequestMessage('')
              setNewFriendUsername('')
            }}
            className="h-10 w-10 rounded-xl border theme-divider theme-surface text-lg transition-all hover:-translate-y-0.5 hover:bg-white/90"
            title="添加好友"
          >
              <Plus size={18} strokeWidth={2.2} />
          </button>
        </div>

        {/* 待处理好友请求 */}
        {pendingRequests.length > 0 && (
          <div className="border-b border-white/30 bg-amber-50/80 p-3 backdrop-blur-sm">
            <div className="mb-2 flex items-center font-medium text-amber-900">
              <Mail size={15} />
              <span className="ml-2">待处理好友请求 ({pendingRequests.length})</span>
            </div>
            {pendingRequests.map((request) => (
              <div key={request.id} className="mb-2 rounded-xl border border-amber-200 bg-white/90 p-2 shadow-sm">
                <p className="mb-2 text-sm theme-muted">
                  <span className="font-medium">{request.senderUsername}</span> 请求添加你为好友
                </p>
                <div className="flex space-x-2">
                  <button
                    onClick={() => handleAcceptRequest(request.senderUsername)}
                    className="flex-1 rounded-lg bg-emerald-600 px-3 py-1 text-sm text-white transition-colors hover:bg-emerald-700"
                  >
                    <span className="icon-label justify-center"><Check size={14} />同意</span>
                  </button>
                  <button
                    onClick={() => handleRejectRequest(request.senderUsername)}
                    className="flex-1 rounded-lg bg-rose-600 px-3 py-1 text-sm text-white transition-colors hover:bg-rose-700"
                  >
                    <span className="icon-label justify-center"><X size={14} />拒绝</span>
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}

        {/* 搜索框 */}
        <div className="chat-search-wrap p-3">
          <input
            type="text"
            placeholder="搜索好友..."
            value={useFriendStore((state) => state.searchQuery)}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full rounded-xl border theme-divider theme-surface px-4 py-2.5 text-sm text-inherit shadow-sm outline-none transition-all focus:border-cyan-400 focus:bg-white focus:ring-2 focus:ring-cyan-200"
          />
        </div>

        {/* 好友列表 */}
        <div className="chat-friend-list flex-1 overflow-y-auto">
          {filteredFriends.length === 0 ? (
            <div className="p-4 text-center theme-muted text-sm">
              {friends.length === 0 ? '暂无好友，点击右上角添加' : '没有找到匹配的好友'}
            </div>
          ) : (
            filteredFriends.map((friend) => {
              const unreadCount = useMessageStore.getState().getUnreadCount(friend.username)
              
              return (
                <div
                  key={friend.username}
                  className="group flex items-center"
                >
                  <button
                    onClick={() => handleSelectFriend(friend.username)}
                    className={`chat-room-item flex-1 ${
                      selectedFriend === friend.username ? 'is-active' : ''
                    }`}
                  >
                    <span className={`mr-3 ${getStatusDotClass(friend.status)}`} />
                    <div className="flex-1 text-left">
                      <div className="flex items-center justify-between">
                        <p className={`text-sm font-medium ${unreadCount > 0 ? 'font-bold text-inherit' : 'text-inherit'}`}>
                          {friend.username}
                        </p>
                        {/* 未读消息提示 */}
                        {unreadCount > 0 && (
                          <span className="min-w-[20px] rounded-full bg-rose-500 px-2 py-0.5 text-center text-xs font-bold text-white">
                            {unreadCount > 99 ? '99+' : unreadCount}
                          </span>
                        )}
                      </div>
                      <p className="text-xs theme-muted">
                        {friend.status === 'online' ? '在线' :
                         friend.status === 'busy' ? '忙碌' :
                         friend.status === 'away' ? '离开' : '离线'}
                      </p>
                    </div>
                  </button>
                  {/* 删除好友按钮 */}
                  <button
                    onClick={(e) => {
                      e.stopPropagation()
                      if (window.confirm(`确定要删除好友 ${friend.username} 吗？`)) {
                        useFriendStore.getState().removeFriend(friend.username)
                        wsService.deleteFriend(friend.username)
                        // 如果删除的是当前选中的好友，清空聊天区域
                        if (selectedFriend === friend.username) {
                          setSelectedFriend(null)
                        }
                      }
                    }}
                    className="mr-2 hidden rounded-lg p-2 text-rose-500 transition-colors hover:bg-rose-50 group-hover:flex"
                    title="删除好友"
                  >
                    <Trash2 size={15} />
                  </button>
                </div>
              )
            })
          )}
        </div>

        {/* 底部用户信息 */}
        <div className="chat-userbar h-14 flex items-center justify-between px-4">
          <div className="flex items-center">
            <span className={`mr-2 ${getStatusDotClass(status)}`} />
            <span className="text-sm font-medium theme-muted">{username}</span>
            <span className="ml-2 text-xs theme-muted">
              {status === 'online' ? '在线' :
               status === 'busy' ? '忙碌' :
               status === 'away' ? '离开' :
               status === 'offline' ? '离线' : '在线'}
            </span>
          </div>
          <button
            onClick={() => setShowLogoutConfirm(true)}
            className="text-sm font-medium text-rose-600 transition-colors hover:text-rose-700"
          >
            退出
          </button>
        </div>
      </div>

      {/* 右侧聊天区域 */}
      <div className={`${showChatPanel ? 'flex' : 'hidden md:flex'} chat-main flex-1 min-h-0 flex-col`}>
        {/* 顶部导航栏 */}
        <div className="chat-topnav h-14 flex items-center justify-between px-6">
          <div className="flex items-center space-x-4">
            <span className="topbar-title">私聊</span>
            <nav className="no-scrollbar hidden md:flex items-center space-x-2 overflow-x-auto pr-1">
              <Link
                to="/contact"
                className="nav-pill"
                title="群聊"
              >
                <span className="icon-label"><Users size={14} />群聊</span>
              </Link>
              <Link
                to="/ai"
                className="nav-pill"
                title="AI 助手"
              >
                <span className="icon-label"><Bot size={14} />AI</span>
              </Link>
              <Link
                to="/settings"
                className="nav-pill"
                title="设置"
              >
                <span className="icon-label"><Settings size={14} />设置</span>
              </Link>
            </nav>
          </div>
          <div className="hidden md:flex items-center space-x-2">
            <span className={`text-sm ${getStatusColor()}`}>
              <span className={`inline-block mr-1.5 align-middle ${getStatusDotClass(status)}`} />
              {status === 'online' ? '在线' :
               status === 'busy' ? '忙碌' :
               status === 'away' ? '离开' : '离线'}
            </span>
          </div>
        </div>

        {/* 聊天顶部栏 */}
        <div className="chat-roombar h-14 flex items-center justify-between px-6">
          <div className="flex items-center">
            {selectedFriend ? (
              <>
                {isMobile && (
                  <button
                    onClick={handleBackToFriendList}
                    className="mr-2 rounded-full p-1 theme-muted hover:bg-white/60"
                    title="返回会话列表"
                  >
                    <ArrowLeft size={16} />
                  </button>
                )}
                <MessageSquare size={18} className="mr-2 text-cyan-700" />
                <span className="font-semibold text-inherit">{selectedFriend}</span>
                <span className={`ml-3 text-sm ${getStatusColor()}`}>
                  <span className={`inline-block mr-1.5 align-middle ${getStatusDotClass(friends.find(f => f.username === selectedFriend)?.status || 'offline')}`} />
                  {getStatusText()}
                </span>
              </>
            ) : (
              <span className="theme-muted">选择一个好友开始聊天</span>
            )}
          </div>

          {selectedFriend && (
            <div className="flex items-center space-x-2">
              <button
                onClick={() => setShowHistoryModal(true)}
                className="action-chip"
                title="历史记录"
              >
                <span className="icon-label"><History size={14} />历史</span>
              </button>
              <button
                onClick={() => setShowExportModal(true)}
                className="action-chip"
                title="导出记录"
              >
                <span className="icon-label"><Download size={14} />导出</span>
              </button>
            </div>
          )}
        </div>

        {/* 消息列表 */}
        <div ref={messageContainerRef} onScroll={handleScroll} className="chat-thread relative flex-1 overflow-y-auto p-3 md:p-6">
          {selectedFriend ? (
            currentMessages.length === 0 ? (
              <div className="h-full flex items-center justify-center text-slate-400">
                <div className="text-center">
                  <MessageSquare size={34} className="mx-auto mb-4 text-slate-300" />
                  <p>还没有消息</p>
                  <p className="text-sm">发送一条消息开始聊天吧</p>
                </div>
              </div>
            ) : (
              <div className="space-y-4">
                {currentMessages.map((msg, index) => (
                  <div
                    key={index}
                    className={`flex ${msg.is_own ? 'justify-end' : 'justify-start'} ${msg.is_error ? 'justify-center' : ''}`}
                  >
                    {msg.is_error ? (
                      <div className="system-pill flex items-center space-x-2">
                        <CircleAlert size={16} className="text-rose-500" />
                        <p>{msg.content}</p>
                        <span className="text-[11px] text-rose-400">{msg.timestamp}</span>
                      </div>
                    ) : (
                      <div
                        className={`message-bubble msg-bubble ${
                          msg.is_own
                            ? 'msg-bubble-own'
                            : 'msg-bubble-other'
                        }`}
                      >
                        {!msg.is_own && (
                          <p className="msg-sender">
                            <User size={12} />
                            <span>{msg.from_user}</span>
                          </p>
                        )}
                        <MarkdownMessage content={msg.content} />
                        <div className={`msg-meta ${msg.is_own ? 'msg-meta-own' : 'msg-meta-other'}`}>
                          <p>
                            {msg.timestamp}
                          </p>
                          {/* 自己发送的消息显示已读状态 */}
                          {msg.is_own && (
                            <span title={msg.is_read ? '已读' : msg.is_delivered ? '已送达' : '发送中'}>
                              {msg.is_read ? (
                                <CheckCheck size={13} />
                              ) : (
                                <Check size={13} className={msg.is_delivered ? '' : 'opacity-50'} />
                              )}
                            </span>
                          )}
                        </div>
                      </div>
                    )}
                  </div>
                ))}
                <div ref={messagesEndRef} />
              </div>
            )
          ) : (
            <div className="h-full flex items-center justify-center text-slate-400">
              <div className="text-center">
                <ArrowLeft size={42} className="mx-auto mb-4 text-slate-300" />
                <p className="text-lg">从左侧选择一个好友</p>
                <p className="text-sm mt-2">开始私聊对话</p>
              </div>
            </div>
          )}

          {selectedFriend && showScrollButton && (
            <button
              onClick={() => scrollToBottom('smooth')}
              className="absolute bottom-4 right-4 rounded-full bg-slate-900/80 p-2 text-white shadow-lg transition hover:bg-slate-800"
              title="回到底部"
            >
              <ChevronDown size={16} />
            </button>
          )}
        </div>

        {/* 输入框 */}
        {selectedFriend && (
          <div className="chat-composer sticky bottom-0 z-10 p-3 md:p-4">
            <div className="flex items-end space-x-3">
              <textarea
                ref={inputRef}
                value={inputMessage}
                onChange={(e) => setInputMessage(e.target.value)}
                onKeyDown={handleKeyDown}
                placeholder="输入消息... (Enter 发送，Shift+Enter 换行)"
                rows={1}
                className="flex-1 resize-none rounded-xl border theme-divider theme-surface px-4 py-3 text-base md:text-sm text-inherit shadow-sm outline-none transition-all focus:border-cyan-400 focus:bg-white focus:ring-2 focus:ring-cyan-200"
                style={{ minHeight: '48px', maxHeight: '120px' }}
              />
              <button
                onClick={handleSendMessage}
                disabled={!inputMessage.trim() || connectionStatus !== 'connected'}
                className="rounded-xl bg-gradient-to-r from-cyan-600 to-sky-600 px-6 py-3 font-medium text-white transition-all hover:from-cyan-500 hover:to-sky-500 disabled:cursor-not-allowed disabled:from-slate-400 disabled:to-slate-400"
              >
                发送
              </button>
            </div>
          </div>
        )}
      </div>

      <MobileTabBar />

      {/* 添加好友模态框 */}
      {showFriendRequestModal && (
        <div className="modal-overlay" onClick={(e) => {
          // 点击背景关闭弹窗
          if (e.target === e.currentTarget) {
            setShowFriendRequestModal(false)
            setNewFriendUsername('')
            setFriendRequestStatus('idle')
            setFriendRequestMessage('')
          }
        }}>
          <div className="modal-card max-w-md">
            <h3 className="modal-title icon-label"><UserPlus size={18} />添加好友</h3>

            {/* 状态提示 */}
            {friendRequestStatus === 'success' && (
              <div className="mb-4 border border-emerald-200 bg-emerald-50/90 text-emerald-700 px-4 py-3 rounded-lg text-sm flex items-center">
                <CheckCircle2 size={16} className="mr-2" />
                {friendRequestMessage}
              </div>
            )}

            {friendRequestStatus === 'error' && (
              <div className="mb-4 border border-rose-200 bg-rose-50/90 text-rose-700 px-4 py-3 rounded-lg text-sm flex items-center">
                <CircleX size={16} className="mr-2" />
                {friendRequestMessage}
              </div>
            )}

            <input
              type="text"
              value={newFriendUsername}
              onChange={(e) => {
                setNewFriendUsername(e.target.value)
                setFriendRequestStatus('idle')
                setFriendRequestMessage('')
              }}
              onKeyDown={(e) => {
                if (e.key === 'Enter' && friendRequestStatus !== 'sending') {
                  handleAddFriend()
                }
              }}
              placeholder="输入用户名"
              disabled={friendRequestStatus === 'sending' || friendRequestStatus === 'success'}
              className="modal-input mb-4 disabled:bg-slate-200"
              autoFocus
            />

            <div className="flex justify-end space-x-3">
              <button
                onClick={() => {
                  setShowFriendRequestModal(false)
                  setNewFriendUsername('')
                  setFriendRequestStatus('idle')
                  setFriendRequestMessage('')
                }}
                disabled={friendRequestStatus === 'sending'}
                className="modal-btn modal-btn-ghost disabled:opacity-50"
              >
                {friendRequestStatus === 'success' ? '关闭' : '取消'}
              </button>
              <button
                onClick={handleAddFriend}
                disabled={friendRequestStatus === 'sending' || friendRequestStatus === 'success'}
                className="modal-btn modal-btn-primary disabled:opacity-55 flex items-center min-w-[100px] justify-center"
              >
                {friendRequestStatus === 'sending' ? (
                  <>
                    <svg className="animate-spin -ml-1 mr-2 h-4 w-4 text-white" fill="none" viewBox="0 0 24 24">
                      <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                      <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                    </svg>
                    发送中...
                  </>
                ) : friendRequestStatus === 'success' ? (
                  '已发送'
                ) : (
                  '发送请求'
                )}
              </button>
            </div>
          </div>
        </div>
      )}

      {/* 登出确认对话框 */}
      {showLogoutConfirm && (
        <div className="modal-overlay">
          <div className="modal-card max-w-sm">
            <h3 className="modal-title mb-2">确认退出</h3>
            <p className="theme-muted mb-6">确定要退出登录吗？</p>
            <div className="flex justify-end space-x-3">
              <button
                onClick={() => setShowLogoutConfirm(false)}
                className="modal-btn modal-btn-ghost"
              >
                取消
              </button>
              <button
                onClick={handleLogout}
                className="modal-btn modal-btn-danger"
              >
                退出
              </button>
            </div>
          </div>
        </div>
      )}

      {/* 其他模态框 */}
      <ContextModal isOpen={showContextModal} onClose={() => setShowContextModal(false)} />
      <HistoryModal isOpen={showHistoryModal} onClose={() => setShowHistoryModal(false)} target={selectedFriend} />
      <SearchModal isOpen={showSearchModal} onClose={() => setShowSearchModal(false)} />
      <ExportModal isOpen={showExportModal} onClose={() => setShowExportModal(false)} target={selectedFriend} />
      <SessionExpiredModal isOpen={showSessionExpired} onClose={() => setShowSessionExpired(false)} onLogout={handleLogout} />
    </div>
  )
}
