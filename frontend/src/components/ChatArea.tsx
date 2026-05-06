import { useState, useRef, useEffect } from 'react'
import { ArrowLeft, MessageSquare } from 'lucide-react'
import type { Message } from '../types'

interface ChatAreaProps {
  messages: Message[]
  currentChat?: string | null
  onSendMessage?: (content: string) => void
  disabled?: boolean
  placeholder?: string
}

export default function ChatArea({
  messages,
  currentChat,
  onSendMessage,
  disabled = false,
  placeholder = '输入消息... (Enter 发送，Shift+Enter 换行)',
}: ChatAreaProps) {
  const [inputMessage, setInputMessage] = useState('')
  const messagesEndRef = useRef<HTMLDivElement>(null)
  const inputRef = useRef<HTMLTextAreaElement>(null)

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' })
  }

  useEffect(() => {
    scrollToBottom()
  }, [messages])

  const handleSendMessage = () => {
    if (!inputMessage.trim() || !onSendMessage) return
    onSendMessage(inputMessage.trim())
    setInputMessage('')
  }

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault()
      handleSendMessage()
    }
  }

  if (!currentChat) {
    return (
      <div className="h-full flex items-center justify-center theme-muted">
        <div className="text-center">
          <ArrowLeft size={42} className="mx-auto mb-4 theme-muted" />
          <p className="text-lg">从左侧选择一个聊天</p>
          <p className="text-sm mt-2">开始对话</p>
        </div>
      </div>
    )
  }

  return (
    <div className="flex flex-col h-full">
      <div className="flex-1 overflow-y-auto theme-surface p-6">
        {messages.length === 0 ? (
          <div className="h-full flex items-center justify-center theme-muted">
            <div className="text-center">
              <MessageSquare size={34} className="mx-auto mb-4 theme-muted" />
              <p>还没有消息</p>
              <p className="text-sm">发送一条消息开始聊天吧</p>
            </div>
          </div>
        ) : (
          <div className="space-y-4">
            {messages.map((msg, index) => (
              <div key={index} className={`flex ${msg.is_own ? 'justify-end' : 'justify-start'}`}>
                <div
                  className={`max-w-md px-4 py-2 rounded-2xl ${
                    msg.is_own
                      ? 'theme-message-own rounded-br-none'
                      : 'theme-message-other rounded-bl-none shadow'
                  }`}
                >
                  {!msg.is_own && <p className="text-xs theme-muted mb-1">{msg.from_user}</p>}
                  <p className="text-sm">{msg.content}</p>
                  <p className={`text-xs mt-1 ${msg.is_own ? 'text-blue-100' : 'theme-muted'}`}>
                    {msg.timestamp}
                  </p>
                </div>
              </div>
            ))}
            <div ref={messagesEndRef} />
          </div>
        )}
      </div>

      <div className="theme-surface theme-divider p-4">
        <div className="flex items-end space-x-3">
          <textarea
            ref={inputRef}
            value={inputMessage}
            onChange={(e) => setInputMessage(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder={placeholder}
            rows={1}
            disabled={disabled}
            className="flex-1 px-4 py-3 theme-input rounded-xl resize-none disabled:bg-gray-100"
            style={{ minHeight: '48px', maxHeight: '120px' }}
          />
          <button
            onClick={handleSendMessage}
            disabled={!inputMessage.trim() || disabled}
            className="px-6 py-3 bg-cyan-600 hover:bg-cyan-700 disabled:bg-gray-400 text-white font-medium rounded-xl transition-colors"
          >
            发送
          </button>
        </div>
      </div>
    </div>
  )
}
