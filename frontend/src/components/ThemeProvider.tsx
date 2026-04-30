import { useEffect } from 'react'
import { useThemeStore } from '../stores/themeStore'

export default function ThemeProvider({ children }: { children: React.ReactNode }) {
  useThemeStore((state) => state.theme)

  useEffect(() => {
    const root = document.documentElement

    root.classList.remove('dark')
    root.classList.add('light')
  }, [])

  return <>{children}</>
}
