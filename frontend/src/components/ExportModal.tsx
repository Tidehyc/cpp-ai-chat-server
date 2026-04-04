import { useState } from 'react'
import { wsService } from '../services/websocket'
import { useMessageStore } from '../stores/messageStore'
import type { Message } from '../types'

interface ExportModalProps {
  isOpen: boolean
  onClose: () => void
  target: string | null
}

export default function ExportModal({ isOpen, onClose, target }: ExportModalProps) {
  const [format, setFormat] = useState<'txt' | 'json'>('txt')
  const [exporting, setExporting] = useState(false)
  const [exported, setExported] = useState(false)

  // 从 store 获取消息
  const messages = useMessageStore((state) =>
    target ? state.getMessages(target) : []
  )

  // 导出聊天记录到文件
  const handleExport = () => {
    if (!target) return

    setExporting(true)

    // 从本地 store 导出消息
    const exportData = messages.map((msg: Message) => ({
      from: msg.from_user,
      to: msg.to_user,
      content: msg.content,
      timestamp: msg.timestamp,
      is_own: msg.is_own,
    }))

    let content: string
    let mimeType: string
    let extension: string

    if (format === 'json') {
      content = JSON.stringify(exportData, null, 2)
      mimeType = 'application/json'
      extension = 'json'
    } else {
      content = exportData.map((msg: any) => 
        `[${msg.timestamp}] ${msg.from}: ${msg.content}`
      ).join('\n')
      mimeType = 'text/plain'
      extension = 'txt'
    }

    // 创建下载链接
    const blob = new Blob([content], { type: mimeType })
    const url = URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url
    link.download = `chat_${target}_${new Date().toISOString().split('T')[0]}.${extension}`
    document.body.appendChild(link)
    link.click()
    document.body.removeChild(link)
    URL.revokeObjectURL(url)

    setExporting(false)
    setExported(true)

    // 2 秒后自动关闭
    setTimeout(() => {
      onClose()
      setExported(false)
    }, 2000)
  }

  if (!isOpen || !target) return null

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-xl p-6 w-full max-w-md">
        <h3 className="text-lg font-semibold mb-4">📥 导出聊天记录</h3>

        {exported ? (
          <div className="text-center py-8">
            <span className="text-5xl mb-4 block">✅</span>
            <p className="text-green-600 font-medium">导出成功！</p>
            <p className="text-sm text-gray-500 mt-2">聊天记录已保存到本地</p>
          </div>
        ) : (
          <>
            {/* 导出目标 */}
            <div className="mb-4">
              <label className="block text-sm font-medium text-gray-700 mb-2">
                导出目标
              </label>
              <div className="bg-gray-50 rounded-lg px-4 py-3">
                <p className="text-lg font-medium text-gray-900">{target}</p>
              </div>
            </div>

            {/* 格式选择 */}
            <div className="mb-6">
              <label className="block text-sm font-medium text-gray-700 mb-2">
                导出格式
              </label>
              <div className="grid grid-cols-2 gap-3">
                <label className="relative cursor-pointer">
                  <input
                    type="radio"
                    name="format"
                    checked={format === 'txt'}
                    onChange={() => setFormat('txt')}
                    className="sr-only"
                  />
                  <div className={`border-2 rounded-lg p-4 transition-colors ${
                    format === 'txt'
                      ? 'border-blue-500 bg-blue-50'
                      : 'border-gray-200 hover:border-gray-300'
                  }`}>
                    <div className="text-center">
                      <span className="text-2xl">📄</span>
                      <p className="text-sm font-medium mt-2">TXT 文本</p>
                      <p className="text-xs text-gray-500 mt-1">纯文本格式</p>
                    </div>
                  </div>
                </label>

                <label className="relative cursor-pointer">
                  <input
                    type="radio"
                    name="format"
                    checked={format === 'json'}
                    onChange={() => setFormat('json')}
                    className="sr-only"
                  />
                  <div className={`border-2 rounded-lg p-4 transition-colors ${
                    format === 'json'
                      ? 'border-blue-500 bg-blue-50'
                      : 'border-gray-200 hover:border-gray-300'
                  }`}>
                    <div className="text-center">
                      <span className="text-2xl">📋</span>
                      <p className="text-sm font-medium mt-2">JSON</p>
                      <p className="text-xs text-gray-500 mt-1">结构化数据</p>
                    </div>
                  </div>
                </label>
              </div>
            </div>

            {/* 操作按钮 */}
            <div className="flex space-x-3">
              <button
                onClick={onClose}
                className="flex-1 px-4 py-2 text-gray-600 hover:bg-gray-100 rounded-lg transition-colors"
              >
                取消
              </button>
              <button
                onClick={handleExport}
                disabled={exporting}
                className="flex-1 bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white font-medium py-2 px-4 rounded-lg transition-colors flex items-center justify-center"
              >
                {exporting ? (
                  <>
                    <svg className="animate-spin -ml-1 mr-2 h-4 w-4 text-white" fill="none" viewBox="0 0 24 24">
                      <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                      <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                    </svg>
                    导出中...
                  </>
                ) : (
                  '导出'
                )}
              </button>
            </div>
          </>
        )}

        {/* 关闭按钮 */}
        {!exported && (
          <div className="mt-4 flex justify-end">
            <button
              onClick={onClose}
              className="px-4 py-2 text-gray-600 hover:bg-gray-100 rounded-lg transition-colors"
            >
              关闭
            </button>
          </div>
        )}
      </div>
    </div>
  )
}
