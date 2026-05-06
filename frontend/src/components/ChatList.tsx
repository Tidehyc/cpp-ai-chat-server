import { useFriendStore } from '../stores/friendStore'
import type { Friend } from '../types/friend'

interface ChatListProps {
  selectedFriend?: string | null
  onSelectFriend?: (username: string) => void
}

export default function ChatList({ selectedFriend, onSelectFriend }: ChatListProps) {
  const { friends, getFilteredFriends } = useFriendStore()
  const filteredFriends = getFilteredFriends()

  const getStatusClass = (status: Friend['status']) => {
    switch (status) {
      case 'online': return 'status-dot status-dot-online'
      case 'busy': return 'status-dot status-dot-busy'
      case 'away': return 'status-dot status-dot-away'
      case 'offline': return 'status-dot status-dot-offline'
      default: return 'status-dot status-dot-offline'
    }
  }

  if (filteredFriends.length === 0) {
    return (
      <div className="p-4 text-center theme-muted text-sm">
        {friends.length === 0 ? '暂无好友，点击右上角添加' : '没有找到匹配的好友'}
      </div>
    )
  }

  return (
    <div className="divide-y divide-slate-200/70">
      {filteredFriends.map((friend) => (
        <button
          key={friend.id}
          onClick={() => onSelectFriend?.(friend.username)}
          className={`w-full flex items-center px-4 py-3 hover:bg-white/60 transition-colors ${
            selectedFriend === friend.username ? 'theme-nav-item is-active border-r-4 border-cyan-500' : ''
          }`}
        >
          <span className={`mr-3 ${getStatusClass(friend.status)}`} />
          <div className="flex-1 text-left">
            <p className="text-sm font-medium text-inherit">{friend.username}</p>
            <p className="text-xs theme-muted">
              {friend.status === 'online' ? '在线' : 
               friend.status === 'busy' ? '忙碌' : 
               friend.status === 'away' ? '离开' : '离线'}
            </p>
          </div>
        </button>
      ))}
    </div>
  )
}
