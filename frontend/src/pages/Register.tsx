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
    <div className="auth-shell min-h-screen flex items-center justify-center p-4">
      <div className="auth-wrap w-full max-w-5xl grid grid-cols-1 lg:grid-cols-[1.1fr_0.9fr] gap-0 overflow-hidden rounded-3xl border border-white/40 bg-white/40 shadow-2xl">
        <section className="auth-hero hidden lg:flex flex-col justify-between p-10">
          <div>
            <div className="auth-brand mb-6 inline-flex items-center gap-3 rounded-2xl border border-white/60 bg-white/60 px-4 py-2 text-sm font-semibold text-slate-700">
              <span className="text-lg">✦</span>
              Flow Chat Engine
            </div>
            <h1 className="auth-hero-title mb-4 text-4xl font-bold leading-tight text-slate-900">
              创建你的工作空间
              <br />
              从实时对话开始
            </h1>
            <p className="text-slate-600 text-base leading-7">
              注册后即可使用好友系统、房间聊天与 AI 助手。
              所有会话都会通过后端规则化流程保障质量。
            </p>
          </div>

          <div className="auth-hero-tags flex flex-wrap gap-2 text-xs text-slate-600">
            <span className="auth-tag">实时消息</span>
            <span className="auth-tag">好友私聊</span>
            <span className="auth-tag">AI 协作</span>
            <span className="auth-tag">安全门禁</span>
          </div>
        </section>

        <section className="auth-card p-6 md:p-10">
          <div className="text-center mb-8">
            <div className="inline-flex items-center justify-center w-16 h-16 rounded-2xl bg-gradient-to-br from-cyan-600 to-blue-700 shadow-lg shadow-cyan-500/30 mb-4">
              <span className="text-3xl">🚀</span>
            </div>
            <h2 className="text-3xl font-bold text-slate-900 mb-2">创建账号</h2>
            <p className="text-slate-600">注册后将自动登录进入聊天</p>
          </div>

          <div className="flex items-center justify-center mb-6">
            <div className={`w-2 h-2 rounded-full ${getStatusColor()} mr-2 animate-pulse`}></div>
            <span className="text-sm text-slate-500">{connectionStatus === 'connected' ? '已连接' : connectionStatus === 'connecting' ? '连接中...' : '未连接'}</span>
          </div>

          <form onSubmit={handleSubmit} className="space-y-4">
            <div>
              <label htmlFor="username" className="auth-label block text-sm font-medium mb-1">
                用户名
              </label>
              <input
                id="username"
                type="text"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                disabled={loading}
                className="auth-input w-full px-4 py-2.5 rounded-xl"
                placeholder="请输入用户名"
                autoComplete="username"
                autoFocus
              />
            </div>

            <div>
              <label htmlFor="password" className="auth-label block text-sm font-medium mb-1">
                密码
              </label>
              <input
                id="password"
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                disabled={loading}
                className="auth-input w-full px-4 py-2.5 rounded-xl"
                placeholder="请输入密码（至少 6 位）"
                autoComplete="new-password"
              />
            </div>

            <div>
              <label htmlFor="confirmPassword" className="auth-label block text-sm font-medium mb-1">
                确认密码
              </label>
              <input
                id="confirmPassword"
                type="password"
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                disabled={loading}
                className="auth-input w-full px-4 py-2.5 rounded-xl"
                placeholder="请再次输入密码"
                autoComplete="new-password"
              />
            </div>

            {error && (
              <div className="auth-error px-4 py-3 rounded-xl text-sm">
                {error}
              </div>
            )}

            {success && (
              <div className="auth-success px-4 py-3 rounded-xl text-sm">
                {success}
              </div>
            )}

            <button
              type="submit"
              disabled={loading || connectionStatus !== 'connected'}
              className="auth-submit w-full rounded-xl py-2.5 px-4 font-medium text-white flex items-center justify-center"
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

          <div className="mt-6 text-center">
            <button
              onClick={() => navigate('/login')}
              className="text-sm font-medium text-cyan-700 hover:text-cyan-800"
            >
              ← 返回登录页面
            </button>
          </div>

          <div className="mt-8 border-t border-slate-200/70 pt-4 text-center text-xs text-slate-500">
            <p>支持好友私聊 · 群聊 · AI 对话</p>
            <p className="mt-1">Powered by C++17 + Boost.Asio + React</p>
          </div>
        </section>
      </div>
    </div>
  )
}
