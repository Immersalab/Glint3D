import { useEffect, useMemo, useState } from 'react'
import { applyOps, getScene, SceneSnapshot, mountFileAndLoad } from '../sdk/viewer'

export function Inspector({ onLog }: { onLog: (s: string)=>void }) {
  const [scene, setScene] = useState<SceneSnapshot | null>(null)
  const [outputPath, setOutputPath] = useState<string>(() => localStorage.getItem('outputPath') || '')
  
  useEffect(() => {
    localStorage.setItem('outputPath', outputPath)
  }, [outputPath])

  function refresh() {
    const s = getScene()
    setScene(s)
  }

  useEffect(() => {
    const id = setInterval(refresh, 1000)
    return () => clearInterval(id)
  }, [])

  function addLight() {
    const ok = applyOps({ op: 'add_light', type: 'point', intensity: 1.0 })
    onLog(ok ? 'Added light' : 'Add light failed')
  }

  function handleImportModel() {
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

  function handleImportJSON() {
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
  }

  function renderImage() {
    if (!outputPath.trim()) {
      onLog('Please set an output path first')
      return
    }
    const ok = applyOps({ 
      op: 'render_image', 
      path: outputPath,
      width: 800,
      height: 600
    })
    onLog(ok ? `Rendering to ${outputPath}` : 'Render failed')
  }

  function exportScene() {
    const scene = getScene()
    if (!scene) {
      onLog('No scene to export')
      return
    }
    
    const dataStr = JSON.stringify(scene, null, 2)
    const dataBlob = new Blob([dataStr], { type: 'application/json' })
    const url = URL.createObjectURL(dataBlob)
    const link = document.createElement('a')
    link.href = url
    link.download = 'scene.json'
    link.click()
    URL.revokeObjectURL(url)
    onLog('Scene exported as scene.json')
  }

  const selectedName = scene?.selected ?? null
  const selectedLight = (scene && scene.selectedLightIndex != null && scene.selectedLightIndex >= 0) ? scene.selectedLightIndex : -1

  function selectObject(name: string) {
    const ok = applyOps({ op: 'select', type: 'object', target: name })
    if (!ok) onLog('Select object failed')
  }
  function selectLight(i: number) {
    const ok = applyOps({ op: 'select', type: 'light', index: i })
    if (!ok) onLog('Select light failed')
  }
  function removeSelected() {
    let ok = false
    if (selectedName) ok = applyOps({ op: 'remove', type: 'object', target: selectedName })
    else if (selectedLight >= 0) ok = applyOps({ op: 'remove', type: 'light', index: selectedLight })
    onLog(ok ? 'Removed selected' : 'Remove failed')
  }
  function duplicateSelected() {
    let ok = false
    if (selectedName) ok = applyOps({ op: 'duplicate', source: selectedName })
    else if (selectedLight >= 0) ok = applyOps({ op: 'duplicate_light', index: selectedLight })
    onLog(ok ? 'Duplicated selected' : 'Duplicate failed')
  }
  function nudge(dx:number,dy:number,dz:number) {
    if (!selectedName) return onLog('Select an object to transform')
    const ok = applyOps({ op: 'transform', target: selectedName, mode: 'delta', transform_delta: { position: [dx,dy,dz] } })
    onLog(ok ? 'Moved object' : 'Move failed')
  }

  return (
    <div className="h-full p-3 text-sm flex flex-col gap-3">
      <div className="flex items-center justify-between">
        <div className="font-semibold">Inspector</div>
        <button className="bg-neutral-700 rounded px-2 py-1" onClick={refresh}>Refresh</button>
      </div>
      <div className="text-neutral-300">
        {scene ? (
          <>
            <div>Objects: {scene.objects?.length ?? 0}</div>
            <div>Lights: {scene.lights?.length ?? 0}</div>
            <div className="mt-2">
              <div className="font-medium">Object Names</div>
              <ul className="list-disc ml-5 max-h-28 overflow-auto">
                {scene.objects?.map((o, i) => (
                  <li key={i}>
                    <button className={`hover:underline ${selectedName===o.name? 'text-blue-400':''}`} onClick={()=>selectObject(o.name)}>{o.name}</button>
                  </li>
                ))}
              </ul>
            </div>
            <div className="mt-2">
              <div className="font-medium">Lights</div>
              <ul className="list-disc ml-5 max-h-28 overflow-auto">
                {scene.lights?.map((L, i) => (
                  <li key={i}>
                    <button className={`hover:underline ${selectedLight===i? 'text-blue-400':''}`} onClick={()=>selectLight(i)}>Light {i}</button>
                    <span className="text-neutral-500"> &nbsp;I={L.intensity.toFixed?.(2) ?? L.intensity}</span>
                  </li>
                ))}
              </ul>
            </div>
          </>
        ) : (
          <div className="text-neutral-500">No scene yet.</div>
        )}
      </div>
      <div className="mt-auto flex flex-col gap-3">
        <div className="flex flex-col gap-2">
          <div className="font-medium">Import/Export</div>
          <div className="flex gap-2 flex-wrap">
            <button className="bg-blue-600 rounded px-3 py-1" onClick={handleImportModel}>Import Model</button>
            <button className="bg-green-600 rounded px-3 py-1" onClick={handleImportJSON}>Load Scene JSON</button>
            <button className="bg-orange-600 rounded px-3 py-1" onClick={exportScene}>Export Scene</button>
          </div>
        </div>

        <div className="flex flex-col gap-2">
          <div className="font-medium">Settings</div>
          <div className="flex flex-col gap-2">
            <label className="text-xs text-neutral-400">Output Path</label>
            <input 
              type="text" 
              value={outputPath} 
              onChange={(e) => setOutputPath(e.target.value)}
              placeholder="/path/to/output.png"
              className="bg-neutral-800 rounded px-2 py-1 text-sm outline-none border border-transparent focus:border-blue-500"
            />
          </div>
          <button className="bg-purple-600 rounded px-3 py-1 mt-1" onClick={renderImage}>Render Image</button>
        </div>

        <div className="flex flex-col gap-2">
          <div className="font-medium">Actions</div>
          <div className="flex gap-2 flex-wrap">
            <button className="bg-neutral-700 rounded px-2 py-1" onClick={addLight}>Add Light</button>
            <button className="bg-red-700 rounded px-2 py-1" onClick={removeSelected}>Delete Selected</button>
            <button className="bg-neutral-700 rounded px-2 py-1" onClick={duplicateSelected}>Duplicate Selected</button>
          </div>
          <div className="mt-2 flex gap-2 flex-wrap">
            <div className="text-neutral-400">Nudge:</div>
            <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge( 0.1, 0, 0)}>+X</button>
            <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(-0.1, 0, 0)}>-X</button>
            <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(0, 0.1, 0)}>+Y</button>
            <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(0,-0.1, 0)}>-Y</button>
            <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(0, 0, 0.1)}>+Z</button>
            <button className="bg-neutral-700 rounded px-2 py-1" onClick={()=>nudge(0, 0,-0.1)}>-Z</button>
          </div>
        </div>
      </div>
    </div>
  )
}
