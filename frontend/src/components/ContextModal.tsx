import { useState, useEffect } from 'react'
import { BookOpen, Trash2 } from 'lucide-react'
import { wsService } from '../services/websocket'

interface ContextModalProps {
  isOpen: boolean
  onClose: () => void
}

export default function ContextModal({ isOpen, onClose }: ContextModalProps) {
  const [contextSize, setContextSize] = useState<number>(10)
  const [currentContext, setCurrentContext] = useState<number>(0)
  const [loading, setLoading] = useState(false)

  // 获取当前上下文
  useEffect(() => {
    if (isOpen) {
      wsService.sendCommand('/context')
      
      const handleContextMessage = (message: any) => {
        const content = message.content || ''
        const match = content.match(/上下文大小：(\d+)/) || content.match(/context size: (\d+)/i)
        if (match) {
          setCurrentContext(parseInt(match[1]))
        }
      }

      wsService.on('system', handleContextMessage)
      
      return () => {
        wsService.off('system', handleContextMessage)
      }
    }
  }, [isOpen])

  // 设置上下文大小
  const handleSetContext = () => {
    setLoading(true)
    wsService.sendCommand(`/context set ${contextSize}`)
    
    setTimeout(() => {
      setLoading(false)
      onClose()
    }, 1000)
  }

  // 清空对话
  const handleClear = () => {
    wsService.sendCommand('/clear')
    onClose()
  }

  if (!isOpen) return null

  return (
    <div className="modal-overlay">
      <div className="theme-modal p-6 w-full max-w-md">
        <h3 className="text-lg font-semibold mb-4 icon-label"><BookOpen size={17} />上下文管理</h3>
        
        <div className="space-y-4">
          {/* 当前上下文大小 */}
          <div className="theme-surface rounded-lg p-4">
            <p className="text-sm theme-muted mb-1">当前上下文大小</p>
            <p className="text-2xl font-bold text-blue-600">{currentContext} 轮</p>
          </div>

          {/* 设置新的上下文大小 */}
          <div>
            <label className="block text-sm font-medium theme-muted mb-2">
              设置上下文大小（轮数）
            </label>
            <input
              type="number"
              min="1"
              max="50"
              value={contextSize}
              onChange={(e) => setContextSize(parseInt(e.target.value) || 0)}
              className="theme-input"
            />
            <p className="text-xs theme-muted mt-1">
              上下文越大，AI 记住的对话越多，但响应速度可能越慢
            </p>
          </div>

          {/* 操作按钮 */}
          <div className="flex flex-col space-y-3 pt-4 border-t theme-divider">
            <button
              onClick={handleSetContext}
              disabled={loading}
              className="w-full bg-cyan-600 hover:bg-cyan-700 disabled:bg-slate-300 text-white font-medium py-2 px-4 rounded-lg transition-colors"
            >
              {loading ? '设置中...' : '设置上下文'}
            </button>
            
            <button
              onClick={handleClear}
              className="w-full bg-rose-50 hover:bg-rose-100 text-rose-600 font-medium py-2 px-4 rounded-lg transition-colors"
            >
              <span className="icon-label justify-center"><Trash2 size={15} />清空对话历史</span>
            </button>
          </div>
        </div>

        {/* 关闭按钮 */}
        <div className="mt-4 flex justify-end">
          <button
            onClick={onClose}
            className="px-4 py-2 theme-muted hover:bg-white/60 rounded-lg transition-colors"
          >
            关闭
          </button>
        </div>
      </div>
    </div>
  )
}
