// Main export for @glint3d/ops-sdk
export * from './types.js';
export * from './ops.js';

// Re-export commonly used items for convenience
export { 
  ops, 
  batch, 
  patterns,
  isValidVec3,
  isValidOperation,
  isValidOperations 
} from './ops.js';

export type {
  JsonOp,
  JsonOps,
  Vec3,
  SceneSnapshot,
  EngineInfo,
  OpLoad,
  OpSetCamera,
  OpAddLight,
  OpSetBackground,
  OpDuplicate,
  OpTransform,
  OpSelect,
  OpRemove,
  OpRender
} from './types.js';