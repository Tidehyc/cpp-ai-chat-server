// 用户状态类型
export type UserStatus = 'online' | 'busy' | 'away' | 'offline'

// 用户信息
export interface User {
  id: number
  username: string
  status: UserStatus
  sessionToken?: string
}

// 消息类型
export type MessageType = 'system' | 'private' | 'room' | 'ai' | 'command'

// 消息接口
export interface Message {
  id?: number
  type: MessageType
  from_user: string
  to_user?: string
  room_name?: string
  content: string
  timestamp: string
  is_ai?: boolean
  is_history?: boolean
  is_own?: boolean
  is_error?: boolean
}

// 聊天房间（AI 房间）
export interface Room {
  id: number
  name: string
  ai_prompt: string
  description?: string
  icon?: string
  members?: string[]
}

// 群聊房间
export interface GroupRoom {
  name: string
  description: string
  icon: string
  members: string[]
  isJoined: boolean
}

// 聊天会话
export interface ChatSession {
  room_name: string
  last_message?: string
  last_time?: string
  unread_count: number
}

// WebSocket 消息
export interface WSMessage {
  type: string
  content?: string
  from?: string
  to?: string
  room?: string
  timestamp?: number
  [key: string]: any
}

// 连接状态
export type ConnectionStatus = 'connecting' | 'connected' | 'disconnected'

// 好友状态
export interface Friend {
  id: number
  username: string
  status: UserStatus
  createdAt: string
}

// 好友请求状态
export type FriendRequestStatus = 'pending' | 'accepted' | 'rejected'

// 好友请求
export interface FriendRequest {
  id: number
  senderId: number
  senderUsername: string
  status: FriendRequestStatus
  createdAt: string
}

// AI 预设
export interface AIPreset {
  id: string
  name: string
  icon: string
  description: string
}

// 主题类型
export type ThemeType = 'light' | 'dark' | 'system'
