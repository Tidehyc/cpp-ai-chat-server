import { useState, useEffect, useRef } from 'react'
import { useNavigate, useLocation } from 'react-router-dom'
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
  // 注册面板状态
  const [regUsername, setRegUsername] = useState('')
  const [regPassword, setRegPassword] = useState('')
  const [regConfirmPassword, setRegConfirmPassword] = useState('')
  const registerUsernameRef = useRef('')
  const registerPasswordRef = useRef('')

  // UI 状态
  const [error, setError] = useState('')
  const [loading, setLoading] = useState(false)
  const [regLoading, setRegLoading] = useState(false)
  const [regError, setRegError] = useState('')
  const [regSuccess, setRegSuccess] = useState('')
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('disconnected')
  const location = useLocation()
  const qs = new URLSearchParams(location.search)
  const initialShow = qs.get('register') === '1'
  const [showRegisterPanel, setShowRegisterPanel] = useState(initialShow)

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

    // 注册成功
    if (content.includes('注册成功') || content.includes('Registration successful')) {
      setRegLoading(false)
      setRegSuccess('注册成功！正在自动登录...')
      setRegError('')
      setTimeout(() => {
        // 自动登录使用刚注册的凭据
        wsService.login(registerUsernameRef.current, registerPasswordRef.current)
      }, 500)
      return
    }

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

  // 处理注册提交（在大屏 Auth 面板中使用）
  const handleRegisterSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    setRegError('')
    setRegSuccess('')

    if (!regUsername.trim() || !regPassword.trim()) {
      setRegError('请输入用户名和密码')
      return
    }

    if (regPassword.length < 6) {
      setRegError('密码长度至少为 6 位')
      return
    }

    if (regPassword !== regConfirmPassword) {
      setRegError('两次输入的密码不一致')
      return
    }

    if (connectionStatus !== 'connected') {
      setRegError('未连接到服务器，请稍后重试')
      return
    }

    setRegLoading(true)
    registerUsernameRef.current = regUsername.trim()
    registerPasswordRef.current = regPassword
    wsService.register(regUsername.trim(), regPassword)

    setTimeout(() => {
      if (regLoading && !regSuccess) {
        setRegLoading(false)
        setRegError('请求超时，请检查网络连接')
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
    <div className="theme-page flex min-h-[100dvh] items-start justify-center overflow-y-auto px-4 py-6 lg:items-center lg:py-8">
      <div className="auth-wrap theme-card w-full max-w-5xl grid grid-cols-1 lg:grid-cols-[1.1fr_0.9fr] gap-0 overflow-hidden rounded-3xl my-auto">
        <section className="auth-hero hidden lg:flex flex-col justify-between p-10 theme-surface">
          <div>
            <div className="auth-brand theme-surface mb-6 inline-flex items-center gap-3 rounded-2xl px-4 py-2 text-sm font-semibold theme-muted">
              <span className="text-lg">✦</span>
              Flow Chat Engine
            </div>
            <h1 className="auth-hero-title mb-4 text-4xl font-bold leading-tight text-inherit">
              更流畅的团队沟通
              <br />
              与 AI 协作体验
            </h1>
            <p className="theme-muted text-base leading-7">
              低延迟消息系统、多轮 AI 对话与协作房间统一在一个界面里。
              现在登录，继续你的实时会话。
            </p>
          </div>

          <div className="auth-hero-tags flex flex-wrap gap-2 text-xs theme-muted">
            <span className="auth-tag">C++17</span>
            <span className="auth-tag">Boost.Asio</span>
            <span className="auth-tag">WebSocket</span>
            <span className="auth-tag">React + TS</span>
          </div>
        </section>

        <section className="auth-card theme-card max-h-[calc(100dvh-3rem)] overflow-y-auto p-5 md:p-8 lg:max-h-[calc(100dvh-4rem)]">
          <div className="text-center mb-6">
            <div className="inline-flex items-center justify-center w-14 h-14 rounded-2xl bg-gradient-to-br from-cyan-600 to-blue-700 shadow-lg shadow-cyan-500/30 mb-3">
              <Sparkles size={26} className="text-white" />
            </div>
            <h2 className="text-2xl md:text-3xl font-bold text-inherit mb-2">欢迎回来</h2>
            <p className="theme-muted">登录你的账号，继续聊天</p>
          </div>

          <div className="flex items-center justify-center mb-5">
            <div className={`w-2 h-2 rounded-full ${getStatusColor()} mr-2 animate-pulse`}></div>
            <span className="text-sm theme-muted">{getStatusText()}</span>
          </div>

          {/* Sliding auth panels: two panels inside a 200% width slider */}
          <div className="w-full overflow-hidden">
            <div
              className="flex w-[200%] shadow-none"
              style={{ transform: showRegisterPanel ? 'translateX(-50%)' : 'translateX(0)', transition: 'transform 420ms cubic-bezier(.2,.8,.2,1)', willChange: 'transform' }}
            >
              {/* 登录面板 */}
              <div
                className="w-1/2 px-0 md:px-0 py-0 md:py-0 pr-6 transition-shadow duration-300"
                style={{
                  boxShadow: showRegisterPanel ? '0 6px 18px rgba(15,23,42,0.06)' : '0 20px 50px rgba(2,6,23,0.12)',
                  transition: 'box-shadow 420ms',
                  willChange: 'box-shadow',
                }}
              >
                <form onSubmit={handleSubmit} className="space-y-3 p-0 md:p-0">
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
                      className="auth-input w-full px-4 py-[9px] rounded-xl"
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
                      className="auth-input w-full px-4 py-[9px] rounded-xl"
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
                    className="auth-submit w-full rounded-xl py-[9px] px-4 font-medium text-white flex items-center justify-center"
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

                  <div className="mt-5 text-center text-sm theme-muted">
                    <p>
                      还没有账号？{' '}
                      <button
                        type="button"
                        onClick={() => {
                          // 在大屏幕切换为注册面板；小屏幕导航到 /register
                          if (window.matchMedia && window.matchMedia('(min-width: 1024px)').matches) {
                            setShowRegisterPanel(true)
                          } else {
                            navigate('/register')
                          }
                        }}
                        className="font-medium text-cyan-700 hover:text-cyan-800"
                      >
                        立即注册
                      </button>
                    </p>
                  </div>
                </form>
              </div>

              {/* 注册面板 */}
              <div
                className="w-1/2 px-0 md:px-0 py-0 md:py-0 pl-6 transition-shadow duration-300"
                style={{
                  boxShadow: showRegisterPanel ? '0 20px 50px rgba(2,6,23,0.12)' : '0 6px 18px rgba(15,23,42,0.06)',
                  transition: 'box-shadow 420ms',
                  willChange: 'box-shadow',
                }}
              >
                <form onSubmit={handleRegisterSubmit} className="space-y-3 p-0 md:p-0">
                  <div>
                    <label htmlFor="regUsername" className="auth-label block text-sm font-medium mb-1">
                      用户名
                    </label>
                    <input
                      id="regUsername"
                      type="text"
                      value={regUsername}
                      onChange={(e) => setRegUsername(e.target.value)}
                      disabled={regLoading}
                      className="auth-input w-full px-4 py-[9px] rounded-xl"
                      placeholder="请输入用户名"
                      autoComplete="username"
                    />
                  </div>

                  <div>
                    <label htmlFor="regPassword" className="auth-label block text-sm font-medium mb-1">
                      密码
                    </label>
                    <input
                      id="regPassword"
                      type="password"
                      value={regPassword}
                      onChange={(e) => setRegPassword(e.target.value)}
                      disabled={regLoading}
                      className="auth-input w-full px-4 py-[9px] rounded-xl"
                      placeholder="请输入密码（至少 6 位）"
                      autoComplete="new-password"
                    />
                  </div>

                  <div>
                    <label htmlFor="regConfirmPassword" className="auth-label block text-sm font-medium mb-1">
                      确认密码
                    </label>
                    <input
                      id="regConfirmPassword"
                      type="password"
                      value={regConfirmPassword}
                      onChange={(e) => setRegConfirmPassword(e.target.value)}
                      disabled={regLoading}
                      className="auth-input w-full px-4 py-[9px] rounded-xl"
                      placeholder="请再次输入密码"
                      autoComplete="new-password"
                    />
                  </div>

                  {regError && (
                    <div className="auth-error px-4 py-3 rounded-xl text-sm">
                      {regError}
                    </div>
                  )}

                  {regSuccess && (
                    <div className="auth-success px-4 py-3 rounded-xl text-sm">
                      {regSuccess}
                    </div>
                  )}

                  <div className="flex items-center gap-3">
                    <button
                      type="submit"
                      disabled={regLoading || connectionStatus !== 'connected'}
                      className="auth-submit flex-1 rounded-xl py-[9px] px-4 font-medium text-white flex items-center justify-center"
                    >
                      {regLoading ? (
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

                    <button
                      type="button"
                      onClick={() => {
                        if (window.matchMedia && window.matchMedia('(min-width: 1024px)').matches) {
                          setShowRegisterPanel(false)
                        } else {
                          navigate('/login')
                        }
                      }}
                      className="text-sm font-medium text-cyan-700 hover:text-cyan-800"
                    >
                      ← 返回登录
                    </button>
                  </div>
                </form>
              </div>
            </div>
          </div>

          <div className="mt-6 border-t theme-divider pt-3 text-center text-xs theme-muted">
            <p>支持好友私聊 · 群聊 · AI 对话</p>
            <p className="mt-1">Powered by C++17 + Boost.Asio + React</p>
          </div>
        </section>
      </div>
    </div>
  )
}
