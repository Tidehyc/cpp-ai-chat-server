import { useEffect } from 'react'

interface SessionExpiredModalProps {
  isOpen: boolean
  onClose: () => void
  onLogout: () => void
}

export default function SessionExpiredModal({ 
  isOpen, 
  onClose, 
  onLogout 
}: SessionExpiredModalProps) {

  // 自动登出
  useEffect(() => {
    if (isOpen) {
      const timer = setTimeout(() => {
        onLogout()
        onClose()
      }, 3000)

      return () => clearTimeout(timer)
    }
  }, [isOpen, onLogout, onClose])

  if (!isOpen) return null

  return (
    <div className="modal-overlay">
      <div className="theme-modal p-6 w-full max-w-sm text-center">
        {/* 图标 */}
        <div className="mb-4">
          <span className="text-5xl">⚠️</span>
        </div>

        {/* 标题 */}
        <h3 className="text-lg font-semibold text-inherit mb-2">
          会话已过期
        </h3>

        {/* 描述 */}
        <p className="theme-muted text-sm mb-6">
          您的登录会话已过期，可能是由于：<br />
          <span className="text-xs theme-muted">
            在其他地方登录、会话超时或服务器重启
          </span>
        </p>

        {/* 进度条 */}
        <div className="mb-4">
          <div className="h-1 bg-slate-200 rounded-full overflow-hidden">
            <div className="h-full bg-red-500 animate-pulse w-full"></div>
          </div>
        </div>

        {/* 提示信息 */}
        <p className="text-xs theme-muted mb-6">
          将在 3 秒后自动返回登录页面...
        </p>

        {/* 按钮 */}
        <div className="flex space-x-3">
          <button
            onClick={() => {
              onClose()
              onLogout()
            }}
            className="flex-1 bg-red-600 hover:bg-red-700 text-white font-medium py-2 px-4 rounded-lg transition-colors"
          >
            立即返回
          </button>
          <button
            onClick={onClose}
            className="flex-1 px-4 py-2 theme-muted hover:bg-white/60 rounded-lg transition-colors"
          >
            取消
          </button>
        </div>
      </div>
    </div>
  )
}
