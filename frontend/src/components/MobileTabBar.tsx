import { useLocation, useNavigate } from 'react-router-dom'
import { MessageSquare, Settings, Sparkles, Users } from 'lucide-react'

const tabs = [
  { path: '/', label: '私聊', icon: MessageSquare },
  { path: '/contact', label: '群聊', icon: Users },
  { path: '/ai', label: 'AI', icon: Sparkles },
  { path: '/settings', label: '设置', icon: Settings },
]

export default function MobileTabBar() {
  const navigate = useNavigate()
  const location = useLocation()

  return (
    <>
      <div className="mobile-tabbar-spacer md:hidden" aria-hidden="true" />
      <nav className="mobile-tabbar md:hidden">
        <div className="mobile-tabbar-inner">
          {tabs.map((tab) => {
            const isActive = location.pathname === tab.path
            const Icon = tab.icon

            return (
              <button
                key={tab.path}
                type="button"
                onClick={() => navigate(tab.path)}
                className={`mobile-tabbar-item ${isActive ? 'is-active' : ''}`}
                aria-current={isActive ? 'page' : undefined}
              >
                <Icon size={18} />
                <span>{tab.label}</span>
              </button>
            )
          })}
        </div>
      </nav>
    </>
  )
}