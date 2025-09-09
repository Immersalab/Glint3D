import { useEffect, useRef } from 'react'
import { init, mountFileAndLoad, applyOps } from '../sdk/viewer'

export function CanvasHost({ onLog }: { onLog: (s: string)=>void }) {
  const ref = useRef<HTMLCanvasElement>(null)

  useEffect(() => {
    if (!ref.current) return
    const canvas = ref.current

    // Ensure canvas drawing buffer tracks CSS size (helps GLFW/WebGL)
    const ro = new ResizeObserver(() => {
      const dpr = window.devicePixelRatio || 1
      const r = canvas.getBoundingClientRect()
      const w = Math.max(1, Math.round(r.width * dpr))
      const h = Math.max(1, Math.round(r.height * dpr))
      if (canvas.width !== w || canvas.height !== h) {
        canvas.width = w
        canvas.height = h
      }
    })
    ro.observe(canvas)

    init(canvas).then(()=>{
      onLog('Engine initialized')
      // Make something visible immediately: gradient background + ensure cube is loaded
      // Using absolute path to avoid cwd ambiguity
      const ok = applyOps([
        { op: 'set_background', top: [0.10, 0.10, 0.12], bottom: [0.0, 0.0, 0.0] },
        { op: 'load', path: '/assets/models/cube.obj', name: 'Cube', transform: { position: [0, 0, -4], scale: [1, 1, 1] } },
        { op: 'add_light', type: 'point', position: [2, 4, 2], color: [0.8, 0.8, 0.7], intensity: 1.0 },
      ])
      if (ok) onLog('Default scene applied')
    }).catch(err => onLog('Engine init error: ' + err.message))

    return () => { try { ro.disconnect() } catch {} }
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

