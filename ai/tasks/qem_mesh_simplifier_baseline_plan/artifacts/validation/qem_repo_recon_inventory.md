<!-- Machine Summary Block -->
{"file":"ai/tasks/qem_mesh_simplifier_baseline_plan/artifacts/validation/qem_repo_recon_inventory.md","purpose":"Records inspected repository files and observations used to ground A-C of the QEM planning task.","exports":["inspected_paths","recon_findings"],"depends_on":["CMakeLists.txt","engine/core/io","engine/core/scene","engine/modules/raytracing"],"notes":["A_to_C_supporting_evidence"]}
<!-- Human Summary -->
Evidence inventory for the A-C draft. This file lists the repo paths inspected and the concrete findings used for architecture and API recommendations.

# QEM Repo Recon Inventory (A-C Support)

## Inspected Build and Topology Files

- `CMakeLists.txt`
  - Confirms CMake build, `glint_core` static library, `glint` executable, optional raytracing/gizmo flags.
  - Confirms current desktop UI + ImGui sources are compiled into `glint_core`.

## Inspected Mesh / Geometry / Scene Files

- `engine/core/io/mesh_loader.h`
  - `MeshData` indexed triangle representation (`positions`, `indices`, optional normals/uvs/tangents).
- `engine/core/io/mesh_loader.cpp`
  - Unified loader dispatches through importer registry.
- `engine/core/io/importer.h`
  - Existing pattern for clean interface + optional backend implementations (useful precedent for QEM core/adapters/io split).
- `engine/core/io/importer_registry.h`
  - Static importer registry pattern.
- `engine/core/io/objloader.h`
  - `ObjLoader` stores positions + `Face{a,b,c}` and exposes rendering-oriented accessors.
- `engine/core/scene/scene_manager.h`
  - `SceneObject` mixes mesh (`ObjLoader`) with OpenGL handles and scene/material state.
- `engine/modules/raytracing/triangle.h`
  - Alternate triangle primitive representation (`v0,v1,v2`).
- `engine/modules/raytracing/raytracer.h`
  - Raytracer owns `std::vector<Triangle>` and accepts `ObjLoader` input.

## Inspected Structure / Conventions Docs

- `engine/core/README.md`
- `engine/core/io/README.md`
- `engine/core/scene/README.md`
- `engine/modules/README.md`
- `engine/platform/desktop/README.md`

These confirm the package split between core, modules, and desktop platform glue, and support the recommendation to keep QEM core outside rendering/UI dependencies.

## Search Findings (negative evidence)

- No half-edge / winged-edge / generic adjacency mesh structure found in engine source search.
- No `Eigen` headers or `Eigen::` symbols found in `.h/.cpp` files.
- `glm` usage is widespread across engine code, making `glm` a practical adapter-layer type but not mandatory for QEM core.

## Implications for A-C

- Prefer `MeshData` as the main Glint adapter boundary.
- Keep QEM core independent and self-contained.
- Build an external UI first and consume Glint rendering via API/CLI bridge, with optional in-Glint panel later.
