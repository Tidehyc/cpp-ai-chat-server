import { useState, useEffect, useRef } from 'react'
import { useNavigate } from 'react-router-dom'
import { useUserStore } from '../stores/userStore'
import { wsService } from '../services/websocket'
import type { ConnectionStatus } from '../types'

export default function Register() {
  const navigate = useNavigate()
  const { login, isLoggedIn, username: storedUsername } = useUserStore()

  // 表单状态
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [confirmPassword, setConfirmPassword] = useState('')
  
  // 保存注册时的用户名和密码（用于自动登录）
  const registerUsernameRef = useRef('')
  const registerPasswordRef = useRef('')
  
  // UI 状态
  const [error, setError] = useState('')
  const [success, setSuccess] = useState('')
  const [loading, setLoading] = useState(false)
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('disconnected')

  // 检查是否已登录
  useEffect(() => {
    if (isLoggedIn && storedUsername) {
      navigate('/')
    }
  }, [isLoggedIn, storedUsername, navigate])

  // 连接 WebSocket
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

    // 监听连接状态
    const unsubscribe = wsService.onConnectionStatusChange((status) => {
      setConnectionStatus(status)
    })

    // 监听注册响应
    wsService.on('system', handleSystemMessage)

    return () => {
      unsubscribe()
      wsService.off('system', handleSystemMessage)
    }
  }, [])

  // 处理系统消息
  const handleSystemMessage = (message: any) => {
    const content = message.content || ''

    // 注册成功
    if (content.includes('注册成功') || content.includes('Registration successful')) {
      setSuccess('注册成功！正在自动登录...')
      setError('')
      // 延迟一点发送登录请求，确保服务器已保存用户数据
      setTimeout(() => {
        wsService.login(registerUsernameRef.current, registerPasswordRef.current)
      }, 500)
      return
    }

    // 登录成功（注册后的自动登录）
    if (content.includes('登录成功') || content.includes('Login successful')) {
      setLoading(false)
      // 从消息中提取用户名（匹配"欢迎 "后面到句号之前的内容）
      const match = content.match(/欢迎 ([^。]+)/) || content.match(/Welcome ([^。]+)/)
      const uname = match ? match[1].trim() : registerUsernameRef.current
      login(uname, Date.now())
      navigate('/')
      return
    }

    // 用户名已存在
    if (content.includes('已存在') || content.includes('already exists')) {
      setLoading(false)
      setError('用户名已存在，请选择其他用户名')
      return
    }

    // 密码太短
    if (content.includes('密码') && (content.includes('短') || content.includes('长度')) ||
        content.includes('password') && content.includes('short')) {
      setLoading(false)
      setError('密码长度至少为 6 位')
      return
    }

    // 注册/登录失败
    if (content.includes('失败') || content.includes('failed') || content.includes('error')) {
      // 跳过登录失败的处理，让登录重试
      if (!content.includes('登录') && !content.includes('Login')) {
        setLoading(false)
        setError('注册失败，请稍后重试')
      }
      return
    }
  }

  // 处理表单提交
  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    setError('')
    setSuccess('')

    // 验证
    if (!username.trim() || !password.trim()) {
      setError('请输入用户名和密码')
      return
    }

    if (password.length < 6) {
      setError('密码长度至少为 6 位')
      return
    }

    if (password !== confirmPassword) {
      setError('两次输入的密码不一致')
      return
    }

    if (connectionStatus !== 'connected') {
      setError('未连接到服务器，请稍后重试')
      return
    }

    setLoading(true)

    // 保存注册信息用于自动登录
    registerUsernameRef.current = username.trim()
    registerPasswordRef.current = password

    // 发送注册命令
    wsService.register(username.trim(), password)

    // 设置超时
    setTimeout(() => {
      if (loading && !success) {
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

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 dark:from-gray-900 dark:to-gray-800 flex items-center justify-center p-4">
      <div className="max-w-md w-full">
        {/* Logo 和标题 */}
        <div className="text-center mb-8">
          <div className="inline-flex items-center justify-center w-16 h-16 bg-blue-600 rounded-full mb-4">
            <span className="text-3xl">🤖</span>
          </div>
          <h1 className="text-3xl font-bold text-gray-900 dark:text-white mb-2">
            创建账号
          </h1>
          <p className="text-gray-600 dark:text-gray-300">
            注册账号，开始使用 AI Chat Server
          </p>
        </div>

        {/* 注册卡片 */}
        <div className="bg-white dark:bg-gray-800 rounded-2xl shadow-xl p-8">
          {/* 连接状态 */}
          <div className="flex items-center justify-center mb-6">
            <div className={`w-2 h-2 rounded-full ${getStatusColor()} mr-2 animate-pulse`}></div>
            <span className="text-sm text-gray-500 dark:text-gray-400">{connectionStatus === 'connected' ? '已连接' : connectionStatus === 'connecting' ? '连接中...' : '未连接'}</span>
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
                autoFocus
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
                placeholder="请输入密码（至少 6 位）"
                autoComplete="new-password"
              />
            </div>

            <div>
              <label htmlFor="confirmPassword" className="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
                确认密码
              </label>
              <input
                id="confirmPassword"
                type="password"
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                disabled={loading}
                className="w-full px-4 py-2 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-900 dark:text-white rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent disabled:bg-gray-100 dark:disabled:bg-gray-800"
                placeholder="请再次输入密码"
                autoComplete="new-password"
              />
            </div>

            {/* 错误提示 */}
            {error && (
              <div className="bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 text-red-700 dark:text-red-400 px-4 py-3 rounded-lg text-sm">
                {error}
              </div>
            )}

            {/* 成功提示 */}
            {success && (
              <div className="bg-green-50 dark:bg-green-900/20 border border-green-200 dark:border-green-800 text-green-700 dark:text-green-400 px-4 py-3 rounded-lg text-sm">
                {success}
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
                  注册中...
                </>
              ) : (
                '注册'
              )}
            </button>
          </form>

          {/* 返回登录 */}
          <div className="mt-6 text-center">
            <button
              onClick={() => navigate('/login')}
              className="text-sm text-gray-600 dark:text-gray-400 hover:text-gray-900 dark:hover:text-white font-medium"
            >
              ← 返回登录页面
            </button>
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
