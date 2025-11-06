<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/artifacts/code/glint_init_scaffolding_design.md","purpose":"Outlines architecture and modular components for the glint init scaffolding command.","exports":["command_flow","template_pipeline","asset_handshake"],"depends_on":["ai/tasks/glint_cli_platform_v1/artifacts/documentation/cli_product_spec.md","ai/tasks/glint_cli_platform_v1/artifacts/documentation/glint_project_manifest_and_run_manifest_spec.md","ai/tasks/glint_cli_platform_v1/artifacts/documentation/config_precedence_and_settings.md"],"notes":["supports_templates","deterministic_outputs","extensible_modules"]}
<!-- Human Summary -->
Design for the `glint init` implementation: command wiring, template system, asset onboarding, and deterministic outputs that align with the new CLI platform architecture.

# `glint init` Scaffolding Design

## 1. Objectives
- Scaffold a ready-to-run workspace with deterministic structure and metadata.
- Provide modular template selection with extension hooks for studios.
- Ensure generated artifacts integrate with configuration precedence, module locking, and asset synchronization.

## 2. Command Overview
- Entry point: `glint init [--workspace <dir>] [--template <name>] [--with-samples] [--force] [--json]`.
- Secondary options:
  - `--no-config`: skip `.glint/config.json` (advanced).
  - `--module <name>` (repeatable): pre-enable modules.
  - `--asset-pack <name>` (repeatable): request asset packs during init.
  - `--dry-run`: produce plan without writing files.
- Exit codes follow CLI contract (`UnknownFlag`, `DependencyError`, `RuntimeError`).

## 3. Execution Pipeline
1. **Parse Arguments**
   - Normalize workspace path (default current directory).
   - Resolve template fallback sequence: explicit flag -> project manifest defaults -> built-in `blank`.
2. **Preflight Checks**
   - Ensure destination directory exists (create if missing) and is empty unless `--force`.
   - Validate template availability from registry.
   - Load module registry and asset catalog metadata (JSON descriptors stored in `resources/templates/index.json` and `resources/assets/index.json`).
3. **Plan Generation**
   - Build an in-memory plan listing file operations, module toggles, and asset sync steps.
   - Validate dependencies: selected template must support requested modules/assets.
   - On `--dry-run`, emit plan via NDJSON and exit success without changes.
4. **Workspace Creation**
   - Create directory skeleton using plan (see Section 4).
   - Copy template files via deterministic file copier (preserve LF line endings, compute SHA-256 for run manifest).
5. **Manifest Authoring**
   - Generate `glint.project.json` using template blueprint plus CLI overrides.
   - Seed `.glint/config.json` with template defaults.
   - Write `modules.lock` and `assets.lock` seeded from template manifest.
6. **Asset Handshake (optional)**
   - If `--with-samples` or asset packs specified, stage download manifest (`assets/packs/<pack>.manifest.json`) but defer actual downloads to `glint assets sync`.
7. **Summary Output**
   - Print deterministic NDJSON events describing each action.
   - Provide next steps: `glint validate`, `glint render`, `glint assets sync`.

## 4. Directory Skeleton
```
<workspace>/
  glint.project.json
  .glint/
    config.json
    cache/              # Created lazily
  modules/
    modules.lock
  assets/
    packs/
  renders/
  shots/
    README.md
  templates/            # Optional local overrides
```

## 5. Template System
- Templates packaged as directories under `resources/templates/<name>/`.
- Each template ships with:
  - `template.json`: metadata (display name, supported modules, sample scenes).
  - `project.patch.json`: JSON patch applied to base manifest.
  - `config.defaults.json`: baseline workspace config.
  - Optional `assets/` subtree for sample data pointers.
- `TemplateEngine` component responsibilities:
  - Enumerate templates.
  - Load metadata and verify compatibility with CLI version (semver range).
  - Apply JSON patches to produce manifest.
  - Support studio overrides via `$GLINT_TEMPLATE_PATH`.

## 6. Module Enablement
- Module registry derived from engine modular layout (`engine/modules` metadata).
- `glint init` writes `modules.lock` with deterministic ordering:
  ```json
  {
    "schema_version": "1.0.0",
    "modules": [
      {"name": "core", "version": "3.0.0", "enabled": true},
      {"name": "raytracing", "version": "2.1.0", "enabled": true}
    ]
  }
  ```
- Modules requested via `--module` merge into template defaults; compatibility checks ensure dependencies satisfied.

## 7. Asset Packs
- Asset catalog entries live in `resources/assets/index.json`.
- CLI records chosen packs in `assets.lock` but defers download:
  ```json
  {
    "schema_version": "1.0.0",
    "packs": [
      {"name": "studio-core", "version": "1.2.0", "status": "pending"}
    ]
  }
  ```
- `--with-samples` acts as shorthand for template-defined pack list.

## 8. Deterministic Outputs
- All generated files use LF endings, UTF-8 encoding.
- CLI writes run manifest stub under `renders/_init/run.json` capturing scaffolding plan if `--json` is provided.
- File operations log includes SHA-256 digests to assist auditing.

## 9. Extensibility Points
- **Template Hooks**: templates may expose `post_init` scripts (YAML) describing follow-up commands; CLI validates but does not execute automatically in v1.
- **Workspace Plugins**: future tasks can allow `--plugin <name>` to register additional scaffolding steps (for example custom pipeline integration). Designed as separate layer around plan generation stage.
- **Studio Overrides**: environment variable `GLINT_TEMPLATE_PATH` or `--template-path` flag allows additional template directories.

## 10. Error Handling
- Missing template: `RuntimeError` with guidance to run `glint templates list`.
- Non-empty directory without `--force`: `RuntimeError`.
- Incompatible module or asset pack: `DependencyError`.
- I/O failure writing files: `RuntimeError` with path details; clean up partial files when safe.

## 11. Testing Strategy
- Smoke test `glint init --dry-run` to confirm plan generation.
- Integration test to scaffold workspace in temp directory and validate produced manifest via schema.
- Snapshot test ensuring generated manifest/config match template fixtures.

---
Ownership: Platform Team - Milestone A (CLI Platform v1)
