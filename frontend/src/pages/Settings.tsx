import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { useUserStore } from '../stores/userStore'
import { wsService } from '../services/websocket'
import { Circle, Home, LogOut, Palette, Settings as SettingsIcon, Sun, User } from 'lucide-react'
import MobileTabBar from '../components/MobileTabBar'

export default function Settings() {
  const navigate = useNavigate()
  const { username, status, setStatus, showLogoutConfirm, setShowLogoutConfirm, logout } = useUserStore()

  const [localStatus, setLocalStatus] = useState(status)

  // 用户状态选项
  const statusOptions = [
    { value: 'online', label: '在线', icon: <Circle className="mx-auto text-emerald-500" size={18} fill="currentColor" /> },
    { value: 'busy', label: '忙碌', icon: <Circle className="mx-auto text-rose-500" size={18} fill="currentColor" /> },
    { value: 'away', label: '离开', icon: <Circle className="mx-auto text-amber-500" size={18} fill="currentColor" /> },
    { value: 'offline', label: '离线', icon: <Circle className="mx-auto theme-muted" size={18} fill="currentColor" /> },
  ]

  // 更新状态
  const handleUpdateStatus = (newStatus: typeof status) => {
    setLocalStatus(newStatus)
    setStatus(newStatus)
    wsService.setStatus(newStatus)
  }

  // 登出
  const handleLogout = () => {
    wsService.logout()
    logout()
    localStorage.removeItem('user-storage')
    navigate('/login')
  }

  return (
    <div className="auth-shell page-enter min-h-screen">
      {/* 顶部栏 */}
      <div className="chat-topnav border-b theme-divider">
        <div className="max-w-4xl mx-auto px-4 py-4 flex items-center justify-between">
          <h1 className="chat-title text-2xl font-bold icon-label"><SettingsIcon size={20} />设置中心</h1>
          <button
            onClick={() => navigate('/')}
            className="action-chip"
          >
            <span className="icon-label"><Home size={14} />返回主界面</span>
          </button>
        </div>
      </div>

      {/* 设置内容 */}
      <div className="max-w-4xl mx-auto px-4 py-8">
        {/* 用户信息卡片 */}
        <div className="panel-card p-6 mb-6">
          <h2 className="text-lg font-semibold text-inherit mb-4 icon-label"><User size={17} />用户信息</h2>
          <div className="flex items-center space-x-4">
            <div className="w-16 h-16 rounded-2xl bg-gradient-to-br from-cyan-600 to-blue-700 flex items-center justify-center text-white text-2xl font-bold shadow-lg shadow-cyan-500/25">
              {username?.charAt(0).toUpperCase()}
            </div>
            <div>
              <p className="text-xl font-medium text-inherit">{username}</p>
              <p className="text-sm theme-muted">
                当前状态：{statusOptions.find(s => s.value === status)?.label}
              </p>
            </div>
          </div>
        </div>

        {/* 状态设置 */}
        <div className="panel-card p-6 mb-6">
          <h2 className="text-lg font-semibold text-inherit mb-4 icon-label"><Circle size={14} fill="currentColor" className="text-emerald-500" />在线状态</h2>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
            {statusOptions.map((option) => (
              <button
                key={option.value}
                onClick={() => handleUpdateStatus(option.value as typeof status)}
                className={`p-4 rounded-xl border transition-all ${
                  localStatus === option.value
                    ? 'border-cyan-500 bg-cyan-50/75 shadow-sm shadow-cyan-200/60'
                    : 'border-theme-divider hover:border-cyan-200 bg-white/78'
                }`}
              >
                <div className="text-center">
                  <span className="text-3xl icon-only">{option.icon}</span>
                  <p className="text-sm font-medium mt-2 text-inherit">{option.label}</p>
                </div>
              </button>
            ))}
          </div>
        </div>

        {/* 主题设置 */}
        <div className="panel-card p-6 mb-6">
          <h2 className="text-lg font-semibold text-inherit mb-4 icon-label"><Palette size={17} />主题设置</h2>
          <div className="theme-surface rounded-xl p-4 flex items-center justify-between">
            <div>
              <p className="text-sm font-medium text-inherit">当前主题</p>
              <p className="text-xs theme-muted mt-1">仅提供浅色主题，界面风格保持统一</p>
            </div>
            <div className="inline-flex items-center gap-2 rounded-full px-3 py-1 theme-nav-item is-active">
              <Sun size={16} />
              <span className="text-sm font-medium">浅色</span>
            </div>
          </div>
        </div>

        {/* 账户操作 */}
        <div className="panel-card p-6">
          <h2 className="text-lg font-semibold text-inherit mb-4 icon-label"><LogOut size={17} />账户操作</h2>
          <div className="space-y-3">
            <button
              onClick={() => setShowLogoutConfirm(true)}
              className="w-full rounded-xl border border-rose-200/70 bg-rose-50/85 hover:bg-rose-100 text-rose-600 font-medium py-3 px-4 transition-colors text-left"
            >
              <span className="icon-label"><LogOut size={15} />退出登录</span>
            </button>
          </div>
        </div>

        {/* 版本信息 */}
        <div className="mt-8 text-center text-sm theme-muted">
          <p>AI Chat Server v1.0</p>
          <p className="mt-1">Powered by C++17 + Boost.Asio + React 18</p>
        </div>
      </div>

      <MobileTabBar />

      {/* 登出确认对话框 */}
      {showLogoutConfirm && (
        <div className="modal-overlay">
          <div className="modal-card max-w-sm">
            <h3 className="modal-title mb-2">确认退出</h3>
            <p className="theme-muted mb-6">确定要退出登录吗？</p>
            <div className="flex justify-end space-x-3">
              <button
                onClick={() => setShowLogoutConfirm(false)}
                className="modal-btn modal-btn-ghost"
              >
                取消
              </button>
              <button
                onClick={handleLogout}
                className="modal-btn modal-btn-danger"
              >
                退出
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}
