<!-- Machine Summary Block -->
{"file":"ai/tasks/qem_mesh_simplifier_baseline_plan/artifacts/documentation/qem_baseline_simplifier_plan.md","purpose":"Repo-grounded planning document for baseline QEM simplifier integration with Glint (A-F planning deliverables).","exports":["repo_recon","proposed_architecture","public_api_design","baseline_qem_algorithm_plan","implementation_checklist","build_integration_notes"],"depends_on":["CMakeLists.txt","engine/core/io/mesh_loader.h","engine/core/scene/scene_manager.h","engine/core/io/importer.h"],"notes":["external_ui_first","qem_core_no_viewer_dependencies","A_to_F_planning_deliverables_present"]}
<!-- Human Summary -->
A-F planning draft for the baseline QEM mesh simplifier task: repo recon, removable module architecture, public API, baseline QEM algorithm plan, staged implementation/test checklist, and build/integration notes designed around an external UI first workflow with a thin Glint adapter.

# Baseline QEM Mesh Simplifier Plan (A-F Planning Draft)

## A) Repo Recon

### Build System (inferred from repo inspection)

- Build system is CMake (root `CMakeLists.txt` declares `project(glint C CXX)` and defines targets such as `glint_core`, `glint`, and `user_paths_probe`).
- `glint_core` is currently a static library, and `glint` is the desktop executable linked against it.
- Optional module toggles exist in CMake (`GLINT_ENABLE_RAYTRACING`, `GLINT_ENABLE_GIZMOS`), which is a useful precedent for making QEM integration optional.

### Top-Level Structure (current conventions)

- `engine/`
  - `core/` contains application, I/O, rendering, and scene systems.
  - `modules/` contains optional features (raytracing, gizmos, post-processing).
  - `platform/desktop/` contains UI bridge, ImGui UI layer, desktop file dialogs.
  - `include/` currently exposes mostly UI/platform-facing headers (`ui_bridge.h`, `imgui_ui_layer.h`, panels).
- `cli/`
  - `include/glint/cli` and `src/commands`, `src/services` for the newer CLI surface.
- `third_party/`
  - `vendored/` includes `glm`, `imgui`, `rapidjson`, `glad`, `stb`, etc.
  - `managed/` includes `assimp`, `glfw`, `openimagedenoise` drop-ins.
- `resources/`, `schemas/`, `docs/`, `tools/` are top-level support areas.

### Rendering / GUI Organization (relevant to QEM isolation)

- OpenGL rendering code lives in `engine/core/rendering/*`.
- Desktop UI/ImGui integration lives in `engine/platform/desktop/*` and `third_party/vendored/imgui/backends/*`.
- Current CMake wiring includes desktop UI sources and ImGui sources in `glint_core`, which means a new QEM algorithm library should not be added directly into existing `glint_core` if we want strict UI independence.

### Existing Mesh / Geometry Representations Found

1. `MeshData` (`engine/core/io/mesh_loader.h`)
- Indexed triangle mesh using:
  - `std::vector<glm::vec3> positions`
  - `std::vector<unsigned> indices` (triangle list, 3*n)
  - optional normals/uvs/tangents
- This is the best thin-adapter input/output target for a baseline QEM simplifier.

2. `ObjLoader` (`engine/core/io/objloader.h`)
- Stores positions and a `Face { unsigned a, b, c; }` triangle list.
- Also stores normals/texcoords/tangents and provides raw-pointer accessors for rendering upload.
- Used by scene/raytracing paths, but it mixes asset representation with engine-specific conveniences.

3. `SceneObject` (`engine/core/scene/scene_manager.h`)
- Owns an `ObjLoader` and OpenGL handles (`VAO`, `VBO_*`, `EBO`) plus materials and transforms.
- This is not a good QEM core input type because it is tightly coupled to rendering state and scene runtime concerns.

4. Raytracing primitives (`engine/modules/raytracing/triangle.h`, `raytracer.h`)
- CPU raytracer stores a `std::vector<Triangle>` with explicit `v0/v1/v2` positions.
- Useful as evidence of another triangle representation, but not ideal as the primary simplifier interface because it is path-tracer specific and non-indexed.

### Adjacency / Topology Structures (what was and was not found)

- No half-edge, winged-edge, or dedicated adjacency-cache mesh structure was found in the inspected engine code.
- No generic mesh simplification module exists today.
- Conclusion: baseline QEM should build its own internal adjacency/topology bookkeeping in the QEM core and adapt from `MeshData` (preferred) or `ObjLoader` via thin adapters.

### Math Libraries (found)

- `glm` is the dominant math library (widely used across engine and CLI-adjacent codepaths).
- No `Eigen` usage was found in repository `.h/.cpp` search.
- Recommendation for removability: keep QEM core math self-contained (small local vector/matrix types or plain structs), and keep `glm` usage in Glint adapters only.

### What could not be fully determined from inspection (A-specific unknowns)

