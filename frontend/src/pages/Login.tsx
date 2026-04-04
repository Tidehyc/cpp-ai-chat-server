import { useState, useEffect } from 'react'
import { useNavigate } from 'react-router-dom'
import { useUserStore } from '../stores/userStore'
import { useMessageStore } from '../stores/messageStore'
import { wsService } from '../services/websocket'
import type { ConnectionStatus } from '../types'

export default function Login() {
  const navigate = useNavigate()
  const { login, isLoggedIn, username: storedUsername } = useUserStore()
  const { addMessage } = useMessageStore()

  // 表单状态
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')

  // UI 状态
  const [error, setError] = useState('')
  const [loading, setLoading] = useState(false)
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('disconnected')

  // 检查是否已登录（从 localStorage 恢复）
  useEffect(() => {
    if (isLoggedIn && storedUsername) {
      navigate('/')
    }
  }, [isLoggedIn, storedUsername, navigate])

  // 连接 WebSocket 并监听状态
  useEffect(() => {
    const connectWS = async () => {
      try {
        setConnectionStatus('connecting')
        await wsService.connect()
        setConnectionStatus('connected')
      } catch (err) {
        setConnectionStatus('disconnected')
        setError('无法连接到服务器，请确保后端服务正在运行')
      }
    }

    connectWS()

    // 监听连接状态变化
    const unsubscribe = wsService.onConnectionStatusChange((status) => {
      setConnectionStatus(status)
    })

    // 监听登录响应
    wsService.on('system', handleSystemMessage)
    
    // 监听离线私聊消息（在 Login 页面就处理，防止消息丢失）
    wsService.on('private_message', handlePrivateMessage)

    return () => {
      unsubscribe()
      wsService.off('system', handleSystemMessage)
      wsService.off('private_message', handlePrivateMessage)
    }
  }, [])

  // 处理离线私聊消息
  const handlePrivateMessage = (message: any) => {
    const fromUser = message.from_user || message.from
    const content = message.content
    // 将离线消息保存到 store，等跳转到 Chat 页面后就能显示
    addMessage(fromUser, {
      type: 'private',
      from_user: fromUser,
      to_user: undefined,
      content,
      timestamp: message.timestamp ? new Date(message.timestamp * 1000).toLocaleString('zh-CN') : new Date().toLocaleString('zh-CN'),
      is_own: false,
    }, false)
  }

  // 处理系统消息（登录/注册响应）
  const handleSystemMessage = (message: any) => {
    const content = message.content || ''
    console.log('[Login] System message:', content)

    // 登录成功
    if (content.includes('登录成功') || content.includes('Login successful')) {
      setLoading(false)
      // 从消息中提取用户名
      const match = content.match(/欢迎 ([^。]+)/)
      const uname = match ? match[1].trim() : username

      // 更新 store，persist 中间件会自动保存到 localStorage
      useUserStore.getState().login(uname, Date.now())
      useUserStore.getState().setStatus('online')

      // 延迟跳转，确保离线消息已处理完
      setTimeout(() => {
        navigate('/')
      }, 300)
      return
    }

    // 用户不存在
    if (content.includes('用户不存在')) {
      setLoading(false)
      setError('用户不存在')
      return
    }

    // 密码错误
    if (content.includes('密码错误')) {
      setLoading(false)
      setError('密码错误')
      return
    }

    // 登录失败（通用）
    if (content.includes('登录失败') || content.includes('Login failed')) {
      setLoading(false)
      setError('用户名或密码错误')
      return
    }

    // 密码太短
    if (content.includes('密码') && content.includes('短') ||
        content.includes('password') && content.includes('short')) {
      setLoading(false)
      setError('密码长度至少为 6 位')
      return
    }
  }

  // 处理表单提交
  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    setError('')

    // 验证
    if (!username.trim() || !password.trim()) {
      setError('请输入用户名和密码')
      return
    }

    if (password.length < 6) {
      setError('密码长度至少为 6 位')
      return
    }

    if (connectionStatus !== 'connected') {
      setError('未连接到服务器，请稍后重试')
      return
    }

    setLoading(true)

    // 发送登录命令
    wsService.login(username.trim(), password)

    // 设置超时
    setTimeout(() => {
      if (loading) {
        setLoading(false)
        setError('请求超时，请检查网络连接')
      }
    }, 10000)
  }

  // 连接状态指示器
  const getStatusColor = () => {
    switch (connectionStatus) {
      case 'connected': return 'bg-green-500'
      case 'connecting': return 'bg-yellow-500'
      case 'disconnected': return 'bg-red-500'
    }
  }

  const getStatusText = () => {
    switch (connectionStatus) {
      case 'connected': return '已连接'
      case 'connecting': return '连接中...'
      case 'disconnected': return '未连接'
    }
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 dark:from-gray-900 dark:to-gray-800 flex items-center justify-center p-4">
      <div className="max-w-md w-full">
        {/* Logo 和标题 */}
        <div className="text-center mb-8">
          <div className="inline-flex items-center justify-center w-16 h-16 bg-blue-600 rounded-full mb-4">
            <span className="text-3xl">🤖</span>
          </div>
          <h1 className="text-3xl font-bold text-gray-900 dark:text-white mb-2">
            AI Chat Server
          </h1>
          <p className="text-gray-600 dark:text-gray-300">
            高性能 C++ AI 聊天服务器
          </p>
        </div>

        {/* 登录卡片 */}
        <div className="bg-white dark:bg-gray-800 rounded-2xl shadow-xl p-8">
          {/* 连接状态 */}
          <div className="flex items-center justify-center mb-6">
            <div className={`w-2 h-2 rounded-full ${getStatusColor()} mr-2 animate-pulse`}></div>
            <span className="text-sm text-gray-500 dark:text-gray-400">{getStatusText()}</span>
          </div>

          {/* 表单 */}
          <form onSubmit={handleSubmit} className="space-y-4">
            <div>
              <label htmlFor="username" className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
                用户名
              </label>
              <input
                id="username"
                type="text"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                disabled={loading}
                className="w-full px-4 py-2 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-900 dark:text-white rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent disabled:bg-gray-100 dark:disabled:bg-gray-800"
                placeholder="请输入用户名"
                autoComplete="username"
              />
            </div>

            <div>
              <label htmlFor="password" className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
                密码
              </label>
              <input
                id="password"
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                disabled={loading}
                className="w-full px-4 py-2 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-900 dark:text-white rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent disabled:bg-gray-100 dark:disabled:bg-gray-800"
                placeholder="请输入密码"
                autoComplete="current-password"
              />
            </div>

            {/* 错误提示 */}
            {error && (
              <div className="bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 text-red-700 dark:text-red-400 px-4 py-3 rounded-lg text-sm">
                {error}
              </div>
            )}

            {/* 提交按钮 */}
            <button
              type="submit"
              disabled={loading || connectionStatus !== 'connected'}
              className="w-full bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white font-medium py-2 px-4 rounded-lg transition-colors flex items-center justify-center"
            >
              {loading ? (
                <>
                  <svg className="animate-spin -ml-1 mr-2 h-4 w-4 text-white" fill="none" viewBox="0 0 24 24">
                    <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                    <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                  </svg>
                  登录中...
                </>
              ) : (
                '登录'
              )}
            </button>
          </form>

          {/* 提示信息 */}
          <div className="mt-6 text-center text-sm text-gray-500 dark:text-gray-400">
            <p>
              还没有账号？{' '}
              <button
                type="button"
                onClick={() => navigate('/register')}
                className="text-blue-600 dark:text-blue-400 hover:text-blue-700 dark:hover:text-blue-300 font-medium"
              >
                立即注册
              </button>
            </p>
          </div>
        </div>

        {/* 底部信息 */}
        <div className="mt-6 text-center text-xs text-gray-500 dark:text-gray-400">
          <p>支持好友私聊 · 群聊 · AI 对话</p>
          <p className="mt-1">Powered by C++17 + Boost.Asio + React</p>
        </div>
      </div>
    </div>
  )
}
