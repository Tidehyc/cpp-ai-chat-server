import { useState, useEffect } from 'react'
import { useNavigate } from 'react-router-dom'
import { Sparkles } from 'lucide-react'
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
    <div className="auth-shell min-h-screen flex items-center justify-center p-4">
      <div className="auth-wrap w-full max-w-5xl grid grid-cols-1 lg:grid-cols-[1.1fr_0.9fr] gap-0 overflow-hidden rounded-3xl border border-white/40 bg-white/40 shadow-2xl">
        <section className="auth-hero hidden lg:flex flex-col justify-between p-10">
          <div>
            <div className="auth-brand mb-6 inline-flex items-center gap-3 rounded-2xl border border-white/60 bg-white/60 px-4 py-2 text-sm font-semibold text-slate-700">
              <span className="text-lg">✦</span>
              Flow Chat Engine
            </div>
            <h1 className="auth-hero-title mb-4 text-4xl font-bold leading-tight text-slate-900">
              更流畅的团队沟通
              <br />
              与 AI 协作体验
            </h1>
            <p className="text-slate-600 text-base leading-7">
              低延迟消息系统、多轮 AI 对话与协作房间统一在一个界面里。
              现在登录，继续你的实时会话。
            </p>
          </div>

          <div className="auth-hero-tags flex flex-wrap gap-2 text-xs text-slate-600">
            <span className="auth-tag">C++17</span>
            <span className="auth-tag">Boost.Asio</span>
            <span className="auth-tag">WebSocket</span>
            <span className="auth-tag">React + TS</span>
          </div>
        </section>

        <section className="auth-card p-6 md:p-10">
          <div className="text-center mb-8">
            <div className="inline-flex items-center justify-center w-16 h-16 rounded-2xl bg-gradient-to-br from-cyan-600 to-blue-700 shadow-lg shadow-cyan-500/30 mb-4">
              <Sparkles size={30} className="text-white" />
            </div>
            <h2 className="text-3xl font-bold text-slate-900 mb-2">欢迎回来</h2>
            <p className="text-slate-600">登录你的账号，继续聊天</p>
          </div>

          <div className="flex items-center justify-center mb-6">
            <div className={`w-2 h-2 rounded-full ${getStatusColor()} mr-2 animate-pulse`}></div>
            <span className="text-sm text-slate-500">{getStatusText()}</span>
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
                placeholder="请输入密码"
                autoComplete="current-password"
              />
            </div>

            {error && (
              <div className="auth-error px-4 py-3 rounded-xl text-sm">
                {error}
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
                  登录中...
                </>
              ) : (
                '登录'
              )}
            </button>
          </form>

          <div className="mt-6 text-center text-sm text-slate-500">
            <p>
              还没有账号？{' '}
              <button
                type="button"
                onClick={() => navigate('/register')}
                className="font-medium text-cyan-700 hover:text-cyan-800"
              >
                立即注册
              </button>
            </p>
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
