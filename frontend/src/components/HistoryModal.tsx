import { useState, useEffect } from 'react'
import { useMessageStore } from '../stores/messageStore'
import { wsService } from '../services/websocket'
import type { Message } from '../types'

interface HistoryModalProps {
  isOpen: boolean
  onClose: () => void
  target: string | null
}

export default function HistoryModal({ isOpen, onClose, target }: HistoryModalProps) {
  const [history, setHistory] = useState<Message[]>([])
  const [loading, setLoading] = useState(false)
  const [loadCount, setLoadCount] = useState(50)

  // 从 store 获取本地消息
  const localMessages = useMessageStore((state) => 
    target ? state.getMessages(target) : []
  )

  useEffect(() => {
    if (isOpen && target) {
      loadHistory()
    }
  }, [isOpen, target])

  // 加载历史记录
  const loadHistory = () => {
    if (!target) return
    
    setLoading(true)
    
    // 发送历史查询命令
    const targetType = target.startsWith('#') ? target : `@${target}`
    wsService.sendCommand(`/history ${targetType} ${loadCount}`)
    
    // 监听历史消息响应
    const handleHistoryMessage = (message: any) => {
      const content = message.content || ''
      
      // 解析历史消息列表
      if (content.includes('历史消息')) {
        // 后端返回的历史消息格式可能是多行文本
        const lines = content.split('\n')
        const parsedMessages: Message[] = []
        
        lines.forEach((line: string) => {
          if (line.trim()) {
            // 尝试解析每行消息
            const match = line.match(/\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\]\s*(\S+):\s*(.*)/)
            if (match) {
              parsedMessages.push({
                type: 'room',
                from_user: match[2],
                content: match[3],
                timestamp: match[1],
                is_history: true,
              })
            }
          }
        })
        
        if (parsedMessages.length > 0) {
          setHistory(parsedMessages)
        }
      }
    }

    wsService.on('system', handleHistoryMessage)
    
    setTimeout(() => {
      setLoading(false)
    }, 1500)

    return () => {
      wsService.off('system', handleHistoryMessage)
    }
  }

  // 加载更多
  const handleLoadMore = () => {
    setLoadCount(prev => prev + 50)
    loadHistory()
  }

  if (!isOpen || !target) return null

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-xl w-full max-w-2xl max-h-[80vh] flex flex-col">
        {/* 标题 */}
        <div className="h-14 border-b border-gray-200 flex items-center justify-between px-6">
          <h3 className="text-lg font-semibold">📜 聊天记录 - {target}</h3>
          <button
            onClick={onClose}
            className="p-2 hover:bg-gray-100 rounded-lg transition-colors"
          >
            ✕
          </button>
        </div>

        {/* 消息列表 */}
        <div className="flex-1 overflow-y-auto p-6">
          {loading ? (
            <div className="flex items-center justify-center py-12">
              <svg className="animate-spin h-8 w-8 text-blue-600" fill="none" viewBox="0 0 24 24">
                <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
              </svg>
              <span className="ml-3 text-gray-600">加载中...</span>
            </div>
          ) : history.length > 0 ? (
            <div className="space-y-4">
              {history.map((msg, index) => (
                <div
                  key={index}
                  className={`flex ${msg.is_own ? 'justify-end' : 'justify-start'}`}
                >
                  <div
                    className={`max-w-md px-4 py-2 rounded-2xl ${
                      msg.is_own
                        ? 'bg-blue-600 text-white rounded-br-none'
                        : 'bg-white text-gray-900 rounded-bl-none shadow'
                    }`}
                  >
                    {!msg.is_own && (
                      <p className="text-xs text-gray-500 mb-1">{msg.from_user}</p>
                    )}
                    <p className="text-sm">{msg.content}</p>
                    <p className={`text-xs mt-1 ${msg.is_own ? 'text-blue-100' : 'text-gray-400'}`}>
                      {msg.timestamp}
                    </p>
                  </div>
                </div>
              ))}
              
              {/* 加载更多 */}
              <div className="text-center pt-4">
                <button
                  onClick={handleLoadMore}
                  className="text-blue-600 hover:text-blue-700 text-sm font-medium"
                >
                  加载更多...
                </button>
              </div>
            </div>
          ) : localMessages.length > 0 ? (
            // 显示本地消息
            <div className="space-y-4">
              {localMessages.map((msg, index) => (
                <div
                  key={index}
                  className={`flex ${msg.is_own ? 'justify-end' : 'justify-start'}`}
                >
                  <div
                    className={`max-w-md px-4 py-2 rounded-2xl ${
                      msg.is_own
                        ? 'bg-blue-600 text-white rounded-br-none'
                        : 'bg-white text-gray-900 rounded-bl-none shadow'
                    }`}
                  >
                    {!msg.is_own && (
                      <p className="text-xs text-gray-500 mb-1">{msg.from_user}</p>
                    )}
                    <p className="text-sm">{msg.content}</p>
                    <p className={`text-xs mt-1 ${msg.is_own ? 'text-blue-100' : 'text-gray-400'}`}>
                      {msg.timestamp}
                    </p>
                  </div>
                </div>
              ))}
            </div>
          ) : (
            <div className="text-center py-12 text-gray-400">
              <span className="text-4xl mb-4 block">📜</span>
              <p>暂无历史记录</p>
            </div>
          )}
        </div>

        {/* 底部按钮 */}
        <div className="h-14 border-t border-gray-200 flex items-center justify-end px-6 space-x-3">
          <button
            onClick={onClose}
            className="px-4 py-2 text-gray-600 hover:bg-gray-100 rounded-lg transition-colors"
          >
            关闭
          </button>
        </div>
      </div>
    </div>
  )
}
