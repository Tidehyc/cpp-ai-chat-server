import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { useUserStore } from '../stores/userStore'
import { useThemeStore } from '../stores/themeStore'
import { wsService } from '../services/websocket'

export default function Settings() {
  const navigate = useNavigate()
  const { username, status, setStatus, showLogoutConfirm, setShowLogoutConfirm, logout } = useUserStore()
  const { theme, setTheme } = useThemeStore()

  const [localStatus, setLocalStatus] = useState(status)

  // 用户状态选项
  const statusOptions = [
    { value: 'online', label: '在线', icon: '🟢' },
    { value: 'busy', label: '忙碌', icon: '🔴' },
    { value: 'away', label: '离开', icon: '🟡' },
    { value: 'offline', label: '离线', icon: '⚪' },
  ]

  // 主题选项
  const themeOptions = [
    { value: 'light', label: '浅色', icon: '☀️' },
    { value: 'dark', label: '深色', icon: '🌙' },
    { value: 'system', label: '系统', icon: '💻' },
  ]

  // 更新状态
  const handleUpdateStatus = (newStatus: typeof status) => {
    setLocalStatus(newStatus)
    setStatus(newStatus)
    wsService.setStatus(newStatus)
  }

  // 更新主题
  const handleUpdateTheme = (newTheme: typeof theme) => {
    setTheme(newTheme)
  }

  // 登出
  const handleLogout = () => {
    wsService.logout()
    logout()
    localStorage.removeItem('user-storage')
    navigate('/login')
  }

  return (
    <div className="min-h-screen bg-gray-100 dark:bg-gray-900">
      {/* 顶部栏 */}
      <div className="bg-white dark:bg-gray-800 border-b border-gray-200 dark:border-gray-700">
        <div className="max-w-4xl mx-auto px-4 py-4 flex items-center justify-between">
          <h1 className="text-2xl font-bold text-gray-900 dark:text-white">⚙️ 设置</h1>
          <button
            onClick={() => navigate('/')}
            className="px-4 py-2 text-blue-600 dark:text-blue-400 hover:bg-blue-50 dark:hover:bg-gray-700 rounded-lg transition-colors"
          >
            ← 返回聊天
          </button>
        </div>
      </div>

      {/* 设置内容 */}
      <div className="max-w-4xl mx-auto px-4 py-8">
        {/* 用户信息卡片 */}
        <div className="bg-white dark:bg-gray-800 rounded-xl shadow-sm p-6 mb-6">
          <h2 className="text-lg font-semibold text-gray-900 dark:text-white mb-4">👤 用户信息</h2>
          <div className="flex items-center space-x-4">
            <div className="w-16 h-16 bg-blue-600 rounded-full flex items-center justify-center text-white text-2xl font-bold">
              {username?.charAt(0).toUpperCase()}
            </div>
            <div>
              <p className="text-xl font-medium text-gray-900 dark:text-white">{username}</p>
              <p className="text-sm text-gray-500 dark:text-gray-400">
                当前状态：{statusOptions.find(s => s.value === status)?.label}
              </p>
            </div>
          </div>
        </div>

        {/* 状态设置 */}
        <div className="bg-white dark:bg-gray-800 rounded-xl shadow-sm p-6 mb-6">
          <h2 className="text-lg font-semibold text-gray-900 dark:text-white mb-4">📍 在线状态</h2>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
            {statusOptions.map((option) => (
              <button
                key={option.value}
                onClick={() => handleUpdateStatus(option.value as typeof status)}
                className={`p-4 rounded-lg border-2 transition-colors ${
                  localStatus === option.value
                    ? 'border-blue-500 bg-blue-50 dark:bg-blue-900/20'
                    : 'border-gray-200 dark:border-gray-700 hover:border-gray-300 dark:hover:border-gray-600'
                }`}
              >
                <div className="text-center">
                  <span className="text-3xl">{option.icon}</span>
                  <p className="text-sm font-medium mt-2 text-gray-900 dark:text-white">{option.label}</p>
                </div>
              </button>
            ))}
          </div>
        </div>

        {/* 主题设置 */}
        <div className="bg-white dark:bg-gray-800 rounded-xl shadow-sm p-6 mb-6">
          <h2 className="text-lg font-semibold text-gray-900 dark:text-white mb-4">🎨 主题设置</h2>
          <div className="grid grid-cols-3 gap-3">
            {themeOptions.map((option) => (
              <button
                key={option.value}
                onClick={() => handleUpdateTheme(option.value as typeof theme)}
                className={`p-4 rounded-lg border-2 transition-colors ${
                  theme === option.value
                    ? 'border-blue-500 bg-blue-50 dark:bg-blue-900/20'
                    : 'border-gray-200 dark:border-gray-700 hover:border-gray-300 dark:hover:border-gray-600'
                }`}
              >
                <div className="text-center">
                  <span className="text-3xl">{option.icon}</span>
                  <p className="text-sm font-medium mt-2 text-gray-900 dark:text-white">{option.label}</p>
                </div>
              </button>
            ))}
          </div>
        </div>

        {/* 账户操作 */}
        <div className="bg-white dark:bg-gray-800 rounded-xl shadow-sm p-6">
          <h2 className="text-lg font-semibold text-gray-900 dark:text-white mb-4">🔐 账户操作</h2>
          <div className="space-y-3">
            <button
              onClick={() => setShowLogoutConfirm(true)}
              className="w-full bg-red-50 dark:bg-red-900/20 hover:bg-red-100 dark:hover:bg-red-900/30 text-red-600 dark:text-red-400 font-medium py-3 px-4 rounded-lg transition-colors text-left"
            >
              🚪 退出登录
            </button>
          </div>
        </div>

        {/* 版本信息 */}
        <div className="mt-8 text-center text-sm text-gray-500 dark:text-gray-400">
          <p>AI Chat Server v1.0</p>
          <p className="mt-1">Powered by C++17 + Boost.Asio + React 18</p>
        </div>
      </div>

      {/* 登出确认对话框 */}
      {showLogoutConfirm && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <div className="bg-white dark:bg-gray-800 rounded-xl p-6 w-full max-w-sm">
            <h3 className="text-lg font-semibold mb-2 text-gray-900 dark:text-white">确认退出</h3>
            <p className="text-gray-600 dark:text-gray-300 mb-6">确定要退出登录吗？</p>
            <div className="flex justify-end space-x-3">
              <button
                onClick={() => setShowLogoutConfirm(false)}
                className="px-4 py-2 text-gray-600 dark:text-gray-300 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg transition-colors"
              >
                取消
              </button>
              <button
                onClick={handleLogout}
                className="px-4 py-2 bg-red-600 hover:bg-red-700 text-white rounded-lg transition-colors"
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
