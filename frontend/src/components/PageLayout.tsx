import { ReactNode } from 'react'

interface PageLayoutProps {
  children: ReactNode
  title?: string
}

export default function PageLayout({ children, title }: PageLayoutProps) {
  return (
    <div className="theme-page">
      {title && (
        <div className="theme-surface theme-divider">
          <div className="max-w-7xl mx-auto px-4 py-4">
            <h1 className="text-2xl font-bold text-inherit">{title}</h1>
          </div>
        </div>
      )}
      <div className="max-w-7xl mx-auto px-4 py-6">
        {children}
      </div>
    </div>
  )
}