- There is no stable, documented public plugin ABI for loading external UI/tool plugins into Glint at runtime.
- The intended long-term in-process API boundary for external tools (library API vs. RPC vs. CLI invocation) is not fully standardized in the inspected files.

Reasonable options:
1. Direct in-process adapter path first (link against QEM static lib from Glint and/or external UI).
2. Out-of-process integration path first (external UI calls Glint CLI now, RPC later when `glint_rpc_daemon` matures).

## B) Proposed Architecture

### Decision (chosen direction)

Recommended and chosen path: External UI first, with an optional thin Glint panel later.

Why this matches the repo and the constraints:
- The repo has CMake optional-module patterns, but no runtime plugin system.
- `SceneObject` is tightly coupled to OpenGL/UI concerns.
- A separate UI keeps QEM core removable, testable, and independent from the current viewer stack.
- Future decomposition animation and analysis workflows fit better in a dedicated tool surface.

### Recommended Placement in This Repo (minimal churn)

Use the existing top-level `tools/` area for the external tool and keep the QEM library inside that subtree as an exportable library target.

```text
tools/qem_simplifier/
  CMakeLists.txt                      # standalone targets; can also be add_subdirectory'ed
  README.md

  include/
    glint_qem/
      simplify.h                      # public core API (SimplifyOptions, SimplifyResult, Simplify)
      types.h                         # core mesh/value types, status enums
      deterministic.h                 # epsilon + tie-break policy structs
      collapse_trace.h                # optional collapse event stream for animation playback

    glint_qem_glint/
      meshdata_adapter.h              # MeshData <-> qem mesh
      objloader_adapter.h             # ObjLoader <-> qem mesh (optional convenience)
      scene_object_extract.h          # read-only extraction from SceneObject -> MeshData/qem mesh

  src/
    core/
      simplify.cpp
      topology.cpp                    # internal adjacency + active/dead bookkeeping
      quadrics.cpp                    # plane equations, quadric accumulation, error eval
      collapse.cpp                    # validation + local updates
      heap.cpp                        # deterministic priority queue wrappers / comparators
      cleanup.cpp                     # compaction + remap generation

    adapters/
      glint/
        meshdata_adapter.cpp
        objloader_adapter.cpp
        scene_object_extract.cpp
        meshdata_apply.cpp            # write simplified mesh back to MeshData/ObjLoader

    io/                               # optional, not required by core
      qem_mesh_io.h/.cpp              # simple OBJ/PLY read/write for tool-only workflows

    ui/                               # external UI app (primary workflow)
      app_main.cpp
      panels/
        mesh_input_panel.cpp
        simplify_controls_panel.cpp
        collapse_timeline_panel.cpp   # future decomposition animation playback
        metrics_panel.cpp
      render_bridge/
        glint_cli_render_client.cpp   # first path: invoke glint CLI/headless render
        glint_rpc_client.cpp          # future path when daemon/API is available

    integration/
      glint_panel/                    # optional in-Glint panel (secondary/debug path)
        qem_simplify_panel.cpp
        qem_simplify_panel.h
        qem_panel_bridge.cpp

  tests/
    unit/
      qem_quadrics_tests.cpp
      qem_topology_tests.cpp
      qem_collapse_validation_tests.cpp
      qem_determinism_tests.cpp
    data/
      tetra.obj
      cube_tris.obj
      plane_grid.obj
      bowtie_nonmanifold.obj
      duplicate_vertex_case.obj

  examples/
    simplify_basic.cpp
    simplify_with_glint_meshdata.cpp
```

### Layer Separation (explicit)

1. Core library (`include/glint_qem`, `src/core`)
- Contains the algorithm and internal topology bookkeeping only.
- Accepts in-memory indexed triangle mesh data.
- No OpenGL, ImGui, GLFW, Assimp, Polyscope, or Glint scene/render headers.
- No file I/O dependency.

2. Adapters (`include/glint_qem_glint`, `src/adapters/glint`)
- Converts between Glint mesh representations (`MeshData`, optionally `ObjLoader`) and QEM core types.
- Handles attribute policy decisions (baseline: recompute normals or keep untouched if remap is not yet supported).
- Is the only layer that includes Glint headers and `glm` if desired.

3. I/O (`src/io`)
- Optional convenience for tool workflows and tests.
- Not needed when Glint already loads meshes.
- Can be excluded from builds to keep dependencies minimal.

4. Viewer integration (`src/ui`, `src/integration/glint_panel`)
- `src/ui` is the primary external UI (separate process/app).
- `src/integration/glint_panel` is optional and behind a compile flag for in-Glint debugging/convenience.
- Both consume the same core + adapter APIs.

### Dependency Policy (important for removability)

