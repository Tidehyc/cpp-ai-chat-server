import type { ConnectionStatus, WSMessage } from '../types'

// 消息处理器类型
type MessageHandler = (message: any) => void

// WebSocket 服务配置
interface WebSocketConfig {
  url: string
  maxReconnectAttempts: number
  reconnectDelay: number
  heartbeatInterval: number
}

class WebSocketService {
  private ws: WebSocket | null = null
  private config: WebSocketConfig
  private messageHandlers: Map<string, Set<MessageHandler>> = new Map()
  
  // 重连状态
  private reconnectAttempts = 0
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null
  private heartbeatTimer: ReturnType<typeof setInterval> | null = null
  
  // 连接状态
  private connectionStatus: ConnectionStatus = 'disconnected'
  private statusListeners: Set<(status: ConnectionStatus) => void> = new Set()
  
  // 消息缓冲
  private messageBuffer: string[] = []
  private connectPromise: Promise<void> | null = null

  constructor(url: string = 'ws://127.0.0.1:8080') {
    this.config = {
      url,
      maxReconnectAttempts: 10,
      reconnectDelay: 3000,
      heartbeatInterval: 30000,
    }
  }

  // ==================== 连接管理 ====================

  /**
   * 连接到 WebSocket 服务器
   */
  async connect(): Promise<void> {
    // 如果已经连接，直接返回
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      console.log('[WS] Already connected')
      return Promise.resolve()
    }

    // 如果正在连接，返回现有的 Promise
    if (this.connectPromise) {
      return this.connectPromise
    }

    this.updateConnectionStatus('connecting')

    this.connectPromise = new Promise((resolve, reject) => {
      try {
        console.log('[WS] Connecting to', this.config.url)
        this.ws = new WebSocket(this.config.url)

        this.ws.onopen = () => {
          console.log('[WS] ✅ Connected')
          this.reconnectAttempts = 0
          this.updateConnectionStatus('connected')
          this.startHeartbeat()
          this.flushMessageBuffer()
          this.connectPromise = null
          resolve()
        }

        this.ws.onerror = (error) => {
          console.error('[WS] ❌ Error:', error)
          this.updateConnectionStatus('disconnected')
          this.connectPromise = null
          reject(new Error('WebSocket 连接失败，请确保后端服务正在运行'))
        }

        this.ws.onmessage = (event) => {
          const text = event.data
          console.log('[WS] 📩 Raw:', text)
          const parsed = this.parseServerMessage(text)
          if (parsed) {
            this.handleMessage(parsed)
          }
        }

        this.ws.onclose = (event) => {
          console.log('[WS] 🔌 Closed:', event.code, event.reason)
          this.updateConnectionStatus('disconnected')
          this.stopHeartbeat()
          this.connectPromise = null
          this.attemptReconnect()
        }
      } catch (error) {
        this.connectPromise = null
        this.updateConnectionStatus('disconnected')
        reject(error)
      }
    })

    // 设置连接超时
    const timeout = new Promise<void>((_, reject) => {
      setTimeout(() => {
        reject(new Error('连接超时 (5 秒)'))
      }, 5000)
    })

