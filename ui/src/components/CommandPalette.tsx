import { useEffect, useMemo, useState } from 'react'
import { applyOps, shareLink, mountBytesAndLoad, mountFileAndLoad } from '../sdk/viewer'

type Command = { id: string, title: string, run: ()=>void }

export function CommandPalette({ onLog }: { onLog: (s: string)=>void }) {
  const [open, setOpen] = useState(false)
  const [q, setQ] = useState('')

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') { e.preventDefault(); setOpen(v=>!v); setQ('') }
      if (e.key === 'Escape') setOpen(false)
    }
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  }, [])

  const cmds: Command[] = useMemo(() => ([
    { id: 'open', title: 'Import Model…', run: async ()=>{
      // Try Tauri dialog first, fallback to HTML file input
      const anyWin = window as any
      if (anyWin.__TAURI__) {
        try {
          const { open } = await import(/* @vite-ignore */ '@tauri-apps/api/dialog')
          const { readBinaryFile } = await import(/* @vite-ignore */ '@tauri-apps/api/fs')
          const selected = await open({ multiple: false, filters: [
            { name: 'Models', extensions: ['obj','ply','glb','gltf','fbx','dae'] }
          ]})
          if (!selected || Array.isArray(selected)) return
          const path = selected as string
          const parts = path.replace(/\\/g,'/').split('/')
          const name = parts[parts.length-1]
          const bytes = await readBinaryFile(path)
          const ok = mountBytesAndLoad(name, new Uint8Array(bytes))
          onLog(ok ? `Loaded ${name}` : `Load failed: ${name}`)
          return
        } catch (e: any) {
          onLog('Open failed: ' + (e?.message || e))
          return
        }
      }
      // Fallback to HTML file input for web
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
    }},
    { id: 'open_json', title: 'Load Scene JSON…', run: ()=>{
      const input = document.createElement('input')
      input.type = 'file'
      input.accept = '.json'
      input.onchange = (e) => {
        const file = (e.target as HTMLInputElement).files?.[0]
        if (file) {
          const reader = new FileReader()
          reader.onload = () => {
            try {
              const json = reader.result as string
              const ops = JSON.parse(json)
              const ok = applyOps(ops)
              onLog(ok ? `Loaded scene from ${file.name}` : `Failed to load scene from ${file.name}`)
            } catch (e: any) {
              onLog(`JSON parse error: ${e?.message || e}`)
            }
          }
          reader.readAsText(file)
        }
      }
      input.click()
    }},
    { id: 'add_light', title: 'Add Light', run: ()=>{ onLog(applyOps({op:'add_light', type:'point', intensity:1.0})? 'Added light' : 'Add light failed') } },
    { id: 'share', title: 'Copy Share Link', run: ()=>{ const link=shareLink(); navigator.clipboard.writeText(link); onLog('Share link copied') } },
  ]), [])

  const filtered = useMemo(() => cmds.filter(c => c.title.toLowerCase().includes(q.toLowerCase())), [q, cmds])

  if (!open) return null
  return (
    <div className="fixed inset-0 bg-black/40 flex items-start justify-center pt-32 z-50" onClick={()=>setOpen(false)}>
      <div className="bg-neutral-900/90 backdrop-blur border border-neutral-700/60 shadow-2xl rounded-xl w-[640px] overflow-hidden" onClick={e=>e.stopPropagation()}>
        <input autoFocus value={q} onChange={e=>setQ(e.target.value)} className="w-full bg-neutral-800/70 px-4 py-3 border-b border-neutral-700/60 outline-none text-neutral-100" placeholder="Type a command... (Ctrl+K)" />
        <div className="max-h-72 overflow-auto">
          {filtered.map(c => (
            <button key={c.id} onClick={()=>{ c.run(); setOpen(false) }} className="block w-full text-left px-4 py-2 hover:bg-neutral-800/70">
              {c.title}
            </button>
          ))}
          {filtered.length===0 && <div className="px-3 py-2 text-neutral-500">No matches</div>}
        </div>
        <div className="px-4 py-2 text-xs text-neutral-500 flex justify-between">
          <span>Press Esc to close</span>
          <span>↑↓ to navigate • Enter to run</span>
        </div>
      </div>
    </div>
  )
}