- `qem_core` target: STL only.
- `qem_glint_adapter` target: depends on Glint headers/types (`MeshData`, `ObjLoader`) and may use `glm`.
- `qem_tool_ui` target: depends on chosen UI toolkit plus either Glint CLI bridge or future RPC bridge.
- `qem_glint_panel` target: optional, depends on Glint desktop UI headers and compile flags.

### Data Ownership / Lifetime Rules (architecture-level)

- QEM core does not retain external pointers after `Simplify(...)` returns.
- Adapters own all conversions and copies between Glint and QEM types.
- External UI owns collapse-trace playback state (for decomposition animation) and rendered preview orchestration.
- If in-place simplification is supported, the caller owns mesh memory and explicitly opts into mutation.

## C) Public API Design

### Design Goals

- Minimal API surface for baseline adoption.
- Core API independent of Glint and `glm`.
- Deterministic behavior configurable and explicit.
- Sufficient stats and trace hooks for future analysis/animation.

### Public Core Types (proposed)

```cpp
// tools/qem_simplifier/include/glint_qem/types.h
#pragma once
#include <cstdint>
#include <vector>

namespace glint_qem {

struct Vec3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct IndexedTriangleMesh {
    std::vector<Vec3f> positions;
    std::vector<uint32_t> indices; // 3 * triangle_count, triangle list
};

enum class SimplifyStatus {
    kSuccess,
    kInvalidInput,
    kNoReductionPossible,
    kStoppedByTarget,
    kStoppedByErrorLimit,
    kStoppedByMaxCollapses,
    kInternalError
};

} // namespace glint_qem
```

```cpp
// tools/qem_simplifier/include/glint_qem/simplify.h
#pragma once
#include <cstdint>
#include <limits>
#include <optional>
#include "glint_qem/types.h"

namespace glint_qem {

struct EpsilonPolicy {
    float area_epsilon = 1e-12f;
    float determinant_epsilon = 1e-10f;
    float position_merge_epsilon = 1e-9f;
    float normal_flip_cos_epsilon = -0.999f; // guard threshold for invalid flips
};

struct DeterminismPolicy {
    bool deterministic = true;
    uint64_t stable_seed = 0; // reserved for future randomized tie-breakers, default unused
    bool stable_priority_queue_tiebreak = true;
    bool stable_edge_enumeration = true;
    bool stable_vertex_reindex_on_output = true;
};

struct SimplifyOptions {
    std::optional<uint32_t> target_triangle_count;
    std::optional<float> target_ratio; // (0,1], applied to input triangle count
    std::optional<double> max_error;
    std::optional<uint32_t> max_collapses;

    bool compact_output = true;
    bool emit_collapse_trace = false; // supports future decomposition animation

    EpsilonPolicy epsilon{};
    DeterminismPolicy determinism{};
};

struct SimplifyStats {
    uint32_t input_vertex_count = 0;
    uint32_t input_triangle_count = 0;
    uint32_t output_vertex_count = 0;
    uint32_t output_triangle_count = 0;
    uint32_t attempted_collapses = 0;
    uint32_t accepted_collapses = 0;
    uint32_t rejected_collapses = 0;
    double final_max_edge_error = 0.0;
    double accumulated_error = 0.0;
};

struct CollapseEvent {
    uint32_t step = 0;
    uint32_t kept_vertex = 0;
    uint32_t removed_vertex = 0;
    Vec3f new_position{};
    double cost = 0.0;
};

struct SimplifyResult {
    SimplifyStatus status = SimplifyStatus::kInternalError;
    SimplifyStats stats{};
    std::vector<uint32_t> old_to_new_vertex_map; // empty if compact_output=false and not requested
    std::vector<CollapseEvent> collapse_trace;    // filled only when emit_collapse_trace=true
    const char* message = "";                    // short status summary; no retained ownership requirements
};

// Out-of-place simplify (default API for safe integration)
SimplifyResult Simplify(const IndexedTriangleMesh& input,
                        const SimplifyOptions& options,
                        IndexedTriangleMesh& output);

// Optional in-place variant for tool workflows
SimplifyResult SimplifyInPlace(IndexedTriangleMesh& mesh,
                               const SimplifyOptions& options);

} // namespace glint_qem
```

### Glint Adapter API (thin boundary)

```cpp
// tools/qem_simplifier/include/glint_qem_glint/meshdata_adapter.h
#pragma once
#include "mesh_loader.h"            // Glint MeshData
#include "glint_qem/types.h"
#include "glint_qem/simplify.h"

namespace glint_qem_glint {

// Copies positions + indices out of MeshData for QEM core.
glint_qem::IndexedTriangleMesh ToQemMesh(const MeshData& src);

// Applies simplified geometry back into MeshData.
// Baseline policy: positions/indices replaced, normals recomputed (uvs/tangents preserved only if still valid, else cleared).
bool ApplySimplifiedMesh(const glint_qem::IndexedTriangleMesh& simplified,
                        MeshData& dst,
                        std::string* warning = nullptr);

// Convenience wrapper used by Glint call sites.
glint_qem::SimplifyResult SimplifyMeshData(const MeshData& input,
                                           const glint_qem::SimplifyOptions& options,
                                           MeshData& output,
                                           std::string* warning = nullptr);

} // namespace glint_qem_glint
```

