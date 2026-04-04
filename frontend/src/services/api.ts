import axios, { AxiosInstance, AxiosError } from 'axios'

const API_BASE_URL = (import.meta as any).env?.VITE_API_URL || 'http://localhost:8080'

// 创建 axios 实例
export const api: AxiosInstance = axios.create({
  baseURL: API_BASE_URL,
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
})

// 请求拦截器
api.interceptors.request.use(
  (config) => {
    // 从 localStorage 获取 token
    const userStorage = localStorage.getItem('user-storage')
    if (userStorage) {
      try {
        const { state } = JSON.parse(userStorage)
        if (state?.sessionToken) {
          config.headers.Authorization = `Bearer ${state.sessionToken}`
        }
      } catch (e) {
        console.error('Failed to parse user storage:', e)
      }
    }
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

// 响应拦截器
api.interceptors.response.use(
  (response) => response,
  (error: AxiosError) => {
    if (error.response?.status === 401) {
      // Token 过期，清除本地状态
      localStorage.removeItem('user-storage')
      window.dispatchEvent(new Event('session-expired'))
    }
    return Promise.reject(error)
  }
)

// ==================== 用户相关 API ====================

export interface LoginResponse {
  success: boolean
  username: string
  userId: number
  sessionToken: string
  message?: string
}

export interface RegisterResponse {
  success: boolean
  message?: string
}

/**
 * HTTP 登录（备用，主要使用 WebSocket 登录）
 */
export const login = async (username: string, password: string): Promise<LoginResponse> => {
  const response = await api.post<LoginResponse>('/api/login', { username, password })
  return response.data
}

/**
 * HTTP 注册（备用，主要使用 WebSocket 注册）
 */
export const register = async (username: string, password: string): Promise<RegisterResponse> => {
  const response = await api.post<RegisterResponse>('/api/register', { username, password })
  return response.data
}

/**
 * 获取用户信息
 */
export const getUserInfo = async (username: string) => {
  const response = await api.get(`/api/users/${username}`)
  return response.data
}

/**
 * 更新用户状态
 */
export const updateUserStatus = async (status: string) => {
  const response = await api.put('/api/users/status', { status })
  return response.data
}

// ==================== 好友相关 API ====================

export interface FriendInfo {
  id: number
  username: string
  status: string
  createdAt: string
}

export interface FriendRequestInfo {
  id: number
  senderId: number
  senderUsername: string
  status: string
  createdAt: string
}

/**
 * 获取好友列表
 */
export const getFriendList = async (): Promise<FriendInfo[]> => {
  const response = await api.get<FriendInfo[]>('/api/friends')
  return response.data
}

/**
 * 获取好友请求列表
 */
export const getFriendRequests = async (): Promise<FriendRequestInfo[]> => {
  const response = await api.get<FriendRequestInfo[]>('/api/friends/requests')
  return response.data
}

/**
 * 发送好友请求
 */
export const sendFriendRequest = async (username: string) => {
  const response = await api.post('/api/friends/request', { username })
  return response.data
}

/**
 * 同意好友请求
 */
export const acceptFriendRequest = async (username: string) => {
  const response = await api.post('/api/friends/accept', { username })
  return response.data
}

/**
 * 拒绝好友请求
 */
export const rejectFriendRequest = async (username: string) => {
  const response = await api.post('/api/friends/reject', { username })
  return response.data
}

/**
 * 删除好友
 */
export const deleteFriend = async (username: string) => {
  const response = await api.delete(`/api/friends/${username}`)
  return response.data
}

// ==================== 消息相关 API ====================

export interface MessageInfo {
  id: number
  fromUser: string
  toUser: string
  content: string
  timestamp: string
  isRead: boolean
}

/**
 * 获取私聊历史记录
 */
export const getPrivateMessages = async (username: string, limit: number = 50): Promise<MessageInfo[]> => {
  const response = await api.get<MessageInfo[]>(`/api/messages/private/${username}?limit=${limit}`)
  return response.data
}

/**
 * 获取群聊历史记录
 */
export const getRoomMessages = async (roomName: string, limit: number = 50): Promise<MessageInfo[]> => {
  const response = await api.get<MessageInfo[]>(`/api/messages/room/${roomName}?limit=${limit}`)
  return response.data
}

/**
 * 搜索消息
 */
export const searchMessages = async (keyword: string, options?: {
  from?: string
  room?: string
  limit?: number
}): Promise<MessageInfo[]> => {
  const params = new URLSearchParams({ keyword })
  if (options?.from) params.append('from', options.from)
  if (options?.room) params.append('room', options.room)
  if (options?.limit) params.append('limit', options.limit.toString())
  
  const response = await api.get<MessageInfo[]>(`/api/messages/search?${params}`)
  return response.data
}

/**
 * 导出聊天记录
 */
export const exportMessages = async (target: string, format: 'json' | 'txt' = 'txt'): Promise<Blob> => {
  const response = await api.get(`/api/messages/export/${target}?format=${format}`, {
    responseType: 'blob',
  })
  return response.data
}

// ==================== 房间相关 API ====================

export interface RoomInfo {
  name: string
  description: string
  members: string[]
  createdAt: string
}

/**
 * 获取房间列表
 */
export const getRoomList = async (): Promise<RoomInfo[]> => {
  const response = await api.get<RoomInfo[]>('/api/rooms')
  return response.data
}

/**
 * 创建房间
 */
export const createRoom = async (name: string, description?: string): Promise<RoomInfo> => {
  const response = await api.post<RoomInfo>('/api/rooms', { name, description })
  return response.data
}

/**
 * 加入房间
 */
export const joinRoom = async (roomName: string) => {
  const response = await api.post(`/api/rooms/${roomName}/join`)
  return response.data
}

/**
 * 离开房间
 */
export const leaveRoom = async (roomName: string) => {
  const response = await api.post(`/api/rooms/${roomName}/leave`)
  return response.data
}

/**
 * 获取房间成员
 */
export const getRoomMembers = async (roomName: string): Promise<string[]> => {
  const response = await api.get<{ members: string[] }>(`/api/rooms/${roomName}/members`)
  return response.data.members
}

// ==================== AI 相关 API ====================

export interface PresetInfo {
  name: string
  description: string
  prompt: string
}

/**
 * 获取 AI 预设列表
 */
export const getPresetList = async (): Promise<PresetInfo[]> => {
  const response = await api.get<PresetInfo[]>('/api/ai/presets')
  return response.data
}

/**
 * 使用 AI 预设
 */
export const usePreset = async (presetName: string) => {
  const response = await api.post('/api/ai/preset', { preset: presetName })
  return response.data
}

/**
 * 清空 AI 对话历史
 */
export const clearAIHistory = async () => {
  const response = await api.delete('/api/ai/history')
  return response.data
}
