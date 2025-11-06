<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/artifacts/code/cli_architecture_overview.md","purpose":"Outlines the architecture and scaffolding plan for Glint CLI primary commands.","exports":["command_surface","component_layers","ndjson_contract"],"depends_on":["ai/tasks/glint_cli_platform_v1/artifacts/documentation/cli_product_spec.md","ai/tasks/glint_cli_platform_v1/artifacts/documentation/config_precedence_and_settings.md","ai/tasks/glint_cli_platform_v1/artifacts/code/glint_init_scaffolding_design.md"],"notes":["phase2_scaffolding","structured_output","determinism_hooks"]}
<!-- Human Summary -->
Architecture plan for wiring Glint CLI v1 commands, covering the dispatcher, shared services, structured output, and determinism hooks.

# Glint CLI v1 Architecture Overview

## 1. Command Surface & Dispatcher
- `glint` executable resolves commands via `CliDispatcher`, a thin front-end that parses global flags (`--project`, `--config`, `--json`, `--verbosity`) and routes to verb-specific handlers.
- Commands implement a lightweight `ICommand` interface:
  ```cpp
  struct CommandExecutionContext { int argc; char** argv; GlobalOptions globals; };
  struct CommandResult { CLIExitCode code; std::vector<OutputEvent> events; };
  class ICommand { public: virtual CommandResult run(const CommandExecutionContext&) = 0; virtual ~ICommand() = default; };
  ```
- The dispatcher performs:
  1. Normalize workspace + manifest context using `ConfigResolver`.
  2. Instantiate the concrete command (`InitCommand`, `ValidateCommand`, etc.).
  3. Stream `OutputEvent` entries via `ndjson::Emitter`, ensuring deterministic ordering.
- Global flags are consumed before hand-off so each command receives only verb-specific arguments.

## 2. Shared Component Layer
| Component | Responsibility | Consumers |
| --- | --- | --- |
| `ConfigResolver` | Merge layered configuration, surface provenance for `config --explain`. | `validate`, `render`, `config`, `doctor` |
| `RunManifestWriter` | Persist `renders/<name>/run.json` with schema versions, module hashes, RNG seeds, timings. | `render`, `validate`, `inspect` (when materializing snapshots) |
| `ModuleRegistry` | Read `engine/modules` metadata, enforce dependencies, update `modules.lock`. | `init`, `modules`, `render`, `validate` |
| `AssetCatalog` | Enumerate asset packs, track sync state in `assets.lock`. | `init`, `assets`, `render` |
| `DiagnosticsSuite` | Execute health checks (GPU driver, schema cache, module ABI). | `doctor`, optionally `render` preflight |
| `WorkspaceGuard` | Enforce safe path operations, lock concurrency around `.glint/` writes. | `init`, `clean`, `modules`, `assets` |

All shared services live under `cli/include/glint/cli/services/` with headers exposing pure abstractions. Concrete implementations use engine subsystems (`engine/core`, `engine/modules`).

## 3. Command Scaffolding
### 3.1 Init (delivered)
- See `glint_init_scaffolding_design.md` for full plan.
- Emits `init_operation` and `init_completed` events; dry-runs produce plan only.

### 3.2 Validate
- Pipeline:
  1. Resolve effective manifest + configuration.
  2. Invoke schema validators (project, modules, assets) from `schemas/`.
  3. Emit NDJSON events per validation phase (`project_validated`, `modules_validated`, `assets_validated`).
- Exit code `SchemaValidationError` on any failure; final summary event includes counts.

### 3.3 Inspect
- Provides human and JSON summaries for manifests, modules, assets.
- Supports `--output` to write formatted report (JSON/YAML) while still emitting NDJSON to stdout.
- Reuses `ConfigResolver` and `ModuleRegistry`; optional `--schema` dumps currently loaded schema hashes.

### 3.4 Render
- Preflight:
  - Validate manifest + scene.
  - Determine render device (`cpu|gpu|auto`) and seed.
- Execution:
  - Stream `render_started`, frame events, `render_completed`.
  - Persist run manifest via `RunManifestWriter` capturing CLI args, config snapshots, module digests, timing.
- Supports `--plan` for dry-run (no frames rendered) while producing run manifest preview.

### 3.5 Config
- Supports `--get`, `--set`, `--unset`, `--scope`.
- Uses `ConfigResolver` plus `ConfigMutator` helper to apply writes to appropriate layer (`global`, `workspace`, `project`).
- Emits `config_diff` events showing before/after values; `--json` ensures deterministic ordering of keys.

### 3.6 Clean
- Removes caches, modules, or assets with safety checks.
- `WorkspaceGuard` ensures paths remain within workspace root.
- Dry-run mode lists candidate paths; `clean_completed` event enumerates removals.

### 3.7 Doctor
- Runs diagnostics defined in `DiagnosticsSuite`.
- Each check produces `doctor_check` event with status (`passed`, `warning`, `failed`).
- Optional `--fix` triggers remediation actions (config refresh, module repair) and emits `doctor_fix_applied`.

### 3.8 Modules
- Subcommands:
  - `list`: enumerate modules with enabled status.
  - `enable`/`disable`: update `modules.lock`, run dependency solver.
  - `sync`: verify installed binaries vs. lockfile.
- Structured output: `modules_event` with `action`, `module`, `status`.
- Integrates with `ModuleRegistry` and ensures deterministic lockfile writes for reproducibility.

### 3.9 Assets
- Subcommands mirror modules; manage `assets.lock` and download manifests.
- `sync` prepares jobs for asset fetcher but defers transfer to future RPC integration.
- Emits `assets_event` per pack plus final summary.

## 4. Structured Output & Logging
- `ndjson::Emitter` handles pretty-free JSON writing (no allocations beyond buffer reuse).
- Each command registers event schemas (type-safe wrappers) to guarantee key consistency.
- Human mode uses `Logger` with severity gating; when `--json` is set, stdout reserved for NDJSON and stderr for human-readable errors.
- Verbose logging controlled via global `--verbosity` flag; default `info`.

## 5. Determinism Hooks
- All file writes pass through `DeterministicFileWriter` which enforces:
  - UTF-8 LF encoding.
  - Stable key ordering when serializing JSON (RapidJSON + custom sorter).
  - SHA-256 digest computation stored alongside outputs (run manifest or diff events).
- Run manifest schema (from manifest spec) versioned and validated before emit.
- Random seeds derived from manifest or CLI override; persisted in run manifest to ensure reproducibility.

## 6. Command Lifecycle State Machine
1. `command_started` event (common for all verbs).
2. Zero or more `command_progress` events (verb-specific).
3. Optional `command_warning` events (non-fatal issues).
4. `command_completed` with exit code + summary metrics.
- On failure, emit `command_failed` with `error_code`, `message`, `details`.

## 7. Testing & Validation Plan
- **Smoke Suite (`artifacts/tests/cli_smoke_suite.md`)**: orchestrates scenario coverage across commands, verifying exit codes and NDJSON payloads.
- **Build Matrix (`artifacts/validation/cli_build_matrix.md`)**: enumerates supported platforms and ensures deterministic output hashes.
- **Documentation Sync**: `glint_cli_command_reference.md` generated from command metadata to keep docs aligned with implementation.

---
Ownership: Platform Team - Milestone A (CLI Platform v1)