### Header-Level Usage Example (how Glint would call it)

```cpp
#include "mesh_loader.h"
#include "glint_qem/simplify.h"
#include "glint_qem_glint/meshdata_adapter.h"

MeshData loaded;
std::string err;
if (!LoadMeshFromFile(path, loaded, nullptr, &err)) {
    // handle load failure
}

glint_qem::SimplifyOptions opts;
opts.target_ratio = 0.35f;
opts.max_collapses = 50000;
opts.emit_collapse_trace = true; // enables future decomposition animation playback
opts.determinism.deterministic = true;
opts.determinism.stable_priority_queue_tiebreak = true;

MeshData simplified;
std::string warn;
auto result = glint_qem_glint::SimplifyMeshData(loaded, opts, simplified, &warn);
if (result.status != glint_qem::SimplifyStatus::kSuccess &&
    result.status != glint_qem::SimplifyStatus::kStoppedByTarget) {
    // handle simplification failure / no-op
}

// Glint can now render `simplified` using existing upload paths.
```

### External UI First Integration Pattern (how this supports your direction)

- External QEM UI loads or receives mesh data, calls `glint_qem::Simplify(...)`, and stores `collapse_trace` for playback.
- Rendering can be delegated to Glint in two stages:
  1. Near-term: invoke `glint render` / headless CLI with generated mesh snapshots or scripted runs.
  2. Later: call a stable Glint API/RPC endpoint when daemon/API work lands.
- Optional in-Glint panel reuses the same core and adapter APIs, but stays secondary and behind a compile flag.

### Ownership / Lifetime Rules (API-level, explicit)

- `Simplify(...)` reads `input` and writes `output`; it never stores references to caller memory after return.
- `SimplifyInPlace(...)` mutates caller-owned mesh buffers only during the call.
- `message` in `SimplifyResult` is a short static string (or implementation-defined stable string literal), so no caller free is required.
- `collapse_trace` and `old_to_new_vertex_map` are fully owned by the returned result object.
- Adapter functions copy between `MeshData` and QEM types to avoid aliasing with Glint render/upload buffers.

### Open Questions for D-F (deferred)

- Attribute propagation policy beyond baseline (UV/tangent remap vs. recompute/clear).
- Non-manifold and boundary handling policy in baseline vs. later constrained version.
- Exact collapse event schema needed for animation scrubbing and metric overlays.

## D) Baseline QEM Algorithm Plan

### Scope (baseline, intentionally unconstrained)

- Triangle meshes only (`IndexedTriangleMesh`: positions + triangle indices).
- Baseline edge-collapse QEM without feature preservation, boundary locking, or attribute-aware errors.
- Deterministic execution is required (same input + options => same output on same platform/build).
- Core implementation remains headless and renderer-agnostic; visualization is handled by the existing QEM preview harness.

### Internal Data Model (qem_core private implementation)

Recommended private structures in `tools/qem_simplifier/src/core/`:

```cpp
struct QemQuadric {
    // Symmetric 4x4 stored in compact form (10 doubles).
    double m[10];
};

struct VertexRec {
    Vec3d p;
    QemQuadric Q;
    uint32_t version;     // increments on topology/position changes
    bool alive;
    bool boundary_hint;   // optional baseline cache (computed from edge incidence)
};

struct FaceRec {
    uint32_t v[3];
    Vec4d plane;          // normalized plane (a,b,c,d)
    double area;
    bool alive;
};

struct EdgeKey { uint32_t a, b; }; // canonicalized: a < b

struct EdgeRec {
    EdgeKey key;
    uint32_t v_keep;      // candidate orientation-independent storage; collapse picks final keep/remove
    uint32_t v_remove;
    Vec3d optimal_pos;
    double cost;
    uint64_t tie_id;      // deterministic heap tiebreak
    uint32_t version_a;   // captured vertex versions for stale-entry detection
    uint32_t version_b;
};
```

Notes:
- Use `double` internally for planes/quadrics/cost solves even if public mesh uses `float`.
- Do not expose any of the above through `glint_qem` public headers.
- Keep output compaction/remapping separate from topology mutation logic.

### Baseline Pipeline (exact execution order)

1. Validate and normalize input
- Reject if index count is not divisible by 3.
- Reject any out-of-range indices.
- Build an initial compact internal copy of vertices/faces (skip no-op duplicate compaction for baseline unless input is obviously invalid).
- Remove or mark degenerate input faces (duplicate indices or near-zero area) before quadric construction.

2. Compute face planes
- For each alive face `(i,j,k)`:
  - `n = normalize(cross(pj - pi, pk - pi))`
  - `d = -dot(n, pi)`
  - plane = `(a,b,c,d) = (n.x,n.y,n.z,d)`
