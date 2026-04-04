import { useEffect, useCallback } from 'react'
import { wsService } from '../services/websocket'
import { useUserStore } from '../stores/userStore'
import { useMessageStore } from '../stores/messageStore'
import { useRoomStore } from '../stores/roomStore'

export function useWebSocket() {
  const login = useUserStore((state) => state.login)
  const logout = useUserStore((state) => state.logout)
  const addMessage = useMessageStore((state) => state.addMessage)
  const setOnlineUsers = useRoomStore((state) => state.setOnlineUsers)

  const handleMessage = useCallback((message: any) => {
    console.log('[Hook] Received message:', message)

    switch (message.type) {
      case 'system':
        // 系统消息
        const content = message.content

        // 检测登录成功
        if (content.includes('登录成功')) {
          const match = content.match(/欢迎 (\S+)/)
          if (match) {
            login(match[1], Date.now())
          }
        }

        // 检测注册成功
        if (content.includes('注册成功')) {
          console.log('注册成功，请登录')
        }

        break

      case 'online_users':
        // 在线用户列表
        if (message.users) {
          console.log('[Hook] Online users:', message.users)
          setOnlineUsers(message.users)
        }
        break

      case 'sent_message':
      case 'room_message':
        // 聊天消息
        addMessage('#general', {
          id: Date.now(),
          type: message.type === 'ai_response' ? 'ai' : 'room',
          room_name: '#general',
          from_user: message.from_user,
          content: message.content,
          timestamp: message.timestamp,
          is_ai: false
        })
        break

      case 'ai_response':
        // AI 回复
        addMessage('#general', {
          id: Date.now(),
          type: 'ai',
          room_name: '#general',
          from_user: '🤖 AI',
          content: message.content,
          timestamp: message.timestamp,
          is_ai: true
        })
        break

      default:
        // 其他消息也显示
        console.log('Unknown message type:', message.type, message)
    }
  }, [login, logout, addMessage, setOnlineUsers])

  useEffect(() => {
    wsService.on('*', handleMessage)
    
    return () => {
      wsService.off('*', handleMessage)
    }
  }, [handleMessage])

  /**
   * 发送命令
   */
  const sendCommand = useCallback((command: string) => {
    const ws = (window as any).chatWS
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(command)
      console.log('[WS] 📤 Sent:', command)
    } else {
      console.warn('[WS] Not connected, command queued:', command)
      wsService.sendCommand(command)
    }
  }, [])

  /**
   * 发送聊天消息
   */
  const sendText = useCallback((text: string) => {
    wsService.sendText(text)
  }, [])

  const connect = useCallback(async () => {
    await wsService.connect()
  }, [])

  const disconnect = useCallback(() => {
    wsService.disconnect()
  }, [])

  return { 
    sendCommand, 
    sendText, 
    connect, 
    disconnect, 
    isConnected: wsService.isConnected() 
  }
}
