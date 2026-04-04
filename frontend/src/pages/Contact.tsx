import { useState, useEffect, useRef } from 'react'
import { useNavigate, Link } from 'react-router-dom'
import { useUserStore } from '../stores/userStore'
import { useMessageStore } from '../stores/messageStore'
import { useRoomStore, DEFAULT_GROUP_ROOMS } from '../stores/roomStore'
import { wsService } from '../services/websocket'
import type { ConnectionStatus } from '../types'

// 模态框组件
import HistoryModal from '../components/HistoryModal'
import ExportModal from '../components/ExportModal'
import SessionExpiredModal from '../components/SessionExpiredModal'

export default function Contact() {
  const navigate = useNavigate()
  
  // Store 状态
  const { username, status, showLogoutConfirm, setShowLogoutConfirm, logout } = useUserStore()
  const { addMessage, getMessages, setCurrentChatTarget } = useMessageStore()
  const { 
    groupRooms, currentRoom,
    setCurrentRoom, joinRoom, leaveRoom, addRoom
  } = useRoomStore()

  // 本地状态
  const [inputMessage, setInputMessage] = useState('')
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('disconnected')
  const [showCreateRoomModal, setShowCreateRoomModal] = useState(false)
  const [newRoomName, setNewRoomName] = useState('')
  const [newRoomDescription, setNewRoomDescription] = useState('')
  
  // 模态框状态
  const [showHistoryModal, setShowHistoryModal] = useState(false)
  const [showExportModal, setShowExportModal] = useState(false)
  const [showSessionExpired, setShowSessionExpired] = useState(false)

  // 消息列表 ref
  const messagesEndRef = useRef<HTMLDivElement>(null)
  const inputRef = useRef<HTMLTextAreaElement>(null)

  // 当前房间消息
  const currentMessages = currentRoom ? getMessages(currentRoom) : []

  // 滚动到底部
  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' })
  }

  useEffect(() => {
    scrollToBottom()
  }, [currentMessages])

  // 使用 ref 存储处理函数，避免依赖项变化
  const handleRoomMessageRef = useRef<(message: any) => void>(() => {})
  const handleSystemMessageRef = useRef<(message: any) => void>(() => {})

  // 处理房间消息
  handleRoomMessageRef.current = (message: any) => {
    console.log('[Contact] Received room message:', message)
    const roomName = message.room || message.room_name
    const fromUser = message.from || message.from_user
    const content = message.content

    if (!roomName) return

    console.log('[Contact] Adding message to room:', roomName, 'Current room:', currentRoom)

    // 添加消息到 store
    addMessage(roomName, {
      type: 'room',
      from_user: fromUser,
      room_name: roomName,
      content,
      timestamp: message.timestamp ? new Date(message.timestamp * 1000).toLocaleString('zh-CN') : new Date().toLocaleString('zh-CN'),
      is_own: fromUser === username,
    })
  }

  // 处理系统消息
  handleSystemMessageRef.current = (message: any) => {
    const content = message.content || ''
    console.log('[Contact] System message:', content)

    // AI 思考中提示
    if (content.includes('AI 思考中')) {
      const roomName = useRoomStore.getState().currentRoom
      if (roomName) {
        addMessage(roomName, {
          type: 'ai',
          from_user: 'AI',
          room_name: roomName,
          content: '🤖 AI 思考中...',
          timestamp: new Date().toLocaleString('zh-CN'),
          is_ai: true,
          is_own: false,
        })
      }
      return
    }

    // 加入房间成功
    if (content.includes('加入房间') || content.includes('joined room')) {
      const match = content.match(/加入房间 (#\S+)/) || content.match(/joined room (#\S+)/)
      if (match) {
        joinRoom(match[1])
      }
    }

    // 离开房间
    if (content.includes('离开房间') || content.includes('left room')) {
      if (currentRoom) {
        leaveRoom(currentRoom)
      }
    }

    // 创建房间成功
    if (content.includes('创建房间') || content.includes('created room')) {
      const match = content.match(/创建房间 (#\S+)/) || content.match(/created room (#\S+)/)
      if (match) {
        const roomName = match[1]
        addRoom({
          name: roomName,
          description: '新创建的房间',
          icon: '💬',
          members: [],
          isJoined: true,
        })
        setCurrentRoom(roomName, 'group')
      }
    }

    // 用户加入房间通知
    if (content.includes('加入了') && !content.includes('你')) {
      const match = content.match(/(\S+) 加入了 (#\S+)/)
      if (match) {
        // 可以更新房间成员列表
      }
    }

    // 用户离开房间通知
    if (content.includes('离开了') && !content.includes('你')) {
      const match = content.match(/(\S+) 离开了 (#\S+)/)
      if (match) {
        // 可以更新房间成员列表
      }
    }

    // 登出提示
    if (content.includes('已退出') || content.includes('logged out')) {
      handleLogout()
    }
  }

  // 处理房间列表更新
  const handleRoomListUpdate = (message: any) => {
    console.log('[Contact] handleRoomListUpdate called:', message)
    // 从 raw 字段中获取 rooms
    const rooms = message.raw?.rooms || message.rooms || []
    console.log('[Contact] Rooms from message:', rooms)
    if (rooms.length > 0) {
      // 使用 store 的 getState 获取最新的 currentRoom，避免闭包陷阱
      const currentRoom = useRoomStore.getState().currentRoom
      const updatedRooms = rooms.map((r: any) => ({
        name: r.name,
        description: r.description || '房间',
        icon: r.icon || '💬',
        members: [],
        isJoined: currentRoom === r.name,
        isCustom: r.isCustom || false,
      }))
      console.log('[Contact] Updating rooms to:', updatedRooms)
      useRoomStore.getState().setGroupRooms(updatedRooms)
    }
  }

  // 初始化 WebSocket 连接
  useEffect(() => {
    const initWebSocket = async () => {
      try {
        setConnectionStatus('connecting')
        await wsService.connect()
        setConnectionStatus('connected')
      } catch (err) {
        setConnectionStatus('disconnected')
      }
    }

    initWebSocket()

    // 监听连接状态
    const unsubscribeStatus = wsService.onConnectionStatusChange((status) => {
      setConnectionStatus(status)
    })

    // 监听房间消息 - 使用稳定引用
    const roomMessageHandler = (msg: any) => handleRoomMessageRef.current(msg)
    const systemMessageHandler = (msg: any) => handleSystemMessageRef.current(msg)
    const roomListUpdateHandler = (msg: any) => handleRoomListUpdate(msg)
    const aiResponseHandler = (msg: any) => {
      console.log('[Contact] Received AI response in room:', msg)
      const content = msg.content || ''
      // 优先使用消息中的 room 字段，如果没有则使用当前房间
      const roomName = msg.room || useRoomStore.getState().currentRoom
      if (roomName) {
        addMessage(roomName, {
          type: 'ai',
          from_user: 'AI',
          room_name: roomName,
          content,
          timestamp: msg.timestamp ? new Date(msg.timestamp * 1000).toLocaleString('zh-CN') : new Date().toLocaleString('zh-CN'),
          is_ai: true,
          is_own: false,
        })
      }
    }

    wsService.on('room_message', roomMessageHandler)
    wsService.on('system', systemMessageHandler)
    wsService.on('room_list_update', roomListUpdateHandler)
    wsService.on('ai_response', aiResponseHandler)

    // 进入页面时主动获取房间列表
    wsService.sendCommand('/rooms_json')

    // 监听会话过期
    const handleSessionExpired = () => {
      setShowSessionExpired(true)
    }
    window.addEventListener('session-expired', handleSessionExpired)

    return () => {
      unsubscribeStatus()
      wsService.off('room_message', roomMessageHandler)
      wsService.off('system', systemMessageHandler)
      wsService.off('room_list_update', roomListUpdateHandler)
      wsService.off('ai_response', aiResponseHandler)
      window.removeEventListener('session-expired', handleSessionExpired)
    }
  }, [])

  // 发送消息
  const handleSendMessage = () => {
    console.log('[Contact] Sending message. Current room:', currentRoom, 'Input:', inputMessage)
    if (!inputMessage.trim() || !currentRoom) return

    const content = inputMessage.trim()

    // 添加自己的消息到 UI
    addMessage(currentRoom, {
      type: 'room',
      from_user: username || 'me',
      room_name: currentRoom,
      content,
      timestamp: new Date().toLocaleString('zh-CN'),
      is_own: true,
    })

    // 发送消息到房间
    wsService.sendText(content)

    setInputMessage('')
  }

  // 处理键盘事件
  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault()
      handleSendMessage()
    }
  }

  // 加入房间
  const handleJoinRoom = (roomName: string) => {
    if (currentRoom && currentRoom !== roomName) {
      // 先离开当前房间
      wsService.leaveRoom()
      leaveRoom(currentRoom)
    }
    
    wsService.joinRoom(roomName)
    setCurrentRoom(roomName, 'group')
    setCurrentChatTarget(roomName, 'room')
  }

  // 离开房间
  const handleLeaveRoom = () => {
    if (currentRoom) {
      wsService.leaveRoom()
      leaveRoom(currentRoom)
      setCurrentRoom(null, 'group')
      setCurrentChatTarget(null)
    }
  }

  // 创建房间
  const handleCreateRoom = () => {
    if (!newRoomName.trim()) return
    
    const roomName = newRoomName.trim().startsWith('#') 
      ? newRoomName.trim() 
      : `#${newRoomName.trim()}`
    
    wsService.createRoom(roomName)
    setNewRoomName('')
    setNewRoomDescription('')
    setShowCreateRoomModal(false)
  }

  // 登出
  const handleLogout = () => {
    wsService.logout()
    logout()
    localStorage.removeItem('user-storage')
    navigate('/login')
  }

  // 连接状态样式
  const getStatusColor = () => {
    switch (connectionStatus) {
      case 'connected': return 'text-green-600'
      case 'connecting': return 'text-yellow-600'
      case 'disconnected': return 'text-red-600'
    }
  }

  const getStatusText = () => {
    switch (connectionStatus) {
      case 'connected': return '已连接'
      case 'connecting': return '连接中...'
      case 'disconnected': return '未连接'
    }
  }

  // 用户状态图标
  const getStatusIcon = (userStatus: string) => {
    switch (userStatus) {
      case 'online': return '🟢'
      case 'busy': return '🔴'
      case 'away': return '🟡'
      case 'offline': return '⚪'
    }
  }

  return (
    <div className="h-screen flex bg-gray-100 dark:bg-gray-900">
      {/* 左侧边栏 - 群聊房间列表 */}
      <div className="w-72 bg-white dark:bg-gray-800 border-r border-gray-200 dark:border-gray-700 flex flex-col">
        {/* 标题栏 */}
        <div className="h-14 border-b border-gray-200 dark:border-gray-700 flex items-center justify-between px-4">
          <h1 className="text-lg font-semibold text-gray-900 dark:text-white">👥 群聊房间</h1>
          <button
            onClick={() => setShowCreateRoomModal(true)}
            className="p-2 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg transition-colors"
            title="创建房间"
          >
            <span className="text-xl">➕</span>
          </button>
        </div>

        {/* 房间列表 */}
        <div className="flex-1 overflow-y-auto">
          {groupRooms.length === 0 ? (
            <div className="p-4 text-center text-gray-500 dark:text-gray-400 text-sm">
              暂无房间，点击 ➕ 创建
            </div>
          ) : (
            groupRooms.map((room) => {
              const isCurrent = currentRoom === room.name

              return (
                <button
                  key={room.name}
                  onClick={() => handleJoinRoom(room.name)}
                  className={`w-full flex items-center px-4 py-3 hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors border-b border-gray-100 dark:border-gray-700 ${
                    isCurrent ? 'bg-blue-50 dark:bg-blue-900/20 border-r-4 border-blue-500' : ''
                  }`}
                >
                  <span className="text-2xl mr-3">{room.icon}</span>
                  <div className="flex-1 text-left">
                    <p className="text-sm font-medium text-gray-900 dark:text-white">{room.name}</p>
                    <p className="text-xs text-gray-500 dark:text-gray-400">{room.description}</p>
                  </div>
                  {isCurrent && (
                    <span className="text-xs text-blue-600 dark:text-blue-400 font-medium">当前</span>
                  )}
                </button>
              )
            })
          )}
        </div>

        {/* 底部用户信息 */}
        <div className="h-14 border-t border-gray-200 dark:border-gray-700 flex items-center justify-between px-4 bg-gray-50 dark:bg-gray-800">
          <div className="flex items-center">
            <span className="mr-2">{getStatusIcon(status)}</span>
            <span className="text-sm font-medium text-gray-700 dark:text-gray-300">{username}</span>
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
            <span className="text-lg font-semibold text-gray-900 dark:text-white">👥 群聊</span>
            <nav className="flex items-center space-x-2">
              <Link
                to="/"
                className="px-3 py-1.5 text-sm text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded transition-colors"
                title="私聊"
              >
                💬 私聊
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
            {currentRoom ? (
              <>
                <span className="text-xl mr-2">
                  {groupRooms.find(r => r.name === currentRoom)?.icon || '💬'}
                </span>
                <span className="font-semibold text-gray-900 dark:text-white">{currentRoom}</span>
                <span className={`ml-3 text-sm ${getStatusColor()}`}>
                  {getStatusText()}
                </span>
              </>
            ) : (
              <span className="text-gray-500 dark:text-gray-400">选择一个群聊房间</span>
            )}
          </div>

          {currentRoom && (
            <div className="flex items-center space-x-2">
              <button
                onClick={handleLeaveRoom}
                className="px-3 py-1.5 text-sm text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded transition-colors"
              >
                离开房间
              </button>
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
          {currentRoom ? (
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
                    className={`flex ${msg.is_own ? 'justify-end' : 'justify-start'}`}
                  >
                    <div
                      className={`max-w-2xl px-4 py-2 rounded-2xl ${
                        msg.is_own
                          ? 'bg-blue-600 text-white rounded-br-none'
                          : 'bg-white dark:bg-gray-800 text-gray-900 dark:text-white rounded-bl-none shadow'
                      }`}
                    >
                      {!msg.is_own && (
                        <p className="text-xs text-gray-500 dark:text-gray-400 mb-1">
                          <span className="font-medium">{msg.from_user}</span>
                        </p>
                      )}
                      <p className="text-sm">{msg.content}</p>
                      <p className={`text-xs mt-1 ${msg.is_own ? 'text-blue-100' : 'text-gray-400'}`}>
                        {msg.timestamp}
                      </p>
                    </div>
                  </div>
                ))}
                <div ref={messagesEndRef} />
              </div>
            )
          ) : (
            <div className="h-full flex items-center justify-center text-gray-400">
              <div className="text-center">
                <span className="text-6xl mb-4 block">👈</span>
                <p className="text-lg">从左侧选择一个房间</p>
                <p className="text-sm mt-2">加入群聊讨论</p>
              </div>
            </div>
          )}
        </div>

        {/* 输入框 */}
        {currentRoom && (
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

      {/* 创建房间模态框 */}
      {showCreateRoomModal && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <div className="bg-white dark:bg-gray-800 rounded-xl p-6 w-full max-w-md">
            <h3 className="text-lg font-semibold mb-4 text-gray-900 dark:text-white">创建群聊房间</h3>
            <div className="space-y-4">
              <div>
                <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
                  房间名称
                </label>
                <input
                  type="text"
                  value={newRoomName}
                  onChange={(e) => setNewRoomName(e.target.value)}
                  placeholder="例如：#tech-talk"
                  className="w-full px-4 py-2 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-900 dark:text-white rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                  autoFocus
                />
              </div>
              <div>
                <label className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
                  房间描述（可选）
                </label>
                <input
                  type="text"
                  value={newRoomDescription}
                  onChange={(e) => setNewRoomDescription(e.target.value)}
                  placeholder="房间简介..."
                  className="w-full px-4 py-2 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-900 dark:text-white rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                />
              </div>
            </div>
            <div className="flex justify-end space-x-3 mt-6">
              <button
                onClick={() => { setShowCreateRoomModal(false); setNewRoomName(''); setNewRoomDescription('') }}
                className="px-4 py-2 text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg transition-colors"
              >
                取消
              </button>
              <button
                onClick={handleCreateRoom}
                className="px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg transition-colors"
              >
                创建
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
      <HistoryModal isOpen={showHistoryModal} onClose={() => setShowHistoryModal(false)} target={currentRoom} />
      <ExportModal isOpen={showExportModal} onClose={() => setShowExportModal(false)} target={currentRoom} />
      <SessionExpiredModal isOpen={showSessionExpired} onClose={() => setShowSessionExpired(false)} onLogout={handleLogout} />
    </div>
  )
}
