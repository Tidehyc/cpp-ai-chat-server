import { create } from 'zustand'
import type { Room, GroupRoom } from '../types'

// 内置 AI 房间配置
export const DEFAULT_AI_ROOMS: Room[] = [
  { id: 1, name: '#general', ai_prompt: 'friendly', description: '友好助手', icon: 'general' },
  { id: 2, name: '#code', ai_prompt: 'coder', description: '编程助手', icon: 'code' },
  { id: 3, name: '#translate', ai_prompt: 'translator', description: '翻译', icon: 'translate' },
  { id: 4, name: '#learn', ai_prompt: 'teacher', description: '学习辅导', icon: 'learn' },
  { id: 5, name: '#write', ai_prompt: 'writer', description: '写作助手', icon: 'write' },
  { id: 6, name: '#debug', ai_prompt: 'debugger', description: '调试专家', icon: 'debug' },
]

// 内置群聊房间配置
export const DEFAULT_GROUP_ROOMS: GroupRoom[] = [
  { name: '#general', description: '综合交流群', icon: 'general', members: [], isJoined: false },
  { name: '#code', description: '编程技术讨论', icon: 'code', members: [], isJoined: false },
  { name: '#gaming', description: '游戏交流', icon: 'gaming', members: [], isJoined: false },
  { name: '#music', description: '音乐分享', icon: 'music', members: [], isJoined: false },
  { name: '#movie', description: '电影推荐', icon: 'movie', members: [], isJoined: false },
]

interface RoomState {
  // AI 房间
  aiRooms: Room[]
  
  // 群聊房间
  groupRooms: GroupRoom[]
  
  // 当前选中的房间
  currentRoom: string | null
  
  // 当前房间类型：'ai' | 'group' | 'private'
  currentRoomType: 'ai' | 'group' | 'private' | null

  // 在线用户列表（当前房间内）
  onlineUsers: Array<{ username: string; status: string }>

  // 是否正在创建房间
  isCreatingRoom: boolean

  // Actions
  setAiRooms: (rooms: Room[]) => void
  setGroupRooms: (rooms: GroupRoom[]) => void
  setCurrentRoom: (room: string | null, type?: 'ai' | 'group' | 'private') => void
  setOnlineUsers: (users: Array<{ username: string; status: string }>) => void
  joinRoom: (roomName: string) => void
  leaveRoom: (roomName: string) => void
  addRoom: (room: GroupRoom) => void
  setIsCreatingRoom: (creating: boolean) => void
  getRoom: (name: string) => GroupRoom | undefined
  clearAll: () => void
}

export const useRoomStore = create<RoomState>((set, get) => ({
  // 初始状态
  aiRooms: DEFAULT_AI_ROOMS,
  groupRooms: DEFAULT_GROUP_ROOMS,
  currentRoom: null,
  currentRoomType: null,
  onlineUsers: [],
  isCreatingRoom: false,

  // 设置 AI 房间
  setAiRooms: (rooms) => set({ aiRooms: rooms }),

  // 设置群聊房间
  setGroupRooms: (rooms) => set({ groupRooms: rooms }),

  // 设置当前房间
  setCurrentRoom: (room, type) => set({ 
    currentRoom: room, 
    currentRoomType: type || null 
  }),

  // 设置在线用户
  setOnlineUsers: (users) => set({ onlineUsers: users }),

  // 加入房间
  joinRoom: (roomName) => set((state) => ({
    groupRooms: state.groupRooms.map(r =>
      r.name === roomName ? { ...r, isJoined: true } : r
    ),
    currentRoom: roomName,
    currentRoomType: 'group'
  })),

  // 离开房间
  leaveRoom: (roomName) => set((state) => ({
    groupRooms: state.groupRooms.map(r =>
      r.name === roomName ? { ...r, isJoined: false, members: [] } : r
    ),
  })),

  // 添加房间
  addRoom: (room) => set((state) => ({
    groupRooms: [...state.groupRooms, room]
  })),

  // 设置创建房间状态
  setIsCreatingRoom: (creating) => set({ isCreatingRoom: creating }),

  // 获取房间
  getRoom: (name) => {
    return get().groupRooms.find(r => r.name === name)
  },

  // 清除所有状态
  clearAll: () => set({
    aiRooms: DEFAULT_AI_ROOMS,
    groupRooms: DEFAULT_GROUP_ROOMS,
    currentRoom: null,
    currentRoomType: null,
    onlineUsers: [],
    isCreatingRoom: false,
  }),
}))
