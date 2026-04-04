import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import { useUserStore } from './stores/userStore'
import ThemeProvider from './components/ThemeProvider'
import Login from './pages/Login'
import Register from './pages/Register'
import Chat from './pages/Chat'
import Contact from './pages/Contact'
import AIAssistant from './pages/AIAssistant'
import Settings from './pages/Settings'

function ProtectedRoute({ children }: { children: React.ReactNode }) {
  const { isLoggedIn, username } = useUserStore()
  
  // 检查是否登录（从 localStorage 恢复的数据也算）
  if (!isLoggedIn && !username) {
    return <Navigate to="/login" />
  }

  return <>{children}</>
}

function App() {
  return (
    <ThemeProvider>
      <BrowserRouter>
        <Routes>
          <Route path="/login" element={<Login />} />
          <Route path="/register" element={<Register />} />
          <Route
            path="/"
            element={
              <ProtectedRoute>
                <Chat />
              </ProtectedRoute>
            }
          />
          <Route
            path="/contact"
            element={
              <ProtectedRoute>
                <Contact />
              </ProtectedRoute>
            }
          />
          <Route
            path="/ai"
            element={
              <ProtectedRoute>
                <AIAssistant />
              </ProtectedRoute>
            }
          />
          <Route
            path="/settings"
            element={
              <ProtectedRoute>
                <Settings />
              </ProtectedRoute>
            }
          />
        </Routes>
      </BrowserRouter>
    </ThemeProvider>
  )
}

export default App
