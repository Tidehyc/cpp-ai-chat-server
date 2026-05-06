import { useCallback, useRef, useState } from 'react'

const BOTTOM_THRESHOLD = 120

export function useSmartScroll() {
  const containerRef = useRef<HTMLDivElement>(null)
  const endRef = useRef<HTMLDivElement>(null)
  const [showScrollButton, setShowScrollButton] = useState(false)

  const isNearBottom = useCallback(() => {
    const container = containerRef.current
    if (!container) return true

    const distanceToBottom = container.scrollHeight - container.scrollTop - container.clientHeight
    return distanceToBottom <= BOTTOM_THRESHOLD
  }, [])

  const scrollToBottom = useCallback((behavior: ScrollBehavior = 'smooth') => {
    endRef.current?.scrollIntoView({ behavior })
    setShowScrollButton(false)
  }, [])

  const handleScroll = useCallback(() => {
    setShowScrollButton(!isNearBottom())
  }, [isNearBottom])

  const handleContentChange = useCallback(() => {
    if (isNearBottom()) {
      scrollToBottom('smooth')
      return
    }

    setShowScrollButton(true)
  }, [isNearBottom, scrollToBottom])

  return {
    containerRef,
    endRef,
    showScrollButton,
    handleScroll,
    scrollToBottom,
    handleContentChange,
  }
}
