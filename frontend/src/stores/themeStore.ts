import { create } from 'zustand'
import { persist } from 'zustand/middleware'
import type { ThemeType } from '../types'

interface ThemeState {
  theme: ThemeType
  setTheme: (theme: ThemeType) => void
}

export const useThemeStore = create<ThemeState>()(
  persist(
    (set, get) => ({
      theme: 'light',
      setTheme: (theme) => set({ theme }),
    }),
    {
      name: 'theme-storage',
    }
  )
)
