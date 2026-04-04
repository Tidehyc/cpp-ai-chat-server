import type { UserStatus } from './index'

// 好友信息
export interface Friend {
  id: number
  username: string
  status: UserStatus
  createdAt: string
}

// 好友请求
export interface FriendRequest {
  id: number
  senderId: number
  senderUsername: string
  status: 'pending' | 'accepted' | 'rejected'
  createdAt: string
}

// 私聊消息
export interface PrivateMessage {
  id: number
  fromUser: string
  toUser: string
  content: string
  timestamp: string
  isRead: boolean
}

// 好友请求通知
export interface FriendRequestNotification {
  type: 'friend_request'
  senderUsername: string
  message: string
}
