// File loading utilities for web environment
import { applyOperations, mountFile, isEngineReady, EngineError } from './engine.js';

// Simplified ops helper - will be enhanced when used with ops-sdk
const ops = {
  load: (path: string, options?: { name?: string }) => ({
    op: 'load',
    path,
    ...(options?.name && { name: options.name })
  })
};

export interface FileLoadOptions {
  name?: string;
  onProgress?: (loaded: number, total: number) => void;
  onSuccess?: (message: string) => void;
  onError?: (error: string) => void;
}

export interface FileLoadResult {
  success: boolean;
  message: string;
  path?: string;
}

export type JsonOp = any; // Will be properly typed when used with ops-sdk

/**
 * Load a file into the engine from File object
 */
export async function loadFile(file: File, options: FileLoadOptions = {}): Promise<FileLoadResult> {
  if (!isEngineReady()) {
    throw new EngineError('Engine not ready');
  }

  const { onProgress, onSuccess, onError } = options;
  
  try {
    // Read file data
    const arrayBuffer = await readFileWithProgress(file, onProgress);
    const data = new Uint8Array(arrayBuffer);
    
    // Generate unique filename
    const timestamp = Date.now();
    const filename = `${timestamp}_${file.name}`;
    
    // Mount in engine filesystem
    const enginePath = mountFile(filename, data);
    
    // Extract base name for object naming
    const baseName = options.name || file.name.replace(/\.[^.]+$/, '');
    
    // Load via JSON ops
    const success = applyOperations(ops.load(enginePath, { name: baseName }));
    
    const message = success ? `Loaded ${file.name}` : `Failed to load ${file.name}`;
    
    if (success) {
      onSuccess?.(message);
    } else {
      onError?.(message);
    }
    
    return {
      success,
      message,
      path: success ? enginePath : undefined
    };
    
  } catch (error) {
    const errorMessage = `Import error: ${error instanceof Error ? error.message : error}`;
    onError?.(errorMessage);
    
    return {
      success: false,
      message: errorMessage
    };
  }
}

/**
 * Load multiple files sequentially
 */
export async function loadFiles(
  files: File[], 
  options: FileLoadOptions = {}
): Promise<FileLoadResult[]> {
  const results: FileLoadResult[] = [];
  
  for (const file of files) {
    const result = await loadFile(file, options);
    results.push(result);
  }
  
  return results;
}

/**
 * Load JSON operations from file
 */
export async function loadJsonOpsFile(file: File, options: FileLoadOptions = {}): Promise<FileLoadResult> {
  const { onSuccess, onError } = options;
  
  try {
    const text = await file.text();
    const operations = JSON.parse(text);
    
    const success = applyOperations(operations);
    const message = success 
      ? `Loaded scene from ${file.name}` 
      : `Failed to load scene from ${file.name}`;
    
    if (success) {
      onSuccess?.(message);
    } else {
      onError?.(message);
    }
    
    return { success, message };
    
  } catch (error) {
    const errorMessage = `JSON parse error: ${error instanceof Error ? error.message : error}`;
    onError?.(errorMessage);
    
    return {
      success: false,
      message: errorMessage
    };
  }
}

/**
 * Create file input element for model loading
 */
export function createModelFileInput(
  onLoad: (result: FileLoadResult) => void,
  options: FileLoadOptions = {}
): HTMLInputElement {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = '.obj,.ply,.glb,.gltf,.fbx,.dae';
  
  input.onchange = async (e) => {
    const file = (e.target as HTMLInputElement).files?.[0];
    if (file) {
      const result = await loadFile(file, options);
      onLoad(result);
    }
  };
  
  return input;
}

/**
 * Create file input element for JSON operations
 */
export function createJsonFileInput(
  onLoad: (result: FileLoadResult) => void,
  options: FileLoadOptions = {}
): HTMLInputElement {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = '.json';
  
  input.onchange = async (e) => {
    const file = (e.target as HTMLInputElement).files?.[0];
    if (file) {
      const result = await loadJsonOpsFile(file, options);
      onLoad(result);
    }
  };
  
  return input;
}

/**
 * Handle drag and drop file loading
 */
export function setupDropZone(
  element: HTMLElement,
  onLoad: (results: FileLoadResult[]) => void,
  options: FileLoadOptions = {}
): () => void {
  const handleDragOver = (e: DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    element.classList.add('drag-over');
  };
  
  const handleDragLeave = (e: DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    element.classList.remove('drag-over');
  };
  
  const handleDrop = async (e: DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    element.classList.remove('drag-over');
    
    const files = Array.from(e.dataTransfer?.files || []);
    if (files.length === 0) return;
    
    // Separate JSON and model files
    const jsonFiles = files.filter(f => f.name.endsWith('.json'));
    const modelFiles = files.filter(f => !f.name.endsWith('.json'));
    
    const results: FileLoadResult[] = [];
    
    // Load JSON files first
    for (const file of jsonFiles) {
      const result = await loadJsonOpsFile(file, options);
      results.push(result);
    }
    
    // Then load model files
    for (const file of modelFiles) {
      const result = await loadFile(file, options);
      results.push(result);
    }
    
    onLoad(results);
  };
  
  // Add event listeners
  element.addEventListener('dragover', handleDragOver);
  element.addEventListener('dragleave', handleDragLeave);
  element.addEventListener('drop', handleDrop);
  
  // Return cleanup function
  return () => {
    element.removeEventListener('dragover', handleDragOver);
    element.removeEventListener('dragleave', handleDragLeave);
    element.removeEventListener('drop', handleDrop);
    element.classList.remove('drag-over');
  };
}

// Helper function to read file with progress
function readFileWithProgress(
  file: File, 
  onProgress?: (loaded: number, total: number) => void
): Promise<ArrayBuffer> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    
    reader.onload = () => {
      resolve(reader.result as ArrayBuffer);
    };
    
    reader.onerror = () => {
      reject(new Error('File read error'));
    };
    
    reader.onprogress = (e) => {
      if (e.lengthComputable && onProgress) {
        onProgress(e.loaded, e.total);
      }
    };
    
    reader.readAsArrayBuffer(file);
  });
}