- Face area:
  - `area = 0.5 * |cross(pj - pi, pk - pi)|`
- Baseline weighting:
  - Start with area-weighted plane quadrics (`Kp *= area`) for better behavior on irregular tessellation.
  - Keep this as a fixed baseline choice (documented), not runtime-tunable yet.

3. Build per-face quadrics
- For each face plane `p = [a b c d]^T`, compute outer product:
  - `Kp = p * p^T` (symmetric 4x4)
- Store only compact symmetric terms.

4. Accumulate per-vertex quadrics
- Initialize all vertex quadrics to zero.
- For each alive face, add `Kp` to each incident vertex quadric.
- Optional baseline cache:
  - mark boundary edges/vertices from edge incidence counts (edges with one incident face).

5. Build adjacency and edge set
- Build face-to-vertex (already present), vertex-to-face incidence, and edge-to-face incidence.
- Enumerate unique undirected edges from alive faces using canonical `(min,max)` keys.
- For each edge, compute initial collapse candidate:
  - combined quadric `Q = Qv[a] + Qv[b]`
  - optimal position solve (see below)
  - fallback candidate if singular
  - cost evaluation at chosen position
- Push `EdgeRec` into min-heap with deterministic tiebreak (`cost`, `a`, `b`, `tie_id`).

6. Priority queue collapse loop
- Stop when any enabled target is reached:
  - target triangle ratio/count
  - error tolerance
  - max collapses
  - heap exhausted / no valid collapses
- Loop:
  - Pop min edge candidate.
  - Revalidate (both vertices alive, versions match, edge still exists, local topology still legal).
  - Recompute candidate/cost if necessary (or treat stale entry as discard and continue).
  - Run collapse guards (degenerate/flip/duplicate/non-manifold checks per baseline policy).
  - If invalid: count rejected, continue.
  - Accept collapse:
    - choose keep/remove vertex IDs (deterministic policy; usually keep lower ID)
    - write new position to kept vertex
    - accumulate quadric: `Q_keep += Q_remove`
    - mark removed vertex dead
    - rewrite incident faces (`remove -> keep`)
    - kill degenerate faces created by rewrite
    - recompute planes/quadrics for affected faces
    - update adjacency in affected 1-ring
    - bump versions on impacted vertices
    - recompute/push neighboring edge candidates
  - Record `CollapseEvent` if enabled.

7. Output reconstruction / compaction
- Collect alive vertices/faces.
- If `compact_output=true`, generate dense vertex remap and compact indices.
- Populate `old_to_new_vertex_map` when requested / implied.
- Fill stats and terminal status (`kSuccess`, `kStoppedByTarget`, `kNoOp`, `kInvalidInput`, etc.).

### Required Adjacency Operations (robust baseline)

The baseline implementation does not need a full half-edge mesh if adjacency operations are explicit and local.

Minimum required adjacency:
- `vertex -> incident faces` (dynamic list/set)
- `face -> 3 vertices`
- `edge -> incident face count` (for uniqueness + boundary detection + collapse validation)
- `vertex -> neighboring vertices` (can be derived from incident faces, but caching is faster)

Recommended implementation strategy (baseline)
- Use stable IDs with `alive` flags for vertices/faces; do not erase from vectors during collapse loop.
- Maintain:
  - `std::vector<VertexRec> vertices`
  - `std::vector<FaceRec> faces`
  - `std::vector<std::vector<uint32_t>> vertex_faces`
  - `std::unordered_map<EdgeKey, EdgeIncidence, EdgeKeyHash>` (or sorted vector map if determinism/perf tradeoffs favor it)
- On collapse, update only the local neighborhood:
  - union of incident faces of `keep` and `remove`
  - union of neighboring vertices around those faces

Why this is sufficient for baseline:
- Edge-collapse legality checks are local (1-ring).
- We avoid half-edge complexity while still supporting deterministic local rewrites and incremental updates.

### Optimal Collapse Position Solve (QEM core math)

For candidate edge `(u,v)`:
- Combined quadric `Q = Qu + Qv`
- Solve for position `x = [x y z 1]^T` minimizing `x^T Q x`

Standard 3x3 solve:
- Let:
  - `A = Q(0:2,0:2)` (upper-left 3x3)
  - `b = -[Q03, Q13, Q23]^T`
- Solve `A * p = b`
- If `det(A)` (or condition estimate) is acceptable, use `p` as optimal position.

Singular / unstable fallback policy (deterministic)
- Evaluate cost at three fixed candidates:
  1. endpoint `pu`
  2. endpoint `pv`
  3. midpoint `0.5 * (pu + pv)`
