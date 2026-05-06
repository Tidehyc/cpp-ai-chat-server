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

  // 现在改为重定向到 /login?register=1，让登录页展示滑动注册面板
  useEffect(() => {
    navigate('/login?register=1', { replace: true })
  }, [navigate])

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
    // Register now redirects to login slider — no standalone page content
    <div className="theme-page flex items-center justify-center p-4">
      <div className="auth-wrap theme-card w-full max-w-5xl grid grid-cols-1 lg:grid-cols-[1.1fr_0.9fr] gap-0 overflow-hidden rounded-3xl">
        <section className="auth-card theme-card p-6 md:p-10 text-center">
          <div className="text-center">
            <h2 className="text-lg font-medium">正在跳转到注册面板…</h2>
            <p className="theme-muted text-sm">如果没有自动跳转，请点击下面按钮。</p>
            <div className="mt-4">
              <button onClick={() => navigate('/login?register=1')} className="btn">前往注册</button>
            </div>
          </div>
        </section>
      </div>
    </div>
  )
}
