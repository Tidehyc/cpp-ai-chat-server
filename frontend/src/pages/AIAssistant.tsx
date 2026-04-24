import { useState, useEffect, useRef } from 'react'
import { useNavigate, Link } from 'react-router-dom'
import ReactMarkdown from 'react-markdown'
import remarkBreaks from 'remark-breaks'
import { useUserStore } from '../stores/userStore'
import { useMessageStore } from '../stores/messageStore'
import { wsService } from '../services/websocket'
import { Bot, BookOpen, Code2, Download, Globe2, History, Lightbulb, MessageSquare, PlusCircle, Settings, Sparkles, Trash2, Users } from 'lucide-react'
import type { ConnectionStatus } from '../types'

// 模态框组件
import ContextModal from '../components/ContextModal'
import HistoryModal from '../components/HistoryModal'
import ExportModal from '../components/ExportModal'

// AI 预设
const AI_PRESETS = [
  { name: 'teacher', label: '老师', description: '耐心解答问题' },
  { name: 'coder', label: '程序员', description: '编程技术专家' },
  { name: 'friend', label: '朋友', description: '知心聊天伙伴' },
  { name: 'interviewer', label: '面试官', description: '模拟面试场景' },
  { name: 'critic', label: '评论家', description: '专业评论分析' },
  { name: 'coach', label: '健身教练', description: '健身指导建议' },
]

