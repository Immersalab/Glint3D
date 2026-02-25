<!-- Machine Summary Block -->
{"file":"tools/qem_simplifier/docs/BACKEND_INTERFACE_DISCUSSION.md","purpose":"Explains the staged path from internal backend interfaces to a future true plugin ABI for QEM and rendering backends.","exports":["backend_vs_plugin_guidance","abi_conscious_rules","migration_plan"],"depends_on":["tools/qem_simplifier/include/glint_qem_tool/backend_contracts.h","tools/qem_simplifier/include/glint_qem_tool/backends.h"],"notes":["plugin_ready_shapes","capability_discovery","versioning_first"]}
<!-- Human Summary -->
Rationale and design rules for starting with internal backend interfaces now while intentionally shaping them so they can become a stable runtime plugin contract later.

# Backend Interfaces First, Plugin ABI Later

## Why this is the right order

- Backend interfaces give fast iteration and testability now.
- A true C++ plugin ABI is costly to stabilize too early (compiler/CRT/STL compatibility, ownership rules, versioning).
- The QEM/render/timeline contracts are still evolving; this scaffold is where we discover the right boundaries before freezing them.

## Backend Interface vs Plugin Interface

- Backend interface (current scaffold)
  - In-process C++ abstraction.
  - Compile-time linkage.
  - Easier to change while requirements are being learned.
- Plugin interface (future)
  - Runtime-loaded ABI boundary (`LoadLibrary`/`dlopen`).
  - Versioned and capability-based.
  - Must avoid STL/GLM/engine-private types across the boundary.

## ABI-Conscious Rules Applied Now

- Capability discovery is explicit (`BackendInfo`, capability flags).
- Request/result shapes are narrow and task-oriented (`SimplifyJob`, `RenderPreviewJob`).
- Future plugin-facing shapes are defined in POD-style structs (`backend_contracts.h`).
- The shell UI depends on interfaces, not concrete backend implementations.
- Core QEM API remains renderer/UI-agnostic.

## Migration Path to a True Plugin System

1. Keep using internal backend interfaces while implementing real QEM + render workflows.
2. Freeze request/result schemas once usage stabilizes.
3. Add runtime plugin host and C ABI exports that marshal to/from `backend_contracts.h`.
4. Move concrete backends (CLI render bridge, future RPC render bridge, alt simplifiers) behind runtime-loaded plugins as needed.

## Practical Recommendation

- Continue shipping internal backend interfaces first.
- Treat them as plugin-contract prototypes, not throwaway scaffolding.
- Do not expose Glint scene/render types across the future plugin ABI.
