<!-- Machine Summary Block -->
{"file":"ai/tasks/engine_modular_architecture/artifacts/code/refactor_patchset_notes.md","purpose":"Captures implementation notes for the modular architecture refactor patchset.","exports":[],"depends_on":["artifacts/documentation"],"notes":["phase4_desktop_cleanup","cmake_switch_removal"]}
<!-- Human Summary -->
Patch-level commentary on the Phase 4 modularization work: desktop source relocations, CMake cleanup, and documentation refresh.

# Refactor Patchset Notes (Phase 4)

## Desktop Platform Relocation
- Moved ImGui UI bridge (`ui_bridge.cpp`), editor layer, file dialog, and scene tree panel into `engine/platform/desktop/`.
- Added Machine Summary metadata to relocated sources and introduced `engine/platform/desktop/panels/README.md` for the new subdirectory.
- Removed obsolete `engine/src/` tree and deleted legacy ImGui backend copies from `engine/`.

## CMake & Build Configuration
- Introduced `GLINT_PLATFORM_DESKTOP_SOURCES` and per-scope include lists (`GLINT_CORE_PUBLIC_INCLUDE_DIRS`, `GLINT_CORE_PRIVATE_INCLUDE_DIRS`).
- Updated targets to consume third-party ImGui backends from `third_party/vendored/imgui/backends` and tightened include visibility for `glint_core`, `glint`, and tooling targets.
- Eliminated transitional `SRC_DIR`/`IMGUI_DIR` aliases, ensuring builds only reference the modular directory hierarchy.

## Documentation & Tooling
- Refreshed the root `README.md`, `ai/FOR_DEVELOPERS.md`, and `docs/doxygen/Doxyfile` to describe the modular layout.
- Added Machine Summary Blocks where missing and documented the new platform panels directory.
- Staged `artifacts/validation/build_matrix.md` for Phase 4 runs (see validation step outputs).
