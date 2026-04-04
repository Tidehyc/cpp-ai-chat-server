import { useFriendStore } from '../stores/friendStore'
import type { Friend } from '../types/friend'

interface ChatListProps {
  selectedFriend?: string | null
  onSelectFriend?: (username: string) => void
}

export default function ChatList({ selectedFriend, onSelectFriend }: ChatListProps) {
  const { friends, getFilteredFriends } = useFriendStore()
  const filteredFriends = getFilteredFriends()

  const getStatusIcon = (status: Friend['status']) => {
    switch (status) {
      case 'online': return '🟢'
      case 'busy': return '🔴'
      case 'away': return '🟡'
      case 'offline': return '⚪'
    }
  }

  if (filteredFriends.length === 0) {
    return (
      <div className="p-4 text-center text-gray-500 text-sm">
        {friends.length === 0 ? '暂无好友，点击 ➕ 添加' : '没有找到匹配的好友'}
      </div>
    )
  }

  return (
    <div className="divide-y divide-gray-100">
      {filteredFriends.map((friend) => (
        <button
          key={friend.id}
          onClick={() => onSelectFriend?.(friend.username)}
          className={`w-full flex items-center px-4 py-3 hover:bg-gray-50 transition-colors ${
            selectedFriend === friend.username ? 'bg-blue-50 border-r-4 border-blue-500' : ''
          }`}
        >
          <span className="mr-3">{getStatusIcon(friend.status)}</span>
          <div className="flex-1 text-left">
            <p className="text-sm font-medium text-gray-900">{friend.username}</p>
            <p className="text-xs text-gray-500">
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
