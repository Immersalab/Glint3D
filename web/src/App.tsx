import { useEffect, useState } from 'react'
import { CanvasHost } from './components/CanvasHost'
import { Console } from './components/Console'
import { Inspector } from './components/Inspector'
import { CommandPalette } from './components/CommandPalette'
import { TopBar } from './components/TopBar'
import { shareLink, openModelViaTauri } from './sdk/viewer'

export default function App() {
  const [logs, setLogs] = useState<string[]>([
    'Welcome — ask me anything in the Console below.',
  ])
  const pushLog = (s: string) => setLogs(l => [...l, s])

  const [theme, setTheme] = useState<'dark'|'light'>(() => (localStorage.getItem('theme') as any) || 'dark')
  useEffect(() => {
    localStorage.setItem('theme', theme)
    document.body.classList.toggle('theme-light', theme === 'light')
  }, [theme])
  useEffect(() => {
    const anyWin = window as any
    if (!anyWin.__TAURI__) return
    import(/* @vite-ignore */ '@tauri-apps/api/event').then(({ listen }) => {
      listen('open-model', () => openModelViaTauri(pushLog))
    })
  }, [])

  return (
    <div className="h-full flex flex-col">
      <TopBar onShare={()=>{ const link=shareLink(); navigator.clipboard.writeText(link); pushLog('Share link copied') }} onLog={pushLog} theme={theme} setTheme={setTheme} />
      <div className="flex-1 app-grid">
        <div className="area-canvas panel">
          <CanvasHost onLog={pushLog} />
        </div>
        <div className="area-console panel">
          <Console logs={logs} onLog={pushLog} />
        </div>
        <div className="area-inspector panel">
          <Inspector onLog={pushLog} />
        </div>
        <CommandPalette onLog={pushLog} />
      </div>
    </div>
  )
}

