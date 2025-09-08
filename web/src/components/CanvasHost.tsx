import { useEffect, useRef } from 'react'
import { init, mountFileAndLoad } from '../sdk/viewer'

export function CanvasHost({ onLog }: { onLog: (s: string)=>void }) {
  const ref = useRef<HTMLCanvasElement>(null)

  useEffect(() => {
    if (!ref.current) return
    init(ref.current).then(()=>{
      onLog('Engine initialized')
    }).catch(err => onLog('Engine init error: ' + err.message))
  }, [])

  function onDrop(e: React.DragEvent) {
    e.preventDefault()
    const f = e.dataTransfer?.files?.[0]
    if (f) mountFileAndLoad(f, onLog)
  }
  function onDragOver(e: React.DragEvent) { e.preventDefault() }

  return (
    <div className="w-full h-full relative" onDrop={onDrop} onDragOver={onDragOver}>
      <canvas ref={ref} className="w-full h-full block outline-none" tabIndex={0}></canvas>
      <div className="absolute top-2 left-2 text-xs text-neutral-300 bg-neutral-800/60 rounded px-2 py-1">
        Drop .obj/.ply/.glb here or use Console to run ops
      </div>
    </div>
  )
}

