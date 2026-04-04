import { create } from 'zustand'
import type { Friend, FriendRequest } from '../types/friend'
import type { UserStatus } from '../types'

interface FriendState {
  // 数据
  friends: Friend[]
  pendingRequests: FriendRequest[]
  onlineFriends: Friend[]

  // 状态
  isLoading: boolean
  error: string | null

  // 搜索
  searchQuery: string

  // 选中的好友（用于私聊）
  selectedFriend: string | null

  // Actions - 好友列表
  setFriends: (friends: Friend[]) => void
  addFriend: (friend: Friend) => void
  removeFriend: (username: string) => void
  updateFriendStatus: (username: string, status: UserStatus) => void
  getFriend: (username: string) => Friend | undefined

  // Actions - 好友请求
  setPendingRequests: (requests: FriendRequest[]) => void
  addPendingRequest: (request: FriendRequest) => void
  removePendingRequest: (senderUsername: string) => void
  acceptRequest: (senderUsername: string) => void
  rejectRequest: (senderUsername: string) => void

  // Actions - 搜索
  setSearchQuery: (query: string) => void
  getFilteredFriends: () => Friend[]

  // Actions - 选中好友
  setSelectedFriend: (username: string | null) => void

  // Actions - 通用
  setLoading: (loading: boolean) => void
  setError: (error: string | null) => void
  clearAll: () => void
}

export const useFriendStore = create<FriendState>((set, get) => ({
  // 初始状态
  friends: [],
  pendingRequests: [],
  onlineFriends: [],
  isLoading: false,
  error: null,
  searchQuery: '',
  selectedFriend: null,

  // 好友列表 Actions
  setFriends: (friends) => {
    const onlineFriends = friends.filter(f => f.status === 'online')
    set({ friends, onlineFriends })
  },

  addFriend: (friend) => set((state) => {
    const exists = state.friends.some(f => f.username === friend.username)
    if (exists) return state

    const newFriends = [...state.friends, friend]
    const onlineFriends = friend.status === 'online'
      ? [...state.onlineFriends, friend]
      : state.onlineFriends

    return { friends: newFriends, onlineFriends }
  }),

  removeFriend: (username) => set((state) => ({
    friends: state.friends.filter(f => f.username !== username),
    onlineFriends: state.onlineFriends.filter(f => f.username !== username)
  })),

  updateFriendStatus: (username, status) => set((state) => {
    const newFriends = state.friends.map(f =>
      f.username === username ? { ...f, status } : f
    )
    const onlineFriends = newFriends.filter(f => f.status === 'online')
    return { friends: newFriends, onlineFriends }
  }),

  getFriend: (username) => {
    return get().friends.find(f => f.username === username)
  },

  // 好友请求 Actions
  setPendingRequests: (requests) => set({ pendingRequests: requests }),

  addPendingRequest: (request) => set((state) => {
    const exists = state.pendingRequests.some(r => r.senderUsername === request.senderUsername)
    if (exists) return state
    return { pendingRequests: [...state.pendingRequests, request] }
  }),

  removePendingRequest: (senderUsername) => set((state) => ({
    pendingRequests: state.pendingRequests.filter(r => r.senderUsername !== senderUsername)
  })),

  acceptRequest: (senderUsername) => {
    const state = get()
    const request = state.pendingRequests.find(r => r.senderUsername === senderUsername)
    if (request) {
      // 将请求者添加为好友
      const newFriend: Friend = {
        id: request.senderId,
        username: senderUsername,
        status: 'online',
        createdAt: new Date().toISOString()
      }
      get().addFriend(newFriend)
      get().removePendingRequest(senderUsername)
    }
  },

  rejectRequest: (senderUsername) => {
    get().removePendingRequest(senderUsername)
  },

  // 搜索 Actions
  setSearchQuery: (query) => set({ searchQuery: query }),

  getFilteredFriends: () => {
    const state = get()
    if (!state.searchQuery.trim()) {
      return state.friends
    }
    return state.friends.filter(f => 
      f.username.toLowerCase().includes(state.searchQuery.toLowerCase())
    )
  },

  // 选中好友 Actions
  setSelectedFriend: (username) => set({ selectedFriend: username }),

  // 通用 Actions
  setLoading: (loading) => set({ isLoading: loading }),
  setError: (error) => set({ error }),
  clearAll: () => set({
    friends: [],
    pendingRequests: [],
    onlineFriends: [],
    isLoading: false,
    error: null,
    searchQuery: '',
    selectedFriend: null,
  }),
}))
