import { useRef } from 'react'
import { applyOps, shareLink } from '../sdk/viewer'

export function Console({ logs, onLog }: { logs: string[], onLog: (s: string)=>void }) {
  const input = useRef<HTMLInputElement>(null)

  function submit(e: React.FormEvent) {
    e.preventDefault()
    const v = input.current?.value?.trim()
    if (!v) return
    input.current!.value = ''
    // Try to parse as JSON ops; fallback to echo
    try {
      const parsed = JSON.parse(v)
      const ok = applyOps(parsed)
      onLog(ok ? 'Applied ops.' : 'Ops failed.')
    } catch {
      onLog('Echo: ' + v)
    }
  }

  return (
    <div className="h-full flex flex-col">
      <div className="flex-1 overflow-auto p-2 text-sm">
        {logs.map((l, i) => (<div key={i} className="text-neutral-300">{l}</div>))}
      </div>
      <form onSubmit={submit} className="p-2 border-t border-neutral-800 flex gap-2">
        <input ref={input} className="flex-1 bg-neutral-800 rounded px-2 py-1 outline-none" placeholder='Enter JSON Ops (e.g., {"op":"add_light"})' />
        <button type="button" className="bg-neutral-700 rounded px-2" onClick={()=>{ onLog('Share: ' + shareLink())}}>Share</button>
        <button type="submit" className="bg-blue-600 rounded px-3">Run</button>
      </form>
    </div>
  )
}

