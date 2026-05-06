import ReactMarkdown from 'react-markdown'
import remarkBreaks from 'remark-breaks'
import remarkGfm from 'remark-gfm'
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter'
import { oneLight } from 'react-syntax-highlighter/dist/esm/styles/prism'

interface MarkdownMessageProps {
  content: string
}

export default function MarkdownMessage({ content }: MarkdownMessageProps) {
  return (
    <div className="text-sm whitespace-pre-wrap break-words">
      <ReactMarkdown
        remarkPlugins={[remarkGfm, remarkBreaks]}
        components={{
          p: ({ node, ...props }) => <p className="my-1" {...props} />,
          ul: ({ node, ...props }) => <ul className="list-disc list-inside my-2 space-y-1" {...props} />,
          ol: ({ node, ...props }) => <ol className="list-decimal list-inside my-2 space-y-1" {...props} />,
          li: ({ node, ...props }) => <li className="ml-2" {...props} />,
          code: ({ inline, className, children, ...props }: any) => {
            const match = /language-(\w+)/.exec(className || '')

            if (!inline) {
              return (
                <SyntaxHighlighter
                  style={oneLight}
                  language={match ? match[1] : 'text'}
                  PreTag="div"
                  customStyle={{
                    margin: '0.5rem 0',
                    borderRadius: '0.5rem',
                    fontSize: '0.75rem',
                  }}
                >
                  {String(children).replace(/\n$/, '')}
                </SyntaxHighlighter>
              )
            }

            return (
              <code className="rounded theme-surface px-1 py-0.5 text-xs font-mono" {...props}>
                {children}
              </code>
            )
          },
          strong: ({ node, ...props }) => <strong className="font-semibold" {...props} />,
          a: ({ node, ...props }) => (
            <a
              className="text-cyan-700 underline underline-offset-2 hover:text-cyan-600"
              target="_blank"
              rel="noreferrer"
              {...props}
            />
          ),
        }}
      >
        {content}
      </ReactMarkdown>
    </div>
  )
}