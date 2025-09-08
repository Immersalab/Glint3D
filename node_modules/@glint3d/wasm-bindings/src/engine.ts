// WebAssembly engine bindings with improved type safety and error handling
// Define types locally to avoid circular dependencies
export type JsonOps = any; // Will be properly typed when used with ops-sdk
export type SceneSnapshot = any; // Will be properly typed when used with ops-sdk  
export type EngineInfo = any; // Will be properly typed when used with ops-sdk

declare global { 
  interface Window { 
    Module: EmscriptenModule & {
      canvas?: HTMLCanvasElement;
      calledRun?: boolean;
      onRuntimeInitialized?: (() => void) | null;
    };
  } 
}

export interface EmscriptenModule {
  cwrap: (
    ident: string,
    returnType: string,
    argTypes?: string[]
  ) => (...args: any[]) => any;
  ccall: (
    ident: string,
    returnType: string,
    argTypes?: string[],
    args?: any[]
  ) => any;
  FS_createPath: (
    parent: string,
    path: string,
    canRead: boolean,
    canWrite: boolean
  ) => void;
  FS_createDataFile: (
    parent: string,
    name: string,
    data: Uint8Array,
    canRead: boolean,
    canWrite: boolean,
    canOwn: boolean
  ) => void;
}

// Engine loading state management
let engineState: 'loading' | 'ready' | 'error' | null = null;
let engineLoadPromise: Promise<void> | null = null;

export class EngineError extends Error {
  constructor(message: string, public cause?: unknown) {
    super(message);
    this.name = 'EngineError';
  }
}

export interface EngineLoadOptions {
  scriptPath?: string;
  timeoutMs?: number;
  canvas?: HTMLCanvasElement;
}

/**
 * Load and initialize the WebAssembly engine
 */
export async function loadEngine(options: EngineLoadOptions = {}): Promise<void> {
  const {
    scriptPath = '/engine/glint3d.js',
    timeoutMs = 15000,
    canvas
  } = options;

  // Return existing promise if already loading
  if (engineLoadPromise) return engineLoadPromise;

  engineState = 'loading';
  engineLoadPromise = (async () => {
    try {
      // Prepare Module object
      const module = (window as any).Module || {};
      if (canvas) module.canvas = canvas;
      (window as any).Module = module;

      // Load engine script
      await loadScript(scriptPath);
      
      // Wait for runtime initialization
      await waitForRuntime(timeoutMs);
      
      // Wait for application ready
      await waitForApplication(timeoutMs);
      
      engineState = 'ready';
    } catch (error) {
      engineState = 'error';
      engineLoadPromise = null; // Allow retry
      throw new EngineError('Failed to load engine', error);
    }
  })();

  return engineLoadPromise;
}

/**
 * Check if engine is loaded and ready
 */
export function isEngineReady(): boolean {
  return engineState === 'ready' && window.Module?.cwrap != null;
}

/**
 * Get engine loading state
 */
export function getEngineState(): typeof engineState {
  return engineState;
}

/**
 * Apply JSON operations to the engine
 */
export function applyOperations(ops: JsonOps): boolean {
  if (!isEngineReady()) {
    throw new EngineError('Engine not ready - call loadEngine() first');
  }

  try {
    const payload = Array.isArray(ops) ? ops : [ops];
    const json = JSON.stringify(payload);
    const result = window.Module.ccall('app_apply_ops_json', 'number', ['string'], [json]);
    return result === 1;
  } catch (error) {
    throw new EngineError('Failed to apply operations', error);
  }
}

/**
 * Get current scene state snapshot
 */
export function getSceneSnapshot(): SceneSnapshot | null {
  if (!isEngineReady()) {
    throw new EngineError('Engine not ready - call loadEngine() first');
  }

  try {
    const getJson = window.Module.cwrap('app_scene_to_json', 'string', []);
    const jsonString = getJson();
    
    if (!jsonString) return null;
    
    return JSON.parse(jsonString) as SceneSnapshot;
  } catch (error) {
    console.warn('Failed to get scene snapshot:', error);
    return null;
  }
}

/**
 * Generate share link for current scene state
 */
export function generateShareLink(): string {
  if (!isEngineReady()) {
    throw new EngineError('Engine not ready - call loadEngine() first');
  }

  try {
    const getLink = window.Module.cwrap('app_share_link', 'string', []);
    return getLink() || '';
  } catch (error) {
    console.warn('Failed to generate share link:', error);
    return '';
  }
}

/**
 * Get engine information and capabilities
 */
export function getEngineInfo(): EngineInfo | null {
  if (!isEngineReady()) return null;

  try {
    // Try to get engine info via JSON (if implemented)
    const getInfo = window.Module.cwrap('app_get_info', 'string', []);
    const infoString = getInfo?.();
    
    if (infoString) {
      return JSON.parse(infoString) as EngineInfo;
    }
    
    // Fallback to basic info
    return {
      version: '0.3.0',
      buildType: 'release',
      platform: 'web',
      features: {
        raytracer: false,
        denoise: false,
        assimp: true,
        ktx2: false
      },
      limits: {
        maxTriangles: 2000000,
        maxTextureSize: 4096,
        maxLights: 32
      }
    };
  } catch (error) {
    console.warn('Failed to get engine info:', error);
    return null;
  }
}

/**
 * Mount file data in engine filesystem
 */
export function mountFile(name: string, data: Uint8Array): string {
  if (!isEngineReady()) {
    throw new EngineError('Engine not ready - call loadEngine() first');
  }

  try {
    const path = `/uploads/${name}`;
    
    // Ensure uploads directory exists
    try {
      window.Module.FS_createPath('/', 'uploads', true, true);
    } catch {
      // Directory might already exist
    }
    
    // Mount file data
    window.Module.FS_createDataFile(path, null as any, data, true, true, true);
    
    return path;
  } catch (error) {
    throw new EngineError(`Failed to mount file: ${name}`, error);
  }
}

/**
 * Check if engine application is ready
 */
export function checkApplicationReady(): boolean {
  if (!isEngineReady()) return false;
  
  try {
    const isReady = window.Module.cwrap('app_is_ready', 'number', []);
    return isReady() === 1;
  } catch {
    return false;
  }
}

// Private helper functions
async function loadScript(src: string): Promise<void> {
  return new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.src = src;
    script.async = true;
    script.onload = () => resolve();
    script.onerror = () => reject(new Error(`Failed to load script: ${src}`));
    document.head.appendChild(script);
  });
}

async function waitForRuntime(timeoutMs: number): Promise<void> {
  const start = Date.now();
  
  return new Promise((resolve, reject) => {
    const check = () => {
      const module = window.Module;
      
      if (module && (module.calledRun || module.onRuntimeInitialized === null)) {
        return resolve();
      }
      
      if (Date.now() - start > timeoutMs) {
        return reject(new Error('Runtime initialization timeout'));
      }
      
      setTimeout(check, 50);
    };
    
    check();
  });
}

async function waitForApplication(timeoutMs: number): Promise<void> {
  const start = Date.now();
  
  return new Promise((resolve, reject) => {
    const check = () => {
      try {
        if (checkApplicationReady()) {
          return resolve();
        }
      } catch {
        // Function might not be available yet
      }
      
      if (Date.now() - start > timeoutMs) {
        return reject(new Error('Application initialization timeout'));
      }
      
      setTimeout(check, 50);
    };
    
    check();
  });
}