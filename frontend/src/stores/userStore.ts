import { create } from 'zustand'
import { persist } from 'zustand/middleware'
import type { UserStatus } from '../types'

interface UserState {
  // 用户信息
  isLoggedIn: boolean
  username: string | null
  userId: number | null
  status: UserStatus

  // 登出确认对话框
  showLogoutConfirm: boolean

  // Actions
  login: (username: string, userId: number) => void
  logout: () => void
  setStatus: (status: UserStatus) => void
  setShowLogoutConfirm: (show: boolean) => void
  clear: () => void
}

export const useUserStore = create<UserState>()(
  persist(
    (set) => ({
      // 初始状态
      isLoggedIn: false,
      username: null,
      userId: null,
      status: 'online',
      showLogoutConfirm: false,

      // 登录
      login: (username, userId) =>
        set({
          isLoggedIn: true,
          username,
          userId,
          status: 'online'
        }),

      // 登出
      logout: () =>
        set({
          isLoggedIn: false,
          username: null,
          userId: null,
          status: 'online',
          showLogoutConfirm: false
        }),

      // 设置状态
      setStatus: (status) => set({ status }),

      // 设置登出确认对话框显示状态
      setShowLogoutConfirm: (show) => set({ showLogoutConfirm: show }),

      // 清除所有状态
      clear: () =>
        set({
          isLoggedIn: false,
          username: null,
          userId: null,
          status: 'online',
          showLogoutConfirm: false
        }),
    }),
    {
      name: 'user-storage',
      partialize: (state) => ({
        isLoggedIn: state.isLoggedIn,
        username: state.username,
        userId: state.userId,
        status: state.status,
      }),
    }
  )
)
