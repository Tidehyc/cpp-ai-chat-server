import { useState, useEffect, useRef, useCallback } from 'react'
import { useNavigate, Link } from 'react-router-dom'
import { useUserStore } from '../stores/userStore'
import { useFriendStore } from '../stores/friendStore'
import { useMessageStore } from '../stores/messageStore'
import { useRoomStore } from '../stores/roomStore'
import { wsService } from '../services/websocket'
import type { ConnectionStatus } from '../types'
import type { FriendRequest } from '../types/friend'

// 模态框组件
import ContextModal from '../components/ContextModal'
import HistoryModal from '../components/HistoryModal'
import SearchModal from '../components/SearchModal'
import ExportModal from '../components/ExportModal'
import SessionExpiredModal from '../components/SessionExpiredModal'

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
  
  // 模态框状态
  const [showContextModal, setShowContextModal] = useState(false)
  const [showHistoryModal, setShowHistoryModal] = useState(false)
  const [showSearchModal, setShowSearchModal] = useState(false)
  const [showExportModal, setShowExportModal] = useState(false)
  const [showSessionExpired, setShowSessionExpired] = useState(false)

  // 消息列表 ref
  const messagesEndRef = useRef<HTMLDivElement>(null)
  const inputRef = useRef<HTMLTextAreaElement>(null)
  const selectedFriendRef = useRef<string | null>(null)
  const lastSentTargetRef = useRef<string | null>(null) // 记录最后发送的目标好友

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

  // 滚动到底部
  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' })
  }

  useEffect(() => {
    scrollToBottom()
  }, [currentMessages])

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

    addMessage(fromUser, {
      type: 'private',
      from_user: fromUser,
      to_user: username || undefined,
      content,
      timestamp: message.timestamp || new Date().toLocaleString('zh-CN'),
      is_own: false,
    }, false)
  }, [addMessage, username])

  // 处理发送的消息
  const handleSentMessage = useCallback((_message: any) => {
    // 自己的消息已经在 UI 上显示，无需额外处理
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

    // 好友请求通知 - 匹配 "📬 {username} 向你发送了好友请求"
    if (content.includes('向你发送了好友请求')) {
      const match = content.match(/📬 (\S+) 向你发送了好友请求/)
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
        content: '❌ 发送失败：' + content,
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

        // 连接成功后，检查是否需要自动登录
        if (username) {
          // 强制重置状态为 online
          useUserStore.getState().setStatus('online')
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
  const getStatusIcon = (userStatus: string) => {
    switch (userStatus) {
      case 'online': return '🟢'
      case 'busy': return '🔴'
      case 'away': return '🟡'
      case 'offline': return '⚪'
      default: return '⚪'
    }
  }

  return (
    <div className="h-screen flex bg-gray-100 dark:bg-gray-900">
      {/* 左侧边栏 - 好友列表 */}
      <div className="w-72 bg-white dark:bg-gray-800 border-r border-gray-200 dark:border-gray-700 flex flex-col">
        {/* 标题栏 */}
        <div className="h-14 border-b border-gray-200 dark:border-gray-700 flex items-center justify-between px-4">
          <h1 className="text-lg font-semibold text-gray-900 dark:text-white">💬 好友聊天</h1>
          <button
            onClick={() => {
              setShowFriendRequestModal(true)
              setFriendRequestStatus('idle')
              setFriendRequestMessage('')
              setNewFriendUsername('')
            }}
            className="p-2 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg transition-colors"
            title="添加好友"
          >
            <span className="text-xl">➕</span>
          </button>
        </div>

        {/* 待处理好友请求 */}
        {pendingRequests.length > 0 && (
          <div className="border-b border-gray-200 dark:border-gray-700 bg-yellow-50 dark:bg-yellow-900/20 p-3">
            <div className="flex items-center text-yellow-800 dark:text-yellow-400 font-medium mb-2">
              <span>📬</span>
              <span className="ml-2">待处理好友请求 ({pendingRequests.length})</span>
            </div>
            {pendingRequests.map((request) => (
              <div key={request.id} className="bg-white dark:bg-gray-800 rounded-lg p-2 mb-2 border border-yellow-200 dark:border-yellow-800">
                <p className="text-sm text-gray-700 dark:text-gray-300 mb-2">
                  <span className="font-medium">{request.senderUsername}</span> 请求添加你为好友
                </p>
                <div className="flex space-x-2">
                  <button
                    onClick={() => handleAcceptRequest(request.senderUsername)}
                    className="flex-1 bg-green-500 hover:bg-green-600 text-white text-sm py-1 px-3 rounded transition-colors"
                  >
                    ✓ 同意
                  </button>
                  <button
                    onClick={() => handleRejectRequest(request.senderUsername)}
                    className="flex-1 bg-red-500 hover:bg-red-600 text-white text-sm py-1 px-3 rounded transition-colors"
                  >
                    ✕ 拒绝
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}

        {/* 搜索框 */}
        <div className="p-3 border-b border-gray-200 dark:border-gray-700">
          <input
            type="text"
            placeholder="🔍 搜索好友..."
            value={useFriendStore((state) => state.searchQuery)}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-900 dark:text-white rounded-lg text-sm focus:ring-2 focus:ring-blue-500 focus:border-transparent"
          />
        </div>

        {/* 好友列表 */}
        <div className="flex-1 overflow-y-auto">
          {filteredFriends.length === 0 ? (
            <div className="p-4 text-center text-gray-500 dark:text-gray-400 text-sm">
              {friends.length === 0 ? '暂无好友，点击 ➕ 添加' : '没有找到匹配的好友'}
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
                    className={`flex-1 flex items-center px-4 py-3 hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors ${
                      selectedFriend === friend.username ? 'bg-blue-50 dark:bg-blue-900/20 border-r-4 border-blue-500' : ''
                    }`}
                  >
                    <span className="mr-3">{getStatusIcon(friend.status)}</span>
                    <div className="flex-1 text-left">
                      <div className="flex items-center justify-between">
                        <p className={`text-sm font-medium ${unreadCount > 0 ? 'text-gray-900 dark:text-white font-bold' : 'text-gray-900 dark:text-white'}`}>
                          {friend.username}
                        </p>
                        {/* 未读消息提示 */}
                        {unreadCount > 0 && (
                          <span className="bg-red-500 text-white text-xs font-bold px-2 py-0.5 rounded-full min-w-[20px] text-center">
                            {unreadCount > 99 ? '99+' : unreadCount}
                          </span>
                        )}
                      </div>
                      <p className="text-xs text-gray-500 dark:text-gray-400">
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
                    className="hidden group-hover:flex mr-2 p-2 text-red-500 hover:bg-red-50 dark:hover:bg-red-900/20 rounded-lg transition-colors"
                    title="删除好友"
                  >
                    <span className="text-sm">🗑️</span>
                  </button>
                </div>
              )
            })
          )}
        </div>

        {/* 底部用户信息 */}
        <div className="h-14 border-t border-gray-200 dark:border-gray-700 flex items-center justify-between px-4 bg-gray-50 dark:bg-gray-800">
          <div className="flex items-center">
            <span className="mr-2">{getStatusIcon(status)}</span>
            <span className="text-sm font-medium text-gray-700 dark:text-gray-300">{username}</span>
            <span className="text-xs text-gray-500 dark:text-gray-400 ml-2">
              {status === 'online' ? '在线' :
               status === 'busy' ? '忙碌' :
               status === 'away' ? '离开' :
               status === 'offline' ? '离线' : '在线'}
            </span>
          </div>
          <button
            onClick={() => setShowLogoutConfirm(true)}
            className="text-sm text-red-600 dark:text-red-400 hover:text-red-700 dark:hover:text-red-300 font-medium"
          >
            退出
          </button>
        </div>
      </div>

      {/* 右侧聊天区域 */}
      <div className="flex-1 flex flex-col">
        {/* 顶部导航栏 */}
        <div className="h-14 bg-white dark:bg-gray-800 border-b border-gray-200 dark:border-gray-700 flex items-center justify-between px-6">
          <div className="flex items-center space-x-4">
            <span className="text-lg font-semibold text-gray-900 dark:text-white">💬 私聊</span>
            <nav className="flex items-center space-x-2">
              <Link
                to="/contact"
                className="px-3 py-1.5 text-sm text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded transition-colors"
                title="群聊"
              >
                👥 群聊
              </Link>
              <Link
                to="/ai"
                className="px-3 py-1.5 text-sm text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded transition-colors"
                title="AI 助手"
              >
                🤖 AI
              </Link>
              <Link
                to="/settings"
                className="px-3 py-1.5 text-sm text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded transition-colors"
                title="设置"
              >
                ⚙️ 设置
              </Link>
            </nav>
          </div>
          <div className="flex items-center space-x-2">
            <span className={`text-sm ${getStatusColor()}`}>
              {getStatusIcon(status)}
              {status === 'online' ? '在线' :
               status === 'busy' ? '忙碌' :
               status === 'away' ? '离开' : '离线'}
            </span>
          </div>
        </div>

        {/* 聊天顶部栏 */}
        <div className="h-14 bg-white dark:bg-gray-800 border-b border-gray-200 dark:border-gray-700 flex items-center justify-between px-6">
          <div className="flex items-center">
            {selectedFriend ? (
              <>
                <span className="text-xl mr-2">💬</span>
                <span className="font-semibold text-gray-900 dark:text-white">{selectedFriend}</span>
                <span className={`ml-3 text-sm ${getStatusColor()}`}>
                  {getStatusIcon(friends.find(f => f.username === selectedFriend)?.status || 'offline')}
                  {getStatusText()}
                </span>
              </>
            ) : (
              <span className="text-gray-500 dark:text-gray-400">选择一个好友开始聊天</span>
            )}
          </div>

          {selectedFriend && (
            <div className="flex items-center space-x-2">
              <button
                onClick={() => setShowHistoryModal(true)}
                className="px-3 py-1.5 text-sm text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded transition-colors"
                title="历史记录"
              >
                📜 历史
              </button>
              <button
                onClick={() => setShowExportModal(true)}
                className="px-3 py-1.5 text-sm text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded transition-colors"
                title="导出记录"
              >
                📥 导出
              </button>
            </div>
          )}
        </div>

        {/* 消息列表 */}
        <div className="flex-1 overflow-y-auto bg-gray-50 dark:bg-gray-900 p-6">
          {selectedFriend ? (
            currentMessages.length === 0 ? (
              <div className="h-full flex items-center justify-center text-gray-400">
                <div className="text-center">
                  <span className="text-4xl mb-4 block">💬</span>
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
                      // 错误消息：红色背景 + 感叹号
                      <div className="max-w-md px-4 py-3 rounded-xl bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 flex items-center space-x-3">
                        <span className="text-red-500 text-xl">❗</span>
                        <div className="flex-1">
                          <p className="text-sm text-red-700 dark:text-red-400">{msg.content}</p>
                          <p className="text-xs text-red-500 dark:text-red-500 mt-1">{msg.timestamp}</p>
                        </div>
                      </div>
                    ) : (
                      <div
                        className={`max-w-md px-4 py-2 rounded-2xl ${
                          msg.is_own
                            ? 'bg-blue-600 text-white rounded-br-none'
                            : 'bg-white dark:bg-gray-800 text-gray-900 dark:text-white rounded-bl-none shadow'
                        }`}
                      >
                        {!msg.is_own && (
                          <p className="text-xs text-gray-500 dark:text-gray-400 mb-1">{msg.from_user}</p>
                        )}
                        <p className="text-sm">{msg.content}</p>
                        <p className={`text-xs mt-1 ${msg.is_own ? 'text-blue-100' : 'text-gray-400'}`}>
                          {msg.timestamp}
                        </p>
                      </div>
                    )}
                  </div>
                ))}
                <div ref={messagesEndRef} />
              </div>
            )
          ) : (
            <div className="h-full flex items-center justify-center text-gray-400">
              <div className="text-center">
                <span className="text-6xl mb-4 block">👈</span>
                <p className="text-lg">从左侧选择一个好友</p>
                <p className="text-sm mt-2">开始私聊对话</p>
              </div>
            </div>
          )}
        </div>

        {/* 输入框 */}
        {selectedFriend && (
          <div className="bg-white dark:bg-gray-800 border-t border-gray-200 dark:border-gray-700 p-4">
            <div className="flex items-end space-x-3">
              <textarea
                ref={inputRef}
                value={inputMessage}
                onChange={(e) => setInputMessage(e.target.value)}
                onKeyDown={handleKeyDown}
                placeholder="输入消息... (Enter 发送，Shift+Enter 换行)"
                rows={1}
                className="flex-1 px-4 py-3 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-900 dark:text-white rounded-xl focus:ring-2 focus:ring-blue-500 focus:border-transparent resize-none"
                style={{ minHeight: '48px', maxHeight: '120px' }}
              />
              <button
                onClick={handleSendMessage}
                disabled={!inputMessage.trim() || connectionStatus !== 'connected'}
                className="px-6 py-3 bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white font-medium rounded-xl transition-colors"
              >
                发送
              </button>
            </div>
          </div>
        )}
      </div>

      {/* 添加好友模态框 */}
      {showFriendRequestModal && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50" onClick={(e) => {
          // 点击背景关闭弹窗
          if (e.target === e.currentTarget) {
            setShowFriendRequestModal(false)
            setNewFriendUsername('')
            setFriendRequestStatus('idle')
            setFriendRequestMessage('')
          }
        }}>
          <div className="bg-white dark:bg-gray-800 rounded-xl p-6 w-full max-w-md">
            <h3 className="text-lg font-semibold mb-4 text-gray-900 dark:text-white">添加好友</h3>

            {/* 状态提示 */}
            {friendRequestStatus === 'success' && (
              <div className="mb-4 bg-green-50 dark:bg-green-900/20 border border-green-200 dark:border-green-800 text-green-700 dark:text-green-400 px-4 py-3 rounded-lg text-sm flex items-center">
                <span className="mr-2">✅</span>
                {friendRequestMessage}
              </div>
            )}

            {friendRequestStatus === 'error' && (
              <div className="mb-4 bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 text-red-700 dark:text-red-400 px-4 py-3 rounded-lg text-sm flex items-center">
                <span className="mr-2">❌</span>
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
              className="w-full px-4 py-2 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-900 dark:text-white rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent mb-4 disabled:bg-gray-100 dark:disabled:bg-gray-800"
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
                className="px-4 py-2 text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg transition-colors disabled:opacity-50"
              >
                {friendRequestStatus === 'success' ? '关闭' : '取消'}
              </button>
              <button
                onClick={handleAddFriend}
                disabled={friendRequestStatus === 'sending' || friendRequestStatus === 'success'}
                className="px-4 py-2 bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white rounded-lg transition-colors flex items-center min-w-[100px] justify-center"
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
                  '✓ 已发送'
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
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <div className="bg-white dark:bg-gray-800 rounded-xl p-6 w-full max-w-sm">
            <h3 className="text-lg font-semibold mb-2 text-gray-900 dark:text-white">确认退出</h3>
            <p className="text-gray-600 dark:text-gray-300 mb-6">确定要退出登录吗？</p>
            <div className="flex justify-end space-x-3">
              <button
                onClick={() => setShowLogoutConfirm(false)}
                className="px-4 py-2 text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg transition-colors"
              >
                取消
              </button>
              <button
                onClick={handleLogout}
                className="px-4 py-2 bg-red-600 hover:bg-red-700 text-white rounded-lg transition-colors"
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
