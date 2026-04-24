import { create } from 'zustand'
import type { Message, MessageType } from '../types'

interface MessageState {
  // 消息存储：key 为房间名或用户名
  messages: Map<string, Message[]>

  // 未读消息计数：key 为用户名或房间名
  unreadCounts: Map<string, number>

  // 当前选中的聊天目标（用户名或房间名）
  currentChatTarget: string | null

  // 当前聊天类型：'private' | 'room' | 'ai'
  currentChatType: 'private' | 'room' | 'ai' | null

  // Actions
  addMessage: (target: string, message: Message, isOwn?: boolean) => void
  getMessages: (target: string) => Message[]
  clearMessages: (target: string) => void
  clearAllMessages: () => void
  setCurrentChatTarget: (target: string | null, type?: 'private' | 'room' | 'ai') => void
  getMessagesByType: (target: string, type: MessageType) => Message[]
  
  // 未读消息 Actions
  getUnreadCount: (target: string) => number
  incrementUnread: (target: string) => void
  clearUnread: (target: string) => void
  clearAllUnread: () => void
  markMessagesRead: (target: string) => void
}

export const useMessageStore = create<MessageState>((set, get) => ({
  // 初始状态
  messages: new Map(),
  unreadCounts: new Map(),
  currentChatTarget: null,
  currentChatType: null,

  // 添加消息
  addMessage: (target, message, isOwn = false) => set((state) => {
    const roomMessages = state.messages.get(target) || []
    const newMessages = [...roomMessages, message]
    const newMap = new Map(state.messages).set(target, newMessages)

    // 限制每个聊天最多保存 100 条消息
    if (newMessages.length > 100) {
      newMessages.shift()
    }

    // 如果不是自己发送的消息，且当前不在该聊天中，增加未读计数
    const isCurrentChat = state.currentChatTarget === target
    if (!isOwn && !isCurrentChat) {
      const currentUnread = state.unreadCounts.get(target) || 0
      const newUnreadMap = new Map(state.unreadCounts).set(target, currentUnread + 1)
      return { messages: newMap, unreadCounts: newUnreadMap }
    }

    return { messages: newMap }
  }),

  // 获取消息
  getMessages: (target) => {
    return get().messages.get(target) || []
  },

  // 清除指定聊天的消息
  clearMessages: (target) => set((state) => {
    const newMap = new Map(state.messages)
    newMap.set(target, [])
    return { messages: newMap }
  }),

  // 清除所有消息
  clearAllMessages: () => set({ messages: new Map() }),

  // 设置当前聊天目标
  setCurrentChatTarget: (target, type) => set((state) => {
    // 切换聊天时清除该聊天的未读计数
    const newUnreadMap = new Map(state.unreadCounts)
    if (target) {
      newUnreadMap.delete(target)
    }
    return {
      currentChatTarget: target,
      currentChatType: type || null,
      unreadCounts: newUnreadMap
    }
  }),

  // 按类型获取消息
  getMessagesByType: (target, type) => {
    const messages = get().messages.get(target) || []
    return messages.filter(m => m.type === type)
  },

  // 获取未读消息数
  getUnreadCount: (target) => {
    return get().unreadCounts.get(target) || 0
  },

  // 增加未读消息数
  incrementUnread: (target) => set((state) => {
    const currentUnread = state.unreadCounts.get(target) || 0
    const newUnreadMap = new Map(state.unreadCounts).set(target, currentUnread + 1)
    return { unreadCounts: newUnreadMap }
  }),

  // 清除未读消息数
  clearUnread: (target) => set((state) => {
    const newUnreadMap = new Map(state.unreadCounts)
    newUnreadMap.delete(target)
    return { unreadCounts: newUnreadMap }
  }),

  // 清除所有未读消息数
  clearAllUnread: () => set({ unreadCounts: new Map() }),

  // 标记指定用户的消息为已读
  markMessagesRead: (target: string) => set((state) => {
    const messages = state.messages.get(target) || []
    let hasChanges = false
    const newMessages = messages.map(msg => {
      if (msg.is_own && !msg.is_read) {
        hasChanges = true
        return { ...msg, is_read: true }
      }
      return msg
    })
    
    if (!hasChanges) return state
    
    const newMap = new Map(state.messages).set(target, newMessages)
    return { messages: newMap }
  }),
}))
