import { create } from 'zustand'
import { persist } from 'zustand/middleware'
import type { ThemeType } from '../types'

interface ThemeState {
  theme: ThemeType
  setTheme: (theme: ThemeType) => void
  // 获取实际生效的主题（处理 system 情况）
  getEffectiveTheme: () => 'light' | 'dark'
}

export const useThemeStore = create<ThemeState>()(
  persist(
    (set, get) => ({
      theme: 'light',
      setTheme: (theme) => set({ theme }),
      getEffectiveTheme: () => {
        const { theme } = get()
        if (theme === 'system') {
          return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
        }
        return theme
      },
    }),
    {
      name: 'theme-storage',
    }
  )
)