- Choose minimum cost; tie-break by fixed order (`pu`, `pv`, midpoint`) or lexicographic position compare.
- Do not use random perturbation in baseline.

Cost evaluation
- For point `p`, evaluate homogeneous vector `[p.x p.y p.z 1]`.
- Cost = `x^T Q x`
- Clamp tiny negative values caused by numerical noise to zero for reporting/comparisons if within epsilon.

### Collapse Validation / Guards (baseline safety)

Required guards before accepting a collapse:

1. Vertex/edge liveness
- Both endpoints alive
- Versions match queued candidate
- Edge still exists in current adjacency

2. Topology degeneracy guard
- Reject if rewriting `remove -> keep` creates faces with repeated indices.
- Reject if collapse would produce duplicate faces in the affected neighborhood (same canonical triangle key).

3. Zero-area / near-zero area guard
- For each affected surviving face after simulated rewrite:
  - compute area
  - reject if `area <= eps_area`

4. Normal flip guard (local)
- For each affected face that survives:
  - compare old normal and new normal
  - reject if:
    - dot(old_n, new_n) < `min_normal_dot`
    - or new normal invalid / near-zero
- Baseline threshold should be conservative and fixed in `EpsilonPolicy`.

5. Duplicate vertex position guard (optional but recommended baseline)
- Reject collapse if new position is within `eps_pos` of an unrelated neighbor and would create collapsed/overlapping triangles in 1-ring.

6. Boundary behavior (baseline unconstrained, but guarded)
- Baseline does not preserve boundaries as features.
- Still reject obviously destructive collapses that break local manifold assumptions:
  - edge with >2 incident faces (non-manifold edge) => skip collapse in baseline
  - vertex neighborhoods that fail simple manifold checks => skip collapse

Resulting policy:
- Baseline is permissive for manifold triangle meshes and conservative on non-manifold input (skip unsafe collapses rather than attempting repair).

### Deterministic Behavior Requirements (must-have)

Determinism rules for `qem_core` baseline:

- Edge keys are canonicalized `(min(v0,v1), max(v0,v1))`.
- Vertex kept/removed orientation is deterministic (prefer lower ID as keep unless guard requires otherwise; document exact rule).
- Heap ordering is stable:
  - primary: `cost`
  - secondary: `edge_key.a`
  - tertiary: `edge_key.b`
  - quaternary: monotonic `tie_id`
- No unordered iteration is allowed to affect decisions:
  - if using hash containers, materialize/sort edge updates before pushing when order impacts behavior
  - or use ordered containers for candidate generation in baseline
- Epsilon values are explicit in `SimplifyOptions::epsilon`; no hidden magic constants.
- Output compaction maps vertices in ascending original ID order.
- Collapse trace step numbering increments strictly by accepted collapse count.

### Baseline `qem_core` Implementation Plan (mapped to current scaffold)

File layout (private core internals):

```text
tools/qem_simplifier/src/core/
  simplify_stub.cpp              # replace implementation body, keep exported API signatures
  qem_internal_types.h           # VertexRec/FaceRec/EdgeRec/QemQuadric
  qem_quadric.h / .cpp           # plane quadric math, cost eval, 3x3 solve
  qem_adjacency.h / .cpp         # local topology bookkeeping + edge enumeration
  qem_collapse.h / .cpp          # validation + face rewrite + local updates
  qem_simplifier.cpp             # orchestration loop (may be merged into simplify_stub.cpp initially)
```

Recommended incremental landing order (for actual implementation, not just doc):
- Land quadric math + unit tests first (`plane -> Kp`, `Q` accumulation, solve/fallback).
- Land adjacency build + edge enumeration + deterministic ordering.
- Land a single validated collapse on a tetrahedron/cube fixture.
- Land full collapse loop + stopping criteria + stats.
- Land output compaction + trace emission.

This aligns with the existing external QEM shell + Glint preview harness already in repo, so visual checks can start as soon as "single collapse" works.

## E) Implementation Checklist

### Staged Implementation Plan (with checkpoints)

The goal is to replace the current stub in `tools/qem_simplifier/src/core/simplify_stub.cpp` incrementally while preserving a runnable vertical slice after each stage.

1. Stage 0: Test harness + fixture setup (no rendering)
- Add a small `qem_core` test target (module-local) with no engine/viewer deps.
- Add tiny fixture meshes in code (tetrahedron, cube, plane patch) and optional fixture loaders later.
- Checkpoint:
  - test executable builds and runs from `tools/qem_simplifier` standalone CMake.

2. Stage 1: Input validation + output passthrough skeleton
- Replace stub body with real validation path and explicit status codes.
- Keep passthrough output behavior while wiring stats and early errors (`kInvalidInput`, `kNoOp`).
- Checkpoint:
  - invalid index / non-triangle input tests pass.
  - shell `simplify` still runs without crashing.

3. Stage 2: Quadric math primitives
- Implement:
  - plane normalization
  - area computation
  - face quadric (`p p^T`)
  - quadric accumulation
  - cost evaluation `x^T Q x`
  - 3x3 solve + singular fallback
- Checkpoint:
  - unit tests for known plane/cost cases.
  - deterministic fallback selection for singular systems.

4. Stage 3: Adjacency build + edge enumeration
- Build internal `VertexRec` / `FaceRec` arrays and local incidence structures.
- Enumerate unique undirected edges and initial edge candidates in deterministic order.
- Checkpoint:
  - tetrahedron/cube edge counts are exact.
  - repeated runs produce identical edge ordering.

5. Stage 4: Single validated collapse (manual step path)
- Implement local collapse simulation/validation for one selected edge:
  - face rewrite
  - degenerate face cull
  - flip/area guards
  - local adjacency updates
- Checkpoint:
  - one accepted collapse on tetrahedron reduces triangle count as expected.
  - rejected invalid collapses increment rejection stats.

6. Stage 5: Priority queue loop (N collapses)
- Add stale-entry tolerant min-heap and local edge recosting after each accepted collapse.
- Implement stopping criteria (`max_collapses`, targets, heap exhaustion).
- Checkpoint:
  - `N` accepted collapses on cube/grid fixtures.
  - counts are monotonic and deterministic.

7. Stage 6: Output compaction + remap + trace emission
- Emit compact mesh output and optional `old_to_new_vertex_map`.
- Emit `collapse_trace` with deterministic step ordering for animation playback.
- Checkpoint:
  - output indices in range.
  - remap validity tests pass.
  - trace length equals accepted collapses when enabled.

8. Stage 7: Adapter integration (Glint mesh bridge) + preview validation
- Wire `MeshData` adapter helpers (thin copy in/out).
- Use existing `previewmesh` / `previewui` harness for visual sanity checks.
- Checkpoint:
  - simplify a known mesh and preview staged output through Glint.
  - no `qem_core` dependency on OpenGL/UI libs.

9. Stage 8: Target reduction behavior + regressions
- Validate ratio/count/error stop conditions on known meshes.
- Capture regression cases from failures (non-manifold edges, duplicate faces, skinny triangles).
- Checkpoint:
  - repeated runs produce stable counts and trace for the same input/options.

### Unit Test Strategy (headless / no rendering)

Repo recon did not identify an existing first-party unit test framework or CTest-based test suite in the main project (only vendored third-party tests). For the QEM module, use a module-local test target with minimal dependencies.

Recommended baseline approach (no new dependency required):
- `tools/qem_simplifier/tests/qem_core_tests.cpp`
- simple assertion-based runner (or lightweight custom macros) compiled as an executable
- optional later migration to Catch2/Doctest if the repo adopts a shared test framework

Suggested test fixtures (small, deterministic)
- `tetrahedron`: smallest closed manifold; good for first collapse and exact counts
- `cube_triangulated`: predictable edge set and multiple valid collapses
- `plane_grid_nxm`: boundary-heavy mesh for reduction/guard behavior
- `two_triangles_shared_edge`: local edge collapse validation
- `degenerate_triangles`: repeated indices / zero-area faces
- `duplicate_vertices_same_position`: duplicate-position guard behavior
- `nonmanifold_edge_fixture`: edge with >2 incident faces (baseline should skip unsafe collapses)

Core invariants to assert after every simplify run
- index count divisible by 3
- all output indices in range
- no alive/output face has repeated vertex indices
- no output face area below `eps_area` (within chosen baseline policy)
- `attempted_collapses >= accepted_collapses`
- `attempted_collapses == accepted_collapses + rejected_collapses` (if every attempt is classified)
- triangle count and vertex count do not increase in simplification runs
- `collapse_trace.size() == accepted_collapses` when `emit_collapse_trace=true`
- `old_to_new_vertex_map` validity when compaction/remap is requested

Determinism tests (must-have)
- Run the same input/options twice in-process and compare:
  - output vertex/index buffers (or a canonicalized hash)
  - stats
  - collapse trace contents
- Add at least one regression test where multiple edges have equal/near-equal cost to verify stable tie-break behavior.

Failure-mode tests (important early)
- invalid input indices
- empty mesh / zero-triangle mesh
- all faces degenerate
- target already satisfied (expect `kNoOp` or `kStoppedByTarget`, per chosen API behavior)

### Practical Development Validation Loop (with current scaffold)

- Headless correctness first:
  - run `qem_core` tests only
- Then shell integration:
  - `tools\qem_simplifier\build-and-run.bat debug run`
  - `simplify` (stub today, real core later)
- Then visual sanity:
  - `previewmesh` / `previewui` using the existing Glint bridge and preview preset workflow

This keeps algorithm debugging isolated from renderer/UI issues while still preserving a visual check path.

## F) Build / Integration Notes

### Build System (inferred and confirmed)

- Glint uses CMake at the repo root (`CMakeLists.txt` present and active).
- The QEM scaffold already builds standalone via `tools/qem_simplifier/CMakeLists.txt`.
- A root-level opt-in hook already exists for the scaffold (default off), minimizing churn for integration into the main solution.

### Recommended Target Split (minimal churn, removable)

Keep the module split aligned with the architecture:

```text
glint_qem_core              # static lib (or object lib): pure algorithm + public API
glint_qem_glint_adapter     # static lib: MeshData <-> qem_core conversions
glint_qem_tool_backends     # static lib: render bridge + shell helpers (Glint CLI backend, staging)
glint_qem_studio_shell      # executable: external shell/UI harness
glint_qem_core_tests        # executable: headless tests (optional, gated)
```

Dependency rules:
- `glint_qem_core`
  - depends only on STL + `glint_qem` public headers
  - no OpenGL / GLFW / ImGui / scene renderer / CLI dependencies
- `glint_qem_glint_adapter`
  - depends on Glint mesh types/loaders (`MeshData` etc.) and `glint_qem_core`
  - no direct viewer/UI dependency
- `glint_qem_tool_backends`
  - may depend on shell/system process launching and staging helpers
  - can target Glint CLI today, other renderers later
- `glint_qem_studio_shell`
  - depends on backend interfaces/implementations only

### Integration Into Glint’s CMake (recommended path)

Near-term (already working pattern)
- Continue building the QEM tool standalone from `tools/qem_simplifier/` for fast iteration.
- Use local scripts (`tools/qem_simplifier/build-and-run.bat`) for day-to-day work.

Optional root solution integration (minimal churn)
- Keep an opt-in cache option (existing scaffold pattern) so the QEM tool/module is only added when requested.
- Add/include the QEM subdirectory under a guarded CMake option rather than wiring targets unconditionally.
- Expose include directories from targets via `target_include_directories(...)` and link via target names only (no global include path churn).

Suggested compile flags / options (QEM module-local)
- `GLINT_QEM_BUILD_TESTS` (default `OFF` in root, `ON` for standalone dev)
- `GLINT_QEM_BUILD_TOOL_SHELL` (default `ON` in standalone, optional in root)
- `GLINT_QEM_ENABLE_GLINT_ADAPTER` (default `ON` in repo; can be `OFF` for pure core packaging)
- `GLINT_QEM_ENABLE_VIEWER_PANEL` (future, default `OFF`)

### Viewer / GUI Integration (optional, behind flags)

- The QEM core must remain usable without compiling any viewer/UI code.
- Any future in-Glint panel should be:
  - thin
  - optional (`GLINT_QEM_ENABLE_VIEWER_PANEL`)
  - implemented against the same `glint_qem` API and adapter boundary
- The current external shell + Glint CLI preview bridge remains the primary integration/test path.

### I/O Policy (core vs optional layers)

- `glint_qem_core` accepts in-memory meshes only (`IndexedTriangleMesh`).
- Mesh file load/save stays outside core:
  - Glint adapter path for integration with existing `MeshData`
  - optional `io` layer for standalone OBJ/PLY utilities if needed later
- This keeps the core removable and easy to package independently.

### Removability / Packaging Guidance

To submit as a standalone plugin/library later:
- Keep public headers under `tools/qem_simplifier/include/glint_qem/`
- Avoid leaking Glint types into `glint_qem` headers
- Keep Glint-specific code in adapters/backends only
- Maintain standalone `tools/qem_simplifier/CMakeLists.txt` as the source of truth for module builds
- Treat backend interfaces as the staging ground for a future runtime plugin ABI, not the ABI itself

### Known Unknowns / Reasonable Options (F-specific)

What could not be conclusively inferred from the repo:
- a preferred first-party unit test framework for new module code
- a standardized install/export pattern for optional internal tools/plugins

Reasonable options:
1. Start with a module-local custom test executable (lowest dependency overhead), then migrate if/when a shared framework is adopted.
2. Add a small CMake install/export stanza for `glint_qem_core` only later, after the public API stabilizes (avoid premature packaging churn during baseline algorithm iteration).

## Implementation Addendum (Vertical Slice Scaffold)

The repository now contains a buildable scaffold under `tools/qem_simplifier/` that validates the agreed architecture direction before the real QEM algorithm is implemented.

Implemented scaffold components:
- Stubbed `qem_core` headers + pass-through implementation (`include/glint_qem/*`, `src/core/simplify_stub.cpp`)
- Backend interfaces + plugin-ABI-conscious contracts (`include/glint_qem_tool/backends.h`, `include/glint_qem_tool/backend_contracts.h`)
- `glint_render_bridge` CLI backend (`include/glint_qem_tool/glint_cli_render_backend.h`, `src/backends/glint_cli_render_backend.cpp`)
- Minimal external shell UI (`src/ui/qem_studio_shell.cpp`)
- Backend interface discussion doc (`tools/qem_simplifier/docs/BACKEND_INTERFACE_DISCUSSION.md`)

This supports early testing of UI/backend/render orchestration while keeping the real QEM implementation isolated and still pending (Deliverable D onward).