export default function AIAssistant() {
  const navigate = useNavigate()
  
  // Store 状态
  const { username, status, showLogoutConfirm, setShowLogoutConfirm, logout } = useUserStore()
  const { addMessage, clearMessages } = useMessageStore()

  // 本地状态
  const [inputMessage, setInputMessage] = useState('')
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>('disconnected')
  const [selectedPreset, setSelectedPreset] = useState('friend')
  const [showPresetMenu, setShowPresetMenu] = useState(false)
  const [showCreatePresetModal, setShowCreatePresetModal] = useState(false)
  const [newPresetName, setNewPresetName] = useState('')
  const [newPresetDescription, setNewPresetDescription] = useState('')

  // 模态框状态
  const [showContextModal, setShowContextModal] = useState(false)
  const [showHistoryModal, setShowHistoryModal] = useState(false)
  const [showExportModal, setShowExportModal] = useState(false)

  // 消息列表 ref
  const messagesEndRef = useRef<HTMLDivElement>(null)
  const inputRef = useRef<HTMLTextAreaElement>(null)

  // 使用 ref 包装处理器，避免依赖项变化
  const handleAIResponseRef = useRef<(message: any) => void>(() => {})
  const handleSystemMessageRef = useRef<(message: any) => void>(() => {})

  // AI 对话消息（使用 ai_chat 作为 key）- 使用 selector 确保状态更新时重新渲染
  const aiMessages = useMessageStore(state => state.getMessages('ai_chat'))

  // 滚动到底部
  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' })
  }

  useEffect(() => {
    scrollToBottom()
  }, [aiMessages])

  // 设置处理器
  handleAIResponseRef.current = (message: any) => {
    console.log('[AI] Received AI response:', message)
    const content = message.content || ''

    addMessage('ai_chat', {
      type: 'ai',
      from_user: 'AI',
      content,
      timestamp: message.timestamp || new Date().toLocaleString('zh-CN'),
      is_ai: true,
      is_own: false,
    })
  }

  handleSystemMessageRef.current = (message: any) => {
    const content = message.content || ''
    console.log('[AI] System message:', content)
    
    // 预设切换成功
    if (content.includes('预设') || content.includes('preset')) {
      const match = content.match(/使用预设：(\S+)/) || content.match(/using preset: (\S+)/i)
      if (match) {
        setSelectedPreset(match[1])
      }
    }
  }

  // 初始化 WebSocket 连接
  useEffect(() => {
    const initWebSocket = async () => {
      try {
        setConnectionStatus('connecting')
        await wsService.connect()
        setConnectionStatus('connected')
      } catch (err) {
        setConnectionStatus('disconnected')
      }
    }

    initWebSocket()

    // 监听连接状态
    const unsubscribeStatus = wsService.onConnectionStatusChange((status) => {
      setConnectionStatus(status)
    })

    // 监听 AI 响应
    const aiResponseHandler = (msg: any) => handleAIResponseRef.current(msg)
    const systemMessageHandler = (msg: any) => handleSystemMessageRef.current(msg)

    wsService.on('ai_response', aiResponseHandler)
    wsService.on('system', systemMessageHandler)

    return () => {
      unsubscribeStatus()
      wsService.off('ai_response', aiResponseHandler)
      wsService.off('system', systemMessageHandler)
    }
  }, [])

  // 发送消息
  const handleSendMessage = () => {
    if (!inputMessage.trim()) return

    const content = inputMessage.trim()

    // 添加自己的消息到 UI
    addMessage('ai_chat', {
      type: 'ai',
      from_user: username || 'me',
      content,
      timestamp: new Date().toLocaleString('zh-CN'),
      is_own: true,
    })

    // 发送 AI 消息（带上当前预设）
    wsService.sendAIMessage(content)

    setInputMessage('')
  }

  // 处理键盘事件
  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault()
      handleSendMessage()
    }
  }

  // 切换预设
  const handleSelectPreset = (presetName: string) => {
    console.log('[AI] Switching preset to:', presetName)
    setSelectedPreset(presetName)
    wsService.usePreset(presetName)
    setShowPresetMenu(false)
    
    // 添加系统提示消息
    const presetLabel = AI_PRESETS.find(p => p.name === presetName)?.label || presetName
    addMessage('ai_chat', {
      type: 'ai',
      from_user: '系统',
      content: `已切换预设：${presetLabel}`,
      timestamp: new Date().toLocaleString('zh-CN'),
      is_ai: false,
      is_own: false,
    })
  }

  // 创建自定义预设
  const handleCreatePreset = () => {
    if (!newPresetName.trim()) return

    // 这里可以调用后端 API 创建自定义预设
    // 暂时只在前端显示
    const newPreset = {
      name: newPresetName.trim().toLowerCase().replace(/\s+/g, '_'),
      label: newPresetName.trim(),
      description: newPresetDescription || '自定义预设',
    }
    
    setSelectedPreset(newPreset.name)
    setNewPresetName('')
    setNewPresetDescription('')
    setShowCreatePresetModal(false)
  }

  // 清空对话
  const handleClear = () => {
    clearMessages('ai_chat')
    wsService.clearHistory()
  }

  // 登出
  const handleLogout = () => {
    wsService.logout()
    logout()
    localStorage.removeItem('user-storage')
    navigate('/login')
  }

  // 连接状态样式
  const getStatusColor = () => {
    switch (connectionStatus) {
      case 'connected': return 'text-green-600'
      case 'connecting': return 'text-yellow-600'
      case 'disconnected': return 'text-red-600'
    }
  }

  const getStatusText = () => {
    switch (connectionStatus) {
      case 'connected': return '已连接'
      case 'connecting': return '连接中...'
      case 'disconnected': return '未连接'
    }
  }

  // 用户状态图标
  const getStatusDotClass = (userStatus: string) => {
    switch (userStatus) {
      case 'online': return 'status-dot status-dot-online'
      case 'busy': return 'status-dot status-dot-busy'
      case 'away': return 'status-dot status-dot-away'
      case 'offline': return 'status-dot status-dot-offline'
      default: return 'status-dot status-dot-offline'
    }
  }

  return (
    <div className="chat-shell page-enter h-screen flex flex-col md:flex-row">
      {/* 左侧边栏 - AI 预设 */}
      <div className="chat-sidebar w-full md:w-80 lg:w-96 flex flex-col">
        {/* 标题栏 */}
        <div className="chat-sidebar-header h-16 flex items-center justify-between px-5">
          <h1 className="chat-title text-lg font-semibold icon-label"><Sparkles size={16} />AI 助手</h1>
        </div>

        {/* 当前预设 */}
        <div className="chat-search-wrap p-4 border-b border-slate-200/70">
          <div className="relative">
            <button
              onClick={() => setShowPresetMenu(!showPresetMenu)}
              className="w-full rounded-xl border border-cyan-200 bg-cyan-50/70 px-4 py-3 text-left transition-colors hover:bg-cyan-100/70"
            >
              <p className="text-xs text-slate-500 mb-1">当前预设</p>
              <p className="font-medium text-slate-900">
                {AI_PRESETS.find(p => p.name === selectedPreset)?.label || '朋友'}
              </p>
            </button>

            {/* 预设菜单 */}
            {showPresetMenu && (
              <div className="chat-menu-panel absolute top-full left-0 right-0 mt-2 z-10 max-h-64 overflow-y-auto">
                {AI_PRESETS.map((preset) => (
                  <button
                    key={preset.name}
                    onClick={() => handleSelectPreset(preset.name)}
                    className={`w-full px-4 py-3 text-left transition-colors hover:bg-cyan-50/70 ${
                      selectedPreset === preset.name ? 'bg-cyan-50/60' : ''
                    }`}
                  >
                    <p className="font-medium text-slate-900">{preset.label}</p>
                    <p className="text-xs text-slate-500">{preset.description}</p>
                  </button>
                ))}
                <div className="border-t border-slate-200">
                  <button
                    onClick={() => {
                      setShowCreatePresetModal(true)
                      setShowPresetMenu(false)
                    }}
                    className="w-full px-4 py-3 text-left transition-colors hover:bg-cyan-50/70 text-cyan-700"
                  >
                    <span className="icon-label"><PlusCircle size={14} />创建自定义预设</span>
                  </button>
                </div>
              </div>
            )}
          </div>
        </div>

        {/* 预设说明 */}
        <div className="chat-friend-list flex-1 overflow-y-auto p-4">
          <div className="rounded-xl border border-slate-200 bg-white/75 p-4 shadow-sm">
            <h3 className="text-sm font-medium text-slate-900 mb-2 icon-label"><Lightbulb size={14} />使用说明</h3>
            <ul className="text-xs text-slate-600 space-y-1">
              <li>• 选择不同的预设切换 AI 角色</li>
              <li>• 创建自定义预设定义专属 AI</li>
              <li>• 对话历史会保存在本地</li>
              <li>• 点击清空按钮可重置对话</li>
            </ul>
          </div>
        </div>

        {/* 底部用户信息 */}
        <div className="chat-userbar h-14 flex items-center justify-between px-4">
          <div className="flex items-center">
            <span className={`mr-2 ${getStatusDotClass(status)}`} />
            <span className="text-sm font-medium text-slate-700">{username}</span>
          </div>
          <button
            onClick={() => setShowLogoutConfirm(true)}
            className="text-sm text-rose-600 hover:text-rose-700 font-medium"
          >
            退出
          </button>
        </div>
      </div>

      {/* 右侧聊天区域 */}
      <div className="chat-main flex-1 flex flex-col">
        {/* 顶部导航栏 */}
        <div className="chat-topnav h-14 flex items-center justify-between px-6">
          <div className="flex items-center space-x-4">
            <span className="topbar-title icon-label"><Sparkles size={16} />AI 助手</span>
            <nav className="flex items-center space-x-2">
              <Link
                to="/"
                className="nav-pill"
                title="私聊"
              >
                <span className="icon-label"><MessageSquare size={14} />私聊</span>
              </Link>
              <Link
                to="/contact"
                className="nav-pill"
                title="群聊"
              >
                <span className="icon-label"><Users size={14} />群聊</span>
              </Link>
              <Link
                to="/settings"
                className="nav-pill"
                title="设置"
              >
                <span className="icon-label"><Settings size={14} />设置</span>
              </Link>
            </nav>
          </div>
          <div className="flex items-center space-x-2">
            <span className={`text-sm ${getStatusColor()}`}>
              <span className={`inline-block mr-1.5 align-middle ${getStatusDotClass(status)}`} />
              {status === 'online' ? '在线' :
               status === 'busy' ? '忙碌' :
               status === 'away' ? '离开' : '离线'}
            </span>
          </div>
        </div>

        {/* 聊天顶部栏 */}
        <div className="chat-roombar h-14 flex items-center justify-between px-6">
          <div className="flex items-center">
            <Bot size={18} className="mr-2 text-cyan-700" />
            <span className="font-semibold text-slate-900">AI 助手</span>
            <span className="ml-2 text-xs text-slate-500 bg-slate-100 px-2 py-1 rounded">
              {AI_PRESETS.find(p => p.name === selectedPreset)?.label || '朋友'}
            </span>
          </div>

          <div className="flex items-center space-x-2">
            <button
              onClick={handleClear}
              className="action-chip"
              title="清空对话"
            >
              <span className="icon-label"><Trash2 size={14} />清空</span>
            </button>
            <button
              onClick={() => setShowContextModal(true)}
              className="action-chip"
              title="上下文管理"
            >
              <span className="icon-label"><BookOpen size={14} />上下文</span>
            </button>
            <button
              onClick={() => setShowHistoryModal(true)}
              className="action-chip"
              title="历史记录"
            >
              <span className="icon-label"><History size={14} />历史</span>
            </button>
            <button
              onClick={() => setShowExportModal(true)}
              className="action-chip"
              title="导出记录"
            >
              <span className="icon-label"><Download size={14} />导出</span>
            </button>
          </div>
        </div>

        {/* 消息列表 */}
        <div className="chat-thread flex-1 overflow-y-auto p-6">
          {aiMessages.length === 0 ? (
            <div className="h-full flex items-center justify-center text-slate-400">
              <div className="text-center max-w-md">
                <Sparkles size={44} className="mx-auto mb-4 text-cyan-600" />
                <h2 className="text-xl font-semibold text-slate-700 mb-2">
                  你好，我是你的 AI 助手
                </h2>
                <p className="text-slate-500 mb-6">
                  我可以回答你的问题、帮助你编程、或者只是聊聊天
                </p>
                <div className="grid grid-cols-2 gap-3 text-left">
                  <div className="bg-white/90 p-3 rounded-lg shadow-sm">
                    <Lightbulb size={17} className="text-amber-500" />
                    <p className="text-sm text-slate-600 mt-1">解答各种问题</p>
                  </div>
                  <div className="bg-white/90 p-3 rounded-lg shadow-sm">
                    <Code2 size={17} className="text-indigo-500" />
                    <p className="text-sm text-slate-600 mt-1">编程技术支持</p>
                  </div>
                  <div className="bg-white/90 p-3 rounded-lg shadow-sm">
                    <BookOpen size={17} className="text-emerald-600" />
                    <p className="text-sm text-slate-600 mt-1">写作润色修改</p>
                  </div>
                  <div className="bg-white/90 p-3 rounded-lg shadow-sm">
                    <Globe2 size={17} className="text-sky-600" />
                    <p className="text-sm text-slate-600 mt-1">翻译多国语言</p>
                  </div>
                </div>
              </div>
            </div>
          ) : (
            <div className="space-y-4">
              {aiMessages.map((msg, index) => (
                <div
                  key={index}
                  className={`flex ${msg.is_own ? 'justify-end' : 'justify-start'}`}
                >
                  <div
                    className={`message-bubble msg-bubble ${
                      msg.is_own
                        ? 'msg-bubble-own'
                        : msg.is_ai
                        ? 'msg-bubble-other'
                        : 'msg-bubble-other'
                    }`}
                  >
                    {!msg.is_own && msg.is_ai && (
                      <p className="msg-sender"><Bot size={12} /><span>AI 助手</span></p>
                    )}
                    {msg.is_ai ? (
                      <div className="text-sm text-slate-800 leading-relaxed">
                        <ReactMarkdown
                          remarkPlugins={[remarkBreaks]}
                          components={{
                            h1: ({node, ...props}) => <h1 className="text-xl font-bold mt-4 mb-2" {...props} />,
                            h2: ({node, ...props}) => <h2 className="text-lg font-bold mt-3 mb-2" {...props} />,
                            h3: ({node, ...props}) => <h3 className="text-base font-bold mt-3 mb-1" {...props} />,
                            h4: ({node, ...props}) => <h4 className="text-sm font-bold mt-2 mb-1" {...props} />,
                            p: ({node, ...props}) => <p className="my-1" {...props} />,
                            ul: ({node, ...props}) => <ul className="list-disc list-inside my-2 space-y-1" {...props} />,
                            ol: ({node, ...props}) => <ol className="list-decimal list-inside my-2 space-y-1" {...props} />,
                            li: ({node, ...props}) => <li className="ml-2" {...props} />,
                            code: ({node, inline, ...props}: any) =>
                              inline ? (
                                <code className="bg-slate-100 px-1 py-0.5 rounded text-xs font-mono" {...props} />
                              ) : (
                                <code className="block bg-slate-100 p-3 rounded my-2 text-xs font-mono overflow-x-auto" {...props} />
                              ),
                            hr: ({node, ...props}) => <hr className="my-4 border-slate-300" {...props} />,
                            strong: ({node, ...props}) => <strong className="font-bold" {...props} />,
                          }}
                        >
                          {msg.content}
                        </ReactMarkdown>
                      </div>
                    ) : (
                      <p className="text-sm whitespace-pre-wrap">{msg.content}</p>
                    )}
                    <div className={`msg-meta ${msg.is_own ? 'msg-meta-own' : 'msg-meta-other'}`}>
                      <p>{msg.timestamp}</p>
                    </div>
                  </div>
                </div>
              ))}
              <div ref={messagesEndRef} />
            </div>
          )}
        </div>

        {/* 输入框 */}
        <div className="chat-composer p-4">
          <div className="flex items-end space-x-3">
            <textarea
              ref={inputRef}
              value={inputMessage}
              onChange={(e) => setInputMessage(e.target.value)}
              onKeyDown={handleKeyDown}
              placeholder="向 AI 提问... (Enter 发送，Shift+Enter 换行)"
              rows={1}
              className="flex-1 resize-none rounded-xl border border-white/60 bg-white/85 px-4 py-3 text-slate-900 shadow-sm outline-none transition-all focus:border-cyan-400 focus:bg-white focus:ring-2 focus:ring-cyan-200"
              style={{ minHeight: '48px', maxHeight: '120px' }}
            />
            <button
              onClick={handleSendMessage}
              disabled={!inputMessage.trim() || connectionStatus !== 'connected'}
              className="rounded-xl bg-gradient-to-r from-cyan-600 to-sky-600 px-6 py-3 font-medium text-white transition-all hover:from-cyan-500 hover:to-sky-500 disabled:cursor-not-allowed disabled:from-slate-400 disabled:to-slate-400"
            >
              发送
            </button>
          </div>
        </div>
      </div>

      {/* 创建预设模态框 */}
      {showCreatePresetModal && (
        <div className="modal-overlay">
          <div className="modal-card max-w-md">
            <h3 className="modal-title icon-label"><PlusCircle size={18} />创建自定义预设</h3>
            <div className="space-y-4">
              <div>
                <label className="modal-label">
                  预设名称
                </label>
                <input
                  type="text"
                  value={newPresetName}
                  onChange={(e) => setNewPresetName(e.target.value)}
                  placeholder="例如：翻译助手"
                  className="modal-input"
                  autoFocus
                />
              </div>
              <div>
                <label className="modal-label">
                  预设描述（可选）
                </label>
                <input
                  type="text"
                  value={newPresetDescription}
                  onChange={(e) => setNewPresetDescription(e.target.value)}
                  placeholder="例如：专业翻译多国语言"
                  className="modal-input"
                />
              </div>
              <div className="flex justify-end space-x-3">
                <button
                  onClick={() => {
                    setShowCreatePresetModal(false)
                    setNewPresetName('')
                    setNewPresetDescription('')
                  }}
                  className="modal-btn modal-btn-ghost"
                >
                  取消
                </button>
                <button
                  onClick={handleCreatePreset}
                  disabled={!newPresetName.trim()}
                  className="modal-btn modal-btn-primary disabled:cursor-not-allowed disabled:opacity-55"
                >
                  创建
                </button>
              </div>
            </div>
          </div>
        </div>
      )}

      {/* 上下文管理弹窗 */}
      <ContextModal
        isOpen={showContextModal}
        onClose={() => setShowContextModal(false)}
      />

      {/* 历史记录弹窗 */}
      <HistoryModal
        isOpen={showHistoryModal}
        onClose={() => setShowHistoryModal(false)}
        target="ai_chat"
      />

      {/* 导出弹窗 */}
      <ExportModal
        isOpen={showExportModal}
        onClose={() => setShowExportModal(false)}
        target="ai_chat"
      />

      {/* 登出确认对话框 */}
      {showLogoutConfirm && (
        <div className="modal-overlay">
          <div className="modal-card max-w-sm">
            <h3 className="modal-title mb-2">确认退出</h3>
            <p className="text-slate-600 mb-6">确定要退出登录吗？</p>
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
