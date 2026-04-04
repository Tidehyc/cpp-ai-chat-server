import { NavLink } from 'react-router-dom'
import { useUserStore } from '../stores/userStore'

interface SidebarProps {
  currentPage?: 'chat' | 'contact' | 'ai' | 'settings'
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export default function Sidebar({ currentPage = 'chat' }: SidebarProps) {
  const { status } = useUserStore()

  const navItems = [
    { id: 'chat', label: '💬 私聊', path: '/' },
    { id: 'contact', label: '👥 群聊', path: '/contact' },
    { id: 'ai', label: '🤖 AI 助手', path: '/ai' },
    { id: 'settings', label: '⚙️ 设置', path: '/settings' },
  ]

  const getStatusIcon = (userStatus: string) => {
    switch (userStatus) {
      case 'online': return '🟢'
      case 'busy': return '🔴'
      case 'away': return '🟡'
      case 'offline': return '⚪'
    }
  }

  return (
    <div className="w-16 bg-white border-r border-gray-200 flex flex-col items-center py-4">
      {/* Logo */}
      <div className="mb-8">
        <span className="text-2xl">🤖</span>
      </div>

      {/* 导航 */}
      <nav className="flex-1 space-y-2">
        {navItems.map((item) => (
          <NavLink
            key={item.id}
            to={item.path}
            className={({ isActive }) =>
              `w-12 h-12 flex items-center justify-center rounded-xl transition-colors ${
                isActive
                  ? 'bg-blue-100 text-blue-600'
                  : 'text-gray-600 hover:bg-gray-100'
              }`
            }
            title={item.label}
          >
            <span className="text-xl">{item.label.split(' ')[0]}</span>
          </NavLink>
        ))}
      </nav>

      {/* 用户状态 */}
      <div className="pt-4 border-t border-gray-200">
        <div className="text-2xl" title={status}>
          {getStatusIcon(status)}
        </div>
      </div>
    </div>
  )
}
