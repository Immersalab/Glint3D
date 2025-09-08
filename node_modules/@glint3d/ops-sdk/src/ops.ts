// JSON Ops helper functions and utilities
import type { JsonOp, JsonOps, Vec3, SceneSnapshot, EngineInfo } from './types.js';

export * from './types.js';

// Utility functions for creating operations
export const ops = {
  // Model loading
  load: (path: string, options?: {
    name?: string;
    position?: Vec3;
    scale?: Vec3;
  }): JsonOp => ({
    op: 'load',
    path,
    ...options
  }),

  // Camera operations
  setCamera: (position: Vec3, options?: {
    target?: Vec3;
    front?: Vec3;
    up?: Vec3;
    fov?: number;
  }): JsonOp => ({
    op: 'set_camera',
    position,
    ...options
  }),

  setCameraPreset: (preset: 'front' | 'back' | 'left' | 'right' | 'top' | 'bottom' | 'iso'): JsonOp => ({
    op: 'set_camera_preset',
    preset
  }),

  // Lighting
  addLight: (options?: {
    type?: 'point' | 'directional' | 'spot';
    position?: Vec3;
    direction?: Vec3;
    color?: Vec3;
    intensity?: number;
    inner_deg?: number;
    outer_deg?: number;
  }): JsonOp => ({
    op: 'add_light',
    type: 'point',
    intensity: 1.0,
    color: [1, 1, 1],
    ...options
  }),

  // Object operations
  select: (target: string): JsonOp => ({
    op: 'select',
    type: 'object',
    target
  }),

  selectLight: (index: number): JsonOp => ({
    op: 'select',
    type: 'light',
    index
  }),

  duplicate: (source: string, name?: string): JsonOp => ({
    op: 'duplicate',
    source,
    ...(name && { name })
  }),

  duplicateLight: (index: number): JsonOp => ({
    op: 'duplicate_light',
    index
  }),

  remove: (target: string): JsonOp => ({
    op: 'remove',
    type: 'object',
    target
  }),

  removeLight: (index: number): JsonOp => ({
    op: 'remove',
    type: 'light',
    index
  }),

  // Transformations
  transform: (target: string, options: {
    mode?: 'absolute' | 'delta';
    position?: Vec3;
    rotation?: Vec3;
    scale?: Vec3;
  }): JsonOp => ({
    op: 'transform',
    target,
    mode: options.mode || 'delta',
    [options.mode === 'absolute' ? 'transform_absolute' : 'transform_delta']: {
      position: options.position,
      rotation: options.rotation,
      scale: options.scale
    }
  }),

  // Environment
  setBackground: (options: 
    | { type: 'color'; color: Vec3 }
    | { type: 'skybox'; skybox: string }
  ): JsonOp => ({
    op: 'set_background',
    ...options
  }),

  // Rendering
  render: (path?: string, options?: {
    width?: number;
    height?: number;
    samples?: number;
    denoise?: boolean;
  }): JsonOp => ({
    op: 'render',
    path,
    width: 800,
    height: 600,
    ...options
  }),

  renderImage: (path: string, options?: {
    width?: number;
    height?: number;
  }): JsonOp => ({
    op: 'render_image',
    path,
    width: 800,
    height: 600,
    ...options
  }),

  // Tone mapping
  setExposure: (exposure: number): JsonOp => ({
    op: 'set_exposure',
    exposure
  }),

  setToneMapping: (mode: 'linear' | 'reinhard' | 'filmic' | 'aces'): JsonOp => ({
    op: 'set_tone_mapping',
    mode
  })
};

// Helper to create operation arrays
export function batch(...operations: JsonOp[]): JsonOps {
  return operations;
}

// Validation helpers
export function isValidVec3(value: any): value is Vec3 {
  return Array.isArray(value) && 
         value.length === 3 && 
         value.every(n => typeof n === 'number' && !isNaN(n));
}

export function isValidOperation(value: any): value is JsonOp {
  return typeof value === 'object' && 
         value !== null && 
         typeof value.op === 'string';
}

export function isValidOperations(value: any): value is JsonOps {
  if (isValidOperation(value)) return true;
  if (Array.isArray(value)) {
    return value.every(isValidOperation);
  }
  return false;
}

// Common operation patterns
export const patterns = {
  // Load model and position it
  loadAndPlace: (path: string, position: Vec3, name?: string): JsonOps => [
    ops.load(path, { name }),
    ops.setCamera([position[0] + 2, position[1] + 2, position[2] + 2], { 
      target: position 
    })
  ],

  // Create studio lighting setup
  studioLighting: (): JsonOps => [
    ops.addLight({ 
      type: 'directional', 
      direction: [-0.5, -0.7, -0.5], 
      intensity: 1.0,
      color: [1, 0.95, 0.8] // warm key light
    }),
    ops.addLight({ 
      type: 'directional', 
      direction: [0.8, -0.2, 0.3], 
      intensity: 0.3,
      color: [0.8, 0.9, 1] // cool fill light
    }),
    ops.addLight({ 
      type: 'directional', 
      direction: [0, 0.5, -1], 
      intensity: 0.5,
      color: [1, 1, 1] // rim light
    })
  ],

  // Camera presets sequence
  cameraOrbit: (steps: number = 8): JsonOps => {
    const operations: JsonOp[] = [];
    for (let i = 0; i < steps; i++) {
      const angle = (i / steps) * Math.PI * 2;
      const radius = 3;
      operations.push(ops.setCamera([
        Math.cos(angle) * radius,
        1.5,
        Math.sin(angle) * radius
      ], { target: [0, 0, 0] }));
    }
    return operations;
  }
};

export default { ops, batch, patterns, isValidVec3, isValidOperation, isValidOperations };