    return Promise.race([this.connectPromise, timeout])
  }

  /**
   * 断开连接
   */
  disconnect(): void {
    this.stopReconnect()
    this.stopHeartbeat()
    if (this.ws) {
      this.ws.close()
      this.ws = null
    }
    this.updateConnectionStatus('disconnected')
  }

  /**
   * 手动重连
   */
  reconnect(): void {
    this.disconnect()
    this.reconnectAttempts = 0
    setTimeout(() => this.connect(), 500)
  }

  // ==================== 消息发送 ====================

  /**
   * 发送命令到服务器
   * 后端使用纯文本命令格式
   */
  sendCommand(command: string): void {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(command)
      console.log('[WS] 📤 Command:', command)
    } else {
      console.warn('[WS] Not connected, command queued')
      this.messageBuffer.push(command)
    }
  }

  /**
   * 发送聊天消息
   */
  sendText(content: string): void {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(content)
      console.log('[WS] 📤 Message:', content)
    } else {
      console.warn('[WS] Not connected, message queued')
      this.messageBuffer.push(content)
    }
  }

  /**
   * 通用发送接口
   */
  send(message: WSMessage | string): void {
    if (typeof message === 'string') {
      this.sendText(message)
    } else if (message.type === 'command') {
      this.sendCommand(message.content || '')
    } else {
      this.sendText(message.content || message.text || '')
    }
  }

  // ==================== 业务方法 ====================

  /**
   * 用户登录
   */
  login(username: string, password: string): void {
    this.sendCommand(`/login ${username} ${password}`)
  }

  /**
   * 用户注册
   */
  register(username: string, password: string): void {
    this.sendCommand(`/register ${username} ${password}`)
  }

  /**
   * 登出
   */
  logout(): void {
    this.sendCommand('/quit')
  }

  /**
   * 发送私聊消息
   */
  sendPrivateMessage(toUser: string, content: string): void {
    this.sendCommand(`/pm ${toUser} ${content}`)
  }

  /**
   * 加入群聊房间
   */
  joinRoom(roomName: string): void {
    this.sendCommand(`/join ${roomName}`)
  }

  /**
   * 离开群聊房间
   */
  leaveRoom(): void {
    this.sendCommand('/leave')
  }

  /**
   * 创建群聊房间
   */
  createRoom(roomName: string): void {
    this.sendCommand(`/create room ${roomName}`)
  }

  /**
   * 发送好友请求
   */
  sendFriendRequest(username: string): void {
    this.sendCommand(`/friend request ${username}`)
  }

  /**
   * 同意好友请求
   */
  acceptFriendRequest(username: string): void {
    this.sendCommand(`/friend accept ${username}`)
  }

  /**
   * 拒绝好友请求
   */
  rejectFriendRequest(username: string): void {
    this.sendCommand(`/friend reject ${username}`)
  }

  /**
   * 删除好友
   */
  deleteFriend(username: string): void {
    this.sendCommand(`/friend del ${username}`)
  }

  /**
   * 获取好友列表
   */
  getFriendList(): void {
    this.sendCommand('/friend list')
  }

  /**
   * 获取好友请求列表
   */
  getFriendRequests(): void {
    this.sendCommand('/friend requests')
  }

  /**
   * 获取房间列表
   */
  getRoomList(): void {
    this.sendCommand('/list')
  }

  /**
   * 获取在线用户
   */
  getOnlineUsers(): void {
    this.sendCommand('/users')
  }

  /**
   * 设置用户状态
   */
  setStatus(status: string): void {
    this.sendCommand(`/status ${status}`)
  }

  /**
   * 发送 AI 消息
   */
  sendAIMessage(content: string): void {
    this.sendCommand(`/msg ai ${content}`)
  }

  /**
   * 使用 AI 预设
   */
  usePreset(presetName: string): void {
    this.sendCommand(`/preset use ${presetName}`)
  }

  /**
   * 清空对话历史
   */
  clearHistory(): void {
    this.sendCommand('/clear')
  }

  /**
   * 搜索消息
   */
  searchMessages(keyword: string): void {
    this.sendCommand(`/search ${keyword}`)
  }

  /**
   * 导出聊天记录
   */
  exportHistory(target: string, format?: 'json' | 'txt'): void {
    const cmd = format === 'json' 
      ? `/export ${target} format=json`
      : `/export ${target}`
    this.sendCommand(cmd)
  }

  // ==================== 事件订阅 ====================

  /**
   * 订阅消息类型
   */
  on(type: string, handler: MessageHandler): void {
    if (!this.messageHandlers.has(type)) {
      this.messageHandlers.set(type, new Set())
    }
    this.messageHandlers.get(type)!.add(handler)
  }

  /**
   * 取消订阅
   */
  off(type: string, handler: MessageHandler): void {
    const handlers = this.messageHandlers.get(type)
    if (handlers) {
      handlers.delete(handler)
    }
  }

  /**
   * 订阅连接状态变化
   */
  onConnectionStatusChange(listener: (status: ConnectionStatus) => void): () => void {
    this.statusListeners.add(listener)
    return () => this.statusListeners.delete(listener)
  }

  /**
   * 获取当前连接状态
   */
  getConnectionStatus(): ConnectionStatus {
    return this.connectionStatus
  }

  /**
   * 检查是否已连接
   */
  isConnected(): boolean {
    return this.ws !== null && this.ws.readyState === WebSocket.OPEN
  }

  // ==================== 私有方法 ====================

  private updateConnectionStatus(status: ConnectionStatus) {
    this.connectionStatus = status
    this.statusListeners.forEach(listener => listener(status))
  }

  private handleMessage(message: any): void {
    console.log('[WS] 📨 Parsed:', message)
    
    // 触发特定类型的处理器
    const handlers = this.messageHandlers.get(message.type)
    if (handlers) {
      handlers.forEach(handler => handler(message))
    }

    // 触发通用处理器
    const allHandlers = this.messageHandlers.get('*')
    if (allHandlers) {
      allHandlers.forEach(handler => handler(message))
    }
  }

  private attemptReconnect(): void {
    if (this.reconnectAttempts < this.config.maxReconnectAttempts) {
      this.reconnectAttempts++
      console.log(`[WS] 🔄 Reconnecting... (${this.reconnectAttempts}/${this.config.maxReconnectAttempts})`)
      
      this.stopReconnect()
      this.reconnectTimer = setTimeout(() => {
        this.connectPromise = null
        this.connect()
      }, this.config.reconnectDelay)
    } else {
      console.error('[WS] ❌ Max reconnect attempts reached')
    }
  }

  private stopReconnect() {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer)
      this.reconnectTimer = null
    }
  }

  private flushMessageBuffer(): void {
    while (this.messageBuffer.length > 0 && this.ws && this.ws.readyState === WebSocket.OPEN) {
      const msg = this.messageBuffer.shift()
      if (msg && this.ws) {
        this.ws.send(msg)
      }
    }
  }

  private startHeartbeat() {
    this.stopHeartbeat()
    this.heartbeatTimer = setInterval(() => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        // 发送心跳（可以使用 ping 命令或空消息）
        this.ws.send('')
        console.log('[WS] 💓 Heartbeat')
      }
    }, this.config.heartbeatInterval)
  }

  private stopHeartbeat() {
    if (this.heartbeatTimer) {
      clearInterval(this.heartbeatTimer)
      this.heartbeatTimer = null
    }
  }

  /**
   * 解析后端服务器的消息
   */
  private parseServerMessage(text: string): any | null {
    if (!text) return null

    try {
      // 尝试解析 JSON
      const data = JSON.parse(text)

      // 检查是否有 content 或 friends 字段
      if (data.content !== undefined || data.friends !== undefined || data.type !== undefined) {
        return this.parseJSONMessage(data)
      }
    } catch {
      // 不是 JSON，尝试解析纯文本格式
      return this.parseTextMessage(text)
    }

    return null
  }

  /**
   * 解析 JSON 消息
   */
  private parseJSONMessage(data: any): any {
    const type = this.determineMessageType(data)
    
    return {
      type,
      from_user: data.from || data.from_user || 'system',
      to_user: data.to || data.to_user,
      room: data.room || data.room_name,
      content: data.content,
      timestamp: data.timestamp 
        ? new Date(data.timestamp * 1000).toLocaleString('zh-CN')
        : new Date().toLocaleString('zh-CN'),
      is_ai: type === 'ai_response',
      raw: data,
    }
  }

  /**
   * 确定消息类型
   */
  private determineMessageType(data: any): string {
    // 如果服务器已经明确指定了 type，直接使用
    if (data.type === 'private_message') {
      return 'private_message'
    }
    if (data.type === 'sent_message') {
      return 'sent_message'
    }
    if (data.type === 'ai_response') {
      return 'ai_response'
    }
    if (data.type === 'system') {
      return 'system'
    }
    if (data.type === 'friend_online') {
      return 'friend_online'
    }
    if (data.type === 'friend_offline') {
      return 'friend_offline'
    }
    if (data.type === 'friend_list') {
      return 'friend_list'
    }

    const content = data.content || ''
    const from = data.from || data.from_user || ''
    const to = data.to || data.to_user || ''

    // 好友列表
    if (data.friends) {
      return 'friend_list'
    }

    // 好友上下线通知
    if (data.username && (data.status === 'online' || data.status === 'offline')) {
      if (data.status === 'online') return 'friend_online'
      if (data.status === 'offline') return 'friend_offline'
    }

    // AI 响应
    if (content.includes('[AI]') || from === 'ai' || from === 'AI') {
      return 'ai_response'
    }

    // 私聊消息
    if (to && to !== 'all' && to !== 'broadcast') {
      return 'private_message'
    }

    // 系统消息
    if (from === 'system' || from === 'Server' || content.startsWith('📢')) {
      return 'system'
    }

    // 房间消息
    if (data.room || data.room_name) {
      return 'room_message'
    }

    // 好友请求通知
    if (content.includes('好友请求') || content.includes('friend request')) {
      return 'friend_request'
    }

    return 'system'
  }

  /**
   * 解析纯文本消息
   */
  private parseTextMessage(text: string): any {
    // 匹配 [用户名]: 消息 格式
    const userMsgMatch = text.match(/^\[([^\]]+)\]:\s*(.*)$/)
    if (userMsgMatch) {
      const [, from, content] = userMsgMatch
      const isOwn = from === '你'
      return {
        type: isOwn ? 'sent_message' : 'room_message',
        from_user: isOwn ? 'me' : from,
        content,
        timestamp: new Date().toLocaleString('zh-CN'),
        is_ai: false,
      }
    }

    // 系统消息
    return {
      type: 'system',
      content: text,
      timestamp: new Date().toLocaleString('zh-CN'),
    }
  }
}

// 导出单例
export const wsService = new WebSocketService()
