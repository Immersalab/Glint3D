// Minimal viewer SDK wrapping Emscripten Module calls

declare global { interface Window { Module: any } }

let engineScriptLoading: Promise<void> | null = null

export function loadEngine(scriptPath = '/engine/glint3d.js'): Promise<void> {
  if (engineScriptLoading) return engineScriptLoading
  engineScriptLoading = new Promise<void>((resolve, reject) => {
    const s = document.createElement('script')
    s.src = scriptPath
    s.async = true
    s.onload = () => resolve()
    s.onerror = () => reject(new Error('Failed to load engine script'))
    document.head.appendChild(s)
  })
  return engineScriptLoading
}

function waitForModuleReady(timeoutMs = 15000): Promise<void> {
  const start = Date.now()
  return new Promise((resolve, reject) => {
    const tick = () => {
      const M = (window as any).Module
      if (M && (M.calledRun || M.onRuntimeInitialized == null)) return resolve()
      if (Date.now() - start > timeoutMs) return reject(new Error('Engine Module timeout'))
      setTimeout(tick, 50)
    }
    tick()
  })
}

export async function init(canvas: HTMLCanvasElement) {
  // Prepare Module before loading engine
  (window as any).Module = (window as any).Module || {}
  window.Module.canvas = canvas
  // Surface Emscripten status updates in dev console (helps diagnose data file fetch)
  window.Module.setStatus = (s: string) => { try { console.debug('[Engine]', s) } catch {} }
  // Extra diagnostics for init failures
  window.Module.onAbort = (what: any) => { try { console.error('[Engine abort]', what) } catch {} }
  window.Module.onExit = (code: number) => { try { console.warn('[Engine exit]', code) } catch {} }
  window.Module.onRuntimeInitialized = () => { try { console.info('[Engine] runtime initialized') } catch {} }
  
  // Fix filename mismatch: emscripten generates objviewer.* but we renamed to glint3d.*
  window.Module.locateFile = (path: string, scriptDirectory: string) => {
    if (path === 'objviewer.wasm') return scriptDirectory + 'glint3d.wasm'
    if (path === 'objviewer.data') return scriptDirectory + 'glint3d.data'
    return scriptDirectory + path
  }
  
  await loadEngine()
  await waitForModuleReady()
  // Wait for app to be ready (g_app set)
  try {
    const isReady = window.Module.cwrap ? window.Module.cwrap('app_is_ready', 'number', []) : null
    const start = Date.now()
    while (isReady && isReady() !== 1) {
      if (Date.now() - start > 15000) throw new Error('Engine app not ready')
      await new Promise(r => setTimeout(r, 50))
    }
  } catch {}
}

export function applyOps(ops: any | any[]): boolean {
  const M = window.Module
  if (!M || !M.ccall) { console.error('Module not ready'); return false }
  const payload = Array.isArray(ops) ? ops : [ops]
  const json = JSON.stringify(payload)
  const ok = M.ccall('app_apply_ops_json', 'number', ['string'], [json])
  return ok === 1
}

export function shareLink(): string {
  const M = window.Module
  if (!M || !M.cwrap) return ''
  const getLink = M.cwrap('app_share_link', 'string', [])
  return getLink() || ''
}

export async function mountFileAndLoad(file: File, onLog?: (s: string)=>void) {
  await waitForModuleReady()
  try {
    const arr = new Uint8Array(await file.arrayBuffer())
    const name = Date.now() + '_' + file.name
    const dst = '/uploads/' + name
    try { window.Module.FS_createPath('/', 'uploads', true, true) } catch {}
    window.Module.FS_createDataFile(dst, null, arr, true, true, true)
    const baseName = file.name.replace(/\.[^.]+$/, '')
    const ok = applyOps({ op: 'load', path: dst, name: baseName })
    onLog?.(ok ? `Loaded ${file.name}` : `Failed to load: ${file.name}`)
  } catch (e: any) {
    onLog?.(`Import error: ${e?.message || e}`)
  }
}

export type SceneSnapshot = {
  camera: { position: number[], front: number[] }
  selected: string | null
  objects: { name: string }[]
  lights: { position: number[], color: number[], intensity: number }[]
}

export function getScene(): SceneSnapshot | null {
  const M = window.Module
  if (!M || !M.cwrap) return null
  const getJson = M.cwrap('app_scene_to_json', 'string', [])
  const s = getJson()
  if (!s) return null
  try { return JSON.parse(s) } catch { return null }
}

export function mountBytesAndLoad(name: string, bytes: Uint8Array, onLog?: (s: string)=>void) {
  const M = window.Module
  if (!M || !M.FS_createDataFile) { onLog?.('Module FS not ready'); return false }
  try {
    const dst = '/uploads/' + name
    try { M.FS_createPath('/', 'uploads', true, true) } catch {}
    M.FS_createDataFile(dst, null, bytes, true, true, true)
    const base = name.replace(/\.[^.]+$/, '')
    return applyOps({ op: 'load', path: dst, name: base })
  } catch (e: any) {
    onLog?.('mountBytesAndLoad error: ' + (e?.message || e))
    return false
  }
}

// Tauri helper: open native dialog, read bytes, mount and load
export async function openModelViaTauri(onLog?: (s: string)=>void) {
  const anyWin = window as any
  if (!anyWin.__TAURI__) { onLog?.('Open is available in the desktop app'); return }
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
  const ok = mountBytesAndLoad(name, new Uint8Array(bytes), onLog)
  onLog?.(ok ? `Loaded ${name}` : `Load failed: ${name}`)
}
