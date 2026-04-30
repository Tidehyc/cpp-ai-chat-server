import { NavLink } from 'react-router-dom'
import { Bot, Circle, MessageSquare, Settings, Users } from 'lucide-react'
import { useUserStore } from '../stores/userStore'

interface SidebarProps {
  currentPage?: 'chat' | 'contact' | 'ai' | 'settings'
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export default function Sidebar({ currentPage = 'chat' }: SidebarProps) {
  const { status } = useUserStore()

  const navItems = [
    { id: 'chat', label: '私聊', path: '/', icon: <MessageSquare size={18} /> },
    { id: 'contact', label: '群聊', path: '/contact', icon: <Users size={18} /> },
    { id: 'ai', label: 'AI 助手', path: '/ai', icon: <Bot size={18} /> },
    { id: 'settings', label: '设置', path: '/settings', icon: <Settings size={18} /> },
  ]

  const getStatusClass = (userStatus: string) => {
    switch (userStatus) {
      case 'online': return 'status-dot status-dot-online'
      case 'busy': return 'status-dot status-dot-busy'
      case 'away': return 'status-dot status-dot-away'
      case 'offline': return 'status-dot status-dot-offline'
      default: return 'status-dot status-dot-offline'
    }
  }

  return (
    <div className="w-16 theme-surface flex flex-col items-center py-4">
      <div className="mb-8">
        <Bot size={22} className="text-cyan-600" />
      </div>

      <nav className="flex-1 space-y-2">
        {navItems.map((item) => (
          <NavLink
            key={item.id}
            to={item.path}
            className={({ isActive }) =>
              `w-12 h-12 flex items-center justify-center rounded-xl transition-colors theme-nav-item ${
                isActive ? 'is-active' : ''
              }`
            }
            title={item.label}
          >
            <span className="icon-only">{item.icon}</span>
          </NavLink>
        ))}
      </nav>

      <div className="pt-4 border-t theme-divider">
        <div className="icon-only" title={status}>
          <Circle size={10} fill="currentColor" className={getStatusClass(status)} />
        </div>
      </div>
    </div>
  )
}
