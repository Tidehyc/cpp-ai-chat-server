import { useState } from 'react'
import { Search, X } from 'lucide-react'
import { wsService } from '../services/websocket'
import type { Message } from '../types'

interface SearchModalProps {
  isOpen: boolean
  onClose: () => void
}

export default function SearchModal({ isOpen, onClose }: SearchModalProps) {
  const [keyword, setKeyword] = useState('')
  const [searchType, setSearchType] = useState<'all' | 'from' | 'room'>('all')
  const [searchValue, setSearchValue] = useState('')
  const [results, setResults] = useState<Message[]>([])
  const [loading, setLoading] = useState(false)
  const [searched, setSearched] = useState(false)

  // 执行搜索
  const handleSearch = () => {
    if (!keyword.trim() && !searchValue.trim()) return

    setLoading(true)
    setSearched(true)

    let command = '/search '
    
    if (searchType === 'from' && searchValue) {
      command += `from:${searchValue} `
    } else if (searchType === 'room' && searchValue) {
      command += `in:${searchValue} `
    }
    
    command += keyword

    wsService.sendCommand(command)

    // 监听搜索结果
    const handleSearchResult = (message: any) => {
      const content = message.content || ''
      
      if (content.includes('搜索结果') || content.includes('search results')) {
        // 解析搜索结果
        const lines = content.split('\n').filter((line: string) => line.trim())
        const parsedMessages: Message[] = []
        
        lines.forEach((line: string, index: number) => {
          if (index === 0) return // 跳过标题行
          
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
        })
        
        setResults(parsedMessages)
        setLoading(false)
      }
    }

    wsService.on('system', handleSearchResult)

    setTimeout(() => {
      setLoading(false)
    }, 2000)

    return () => {
      wsService.off('system', handleSearchResult)
    }
  }

  // 清除搜索
  const handleClear = () => {
    setKeyword('')
    setSearchValue('')
    setResults([])
    setSearched(false)
  }

  if (!isOpen) return null

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-xl w-full max-w-2xl max-h-[80vh] flex flex-col">
        {/* 标题 */}
        <div className="h-14 border-b border-gray-200 flex items-center justify-between px-6">
          <h3 className="text-lg font-semibold icon-label"><Search size={17} />搜索消息</h3>
          <button
            onClick={onClose}
            className="p-2 hover:bg-gray-100 rounded-lg transition-colors"
          >
            <X size={16} />
          </button>
        </div>

        {/* 搜索表单 */}
        <div className="p-6 border-b border-gray-200">
          <div className="space-y-4">
            {/* 搜索类型选择 */}
            <div className="flex space-x-4">
              <label className="flex items-center">
                <input
                  type="radio"
                  checked={searchType === 'all'}
                  onChange={() => setSearchType('all')}
                  className="mr-2"
                />
                <span className="text-sm text-gray-700">全部</span>
              </label>
              <label className="flex items-center">
                <input
                  type="radio"
                  checked={searchType === 'from'}
                  onChange={() => setSearchType('from')}
                  className="mr-2"
                />
                <span className="text-sm text-gray-700">按用户</span>
              </label>
              <label className="flex items-center">
                <input
                  type="radio"
                  checked={searchType === 'room'}
                  onChange={() => setSearchType('room')}
                  className="mr-2"
                />
                <span className="text-sm text-gray-700">按房间</span>
              </label>
            </div>

            {/* 搜索值输入 */}
            {(searchType === 'from' || searchType === 'room') && (
              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">
                  {searchType === 'from' ? '用户名' : '房间名'}
                </label>
                <input
                  type="text"
                  value={searchValue}
                  onChange={(e) => setSearchValue(e.target.value)}
                  placeholder={searchType === 'from' ? '用户名' : '#房间名'}
                  className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                />
              </div>
            )}

            {/* 关键词输入 */}
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1">
                搜索关键词
              </label>
              <input
                type="text"
                value={keyword}
                onChange={(e) => setKeyword(e.target.value)}
                placeholder="输入要搜索的内容..."
                className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                autoFocus
              />
            </div>

            {/* 操作按钮 */}
            <div className="flex space-x-3 pt-2">
              <button
                onClick={handleSearch}
                disabled={loading || (!keyword.trim() && !searchValue.trim())}
                className="flex-1 bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white font-medium py-2 px-4 rounded-lg transition-colors"
              >
                {loading ? '搜索中...' : '搜索'}
              </button>
              <button
                onClick={handleClear}
                className="px-4 py-2 text-gray-600 hover:bg-gray-100 rounded-lg transition-colors"
              >
                清除
              </button>
            </div>
          </div>
        </div>

        {/* 搜索结果 */}
        <div className="flex-1 overflow-y-auto p-6">
          {!searched ? (
            <div className="text-center py-12 text-gray-400">
              <Search size={32} className="mx-auto mb-4 text-slate-300" />
              <p>输入关键词开始搜索</p>
            </div>
          ) : loading ? (
            <div className="flex items-center justify-center py-12">
              <svg className="animate-spin h-8 w-8 text-blue-600" fill="none" viewBox="0 0 24 24">
                <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
              </svg>
              <span className="ml-3 text-gray-600">搜索中...</span>
            </div>
          ) : results.length > 0 ? (
            <div className="space-y-4">
              <p className="text-sm text-gray-600">找到 {results.length} 条相关消息</p>
              {results.map((msg, index) => (
                <div key={index} className="bg-gray-50 rounded-lg p-4">
                  <div className="flex items-center justify-between mb-2">
                    <span className="text-sm font-medium text-blue-600">{msg.from_user}</span>
                    <span className="text-xs text-gray-500">{msg.timestamp}</span>
                  </div>
                  <p className="text-sm text-gray-900">{msg.content}</p>
                </div>
              ))}
            </div>
          ) : (
            <div className="text-center py-12 text-gray-400">
              <Search size={32} className="mx-auto mb-4 text-slate-300" />
              <p>没有找到相关消息</p>
            </div>
          )}
        </div>

        {/* 底部按钮 */}
        <div className="h-14 border-t border-gray-200 flex items-center justify-end px-6">
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
