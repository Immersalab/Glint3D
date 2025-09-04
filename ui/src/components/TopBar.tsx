import { openModelViaTauri } from '../sdk/viewer'

export function TopBar({ onShare, onLog, theme, setTheme }: { onShare: ()=>void, onLog: (s:string)=>void, theme: 'dark'|'light', setTheme: (t:'dark'|'light')=>void }) {
  const isTauri = typeof (window as any).__TAURI__ !== 'undefined'
  return (
    <div className="h-12 flex items-center justify-between px-4 border-b border-neutral-800/70 bg-neutral-900/70 backdrop-blur">
      <div className="flex items-center gap-3">
        <div className="font-semibold tracking-wide">Glint3D</div>
        {isTauri && (
          <button className="bg-neutral-700 hover:bg-neutral-600 rounded px-2 py-1 text-sm" onClick={()=>openModelViaTauri(onLog)}>Open Model… ⌘O</button>
        )}
      </div>
      <div className="flex items-center gap-2">
        <button className="bg-neutral-700 hover:bg-neutral-600 rounded px-2 py-1 text-sm" onClick={onShare}>Share</button>
        <button className="bg-neutral-700 hover:bg-neutral-600 rounded px-2 py-1 text-sm" onClick={()=>setTheme(theme==='dark'?'light':'dark')}>{theme==='dark'?'Light':'Dark'} Theme</button>
      </div>
    </div>
  )
}

