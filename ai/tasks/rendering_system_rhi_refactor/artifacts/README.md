<!-- Machine Summary Block -->
{"file":"ai/tasks/rendering_system_rhi_refactor/artifacts/README.md","purpose":"Describes artifacts generated during rendering system RHI refactoring.","exports":["artifact_inventory"],"depends_on":["../task.json"],"notes":["living_document","remove_stale_artifacts"]}
<!-- Human Summary -->
Directory containing deliverables produced during the rendering system refactoring, including architecture documentation, code specifications, validation reports, and migration guides.

# Rendering System RHI Refactor Task Artifacts

This directory captures deliverables generated while refactoring the rendering system to use RHI abstraction and unified materials.

## Documentation Artifacts
- `documentation/architecture_audit_report.md` — Comprehensive audit documenting current dual-pipeline architecture, material duplication patterns, OpenGL coupling, legacy code inventory, and refactoring recommendations with file paths and line numbers.
- `documentation/rhi_interface_specification.md` — Complete RHI interface specification defining abstract GPU operations, platform-agnostic types, error handling patterns, and backend implementation contract.
- `documentation/material_core_specification.md` — MaterialCore struct definition with property descriptions, valid ranges, pipeline usage patterns, and migration path from dual storage.
- `documentation/migration_guide.md` — Step-by-step guide for developers implementing RHI backends, porting shaders, and adopting MaterialCore in new features.

## Code Artifacts
- `code/rhi_abstraction_layer.md` — Design notes for RHI interface hierarchy, RhiGL implementation details, resource handle management, and state tracking patterns.
- `code/material_unification_notes.md` — Running log of material system changes, conversion utilities, shader uniform mappings, and deprecation timeline for legacy Material class.

## Validation Artifacts
- `validation/golden_image_regression_tests.md` — Test scene inventory, SSIM comparison results, visual diff reports, and regression test automation scripts for ensuring rendering parity.
- `validation/performance_benchmarks.md` — Frame time measurements, RHI overhead analysis, memory usage delta, draw call counts, and performance budget validation across platforms.

## Maintenance Notes
Remove any temporary notes or WIP documentation once the refactor is complete. This directory should contain only final deliverables and historical context for future reference.
