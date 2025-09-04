import { openModelViaTauri, mountFileAndLoad } from '../sdk/viewer'

export function TopBar({ onShare, onLog, theme, setTheme }: { onShare: ()=>void, onLog: (s:string)=>void, theme: 'dark'|'light', setTheme: (t:'dark'|'light')=>void }) {
  const isTauri = typeof (window as any).__TAURI__ !== 'undefined'
  
  function handleImport() {
    if (isTauri) {
      openModelViaTauri(onLog)
    } else {
      // Web fallback
      const input = document.createElement('input')
      input.type = 'file'
      input.accept = '.obj,.ply,.glb,.gltf,.fbx,.dae'
      input.onchange = (e) => {
        const file = (e.target as HTMLInputElement).files?.[0]
        if (file) {
          mountFileAndLoad(file, onLog)
        }
      }
      input.click()
    }
  }
  
  return (
    <div className="h-12 flex items-center justify-between px-4 border-b border-neutral-800/70 bg-neutral-900/70 backdrop-blur">
      <div className="flex items-center gap-3">
        <div className="font-semibold tracking-wide">Glint3D</div>
        <button className="bg-blue-600 hover:bg-blue-500 rounded px-3 py-1 text-sm" onClick={handleImport}>
          Import {isTauri ? '⌘O' : ''}
        </button>
      </div>
      <div className="flex items-center gap-2">
        <button className="bg-neutral-700 hover:bg-neutral-600 rounded px-2 py-1 text-sm" onClick={onShare}>Share</button>
        <button className="bg-neutral-700 hover:bg-neutral-600 rounded px-2 py-1 text-sm" onClick={()=>setTheme(theme==='dark'?'light':'dark')}>{theme==='dark'?'Light':'Dark'} Theme</button>
      </div>
    </div>
  )
}

