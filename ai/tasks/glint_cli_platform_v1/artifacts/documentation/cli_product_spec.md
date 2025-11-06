<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/artifacts/documentation/cli_product_spec.md","purpose":"Defines the product vision, personas, and command contracts for Glint CLI v1.","exports":["vision","user_journeys","interface_contracts"],"depends_on":["ai/tasks/glint_cli_platform_v1/task.json","ai/tasks/glint_cli_platform_v1/checklist.md"],"notes":["milestone_a","documents_core_verbs","guides_acceptance"]}
<!-- Human Summary -->
Concept brief for Glint CLI v1 covering goals, primary users, end-to-end journeys, and command surface expectations.

# Glint CLI v1 Product Spec

## 1. Product Vision
- Provide a deterministic command-line front door to Glint3D that mirrors the modular engine layout and exposes reproducible renders and asset management.
- Empower technical directors, CI pipelines, and power users to scaffold projects, validate content, and render headless outputs without touching the desktop UI.
- Serve as the canonical automation surface for future SDKs and the RPC daemon, sharing schemas and provenance artifacts.

### Goals
- Deterministic behavior with stable exit codes and machine-readable output.
- Ergonomic onboarding (`glint init`) with template selection and optional sample assets.
- First-class provenance: every render writes a reproducible `renders/<name>/run.json`.
- Configuration layering that matches studio expectations (flags -> project manifest -> .glint/config.json -> environment -> global config).

### Non-Goals (v1)
- Real-time viewport control or interactive editing.
- Full module marketplace or remote asset hosting.
- GUI installers or platform-dependent packaging (handled in separate tasks).

## 2. Primary Personas

| Persona | Needs | Key Commands |
| --- | --- | --- |
| Pipeline TD (Helena) | Generate deterministic renders in CI, enforce schema compliance, manage modules per show. | `render`, `validate`, `modules`, `doctor` |
| Lookdev Artist (Ravi) | Spin up local workspace with sample assets, tweak configs, submit renders. | `init`, `render`, `config`, `assets` |
| DevOps Engineer (Amelia) | Install CLI on build agents, monitor health, keep configs in sync. | `init`, `doctor`, `clean`, `config` |
| SDK Integrator (Noah) | Integrate CLI verbs into higher-level tooling, rely on structured output. | All commands with `--json`, exit codes, run manifests |

## 3. End-to-End Journeys

1. **New Workspace Bootstrap**
   - `glint init --workspace my_show --template cinematic --with-samples`
   - CLI creates directory structure, writes `glint.project.json`, seeds `.glint/config.json`, optionally downloads sample assets, prints next steps.
2. **CI Validation + Render**
   - `glint validate --project path/to/glint.project.json --strict`
   - `glint render --project path/to/glint.project.json --scene shots/shot010.json --output renders/shot010`
   - Emits `renders/shot010/run.json` capturing args, versions, device info, seeds, timings.
3. **Module & Asset Management**
   - `glint modules list --json` to view status.
   - `glint modules enable raytracing`
   - `glint assets sync --pack studio-core`
4. **Diagnostics & Cleanup**
   - `glint doctor --json` delivers health checks (schema cache, module integrity, GPU drivers).
   - `glint clean --artifacts` purges caches, `glint clean --modules --dry-run` previews removals.

## 4. CLI Surface Contracts

### 4.1 Global Behavior
- All verbs support `--json` for structured NDJSON output (one JSON object per logical event).
- `--verbosity` (`quiet`, `warn`, `info`, `debug`) controls log levels; defaults to `info`.
- `--config <path>` overrides discovery of `.glint/config.json`.
- `--project <path>` pins the active project manifest (`glint.project.json`).
- Deterministic exit codes (see Section 5) and descriptive stderr lines prefixed with timestamp + severity.

### 4.2 Commands

| Command | Synopsis | Key Flags | Output Artifacts | Notes |
| --- | --- | --- | --- | --- |
| `glint init` | Scaffold a new workspace or project. | `--workspace <dir>`, `--template <name>`, `--module <name>`, `--asset-pack <name>`, `--with-samples`, `--no-config`, `--force`, `--dry-run`, `--json` | Creates workspace tree, `glint.project.json`, `.glint/config.json`, optional assets manifest. | Templates referenced from `resources/templates`. Validates destination emptiness unless `--force`; `--dry-run` emits plan without writing. |
| `glint validate` | Validate project manifest, assets, schema versions. | `--project <path>`, `--strict`, `--modules`, `--assets`, `--json` | NDJSON validation report. | Fails with SchemaValidationError on violation. |
| `glint inspect` | Inspect project/components metadata. | `--project`, `--modules`, `--assets`, `--schema`, `--json` | Prints structured summary or writes to `--output`. | Intended for tooling introspection. |
| `glint render` | Execute render jobs defined in manifest or inline. | `--project`, `--scene <path|id>`, `--ops <file>`, `--output <dir>`, `--frames <list>`, `--resume`, `--device <cpu|gpu|auto>`, `--headless`, `--json` | `renders/<name>/run.json`, image outputs, logs. | `run.json` includes CLI args, schema version hashes, module digests, RNG seed, timing. Supports dry-run via `--plan`. |
| `glint config` | View or mutate layered configuration. | `--get <key>`, `--set <key=value>`, `--unset <key>`, `--scope <project|workspace|global>`, `--json` | Writes changes back respecting precedence, emits diff preview. | `--json` returns change set. |
| `glint clean` | Remove caches, temporary artifacts, modules. | `--artifacts`, `--modules`, `--assets`, `--dry-run`, `--force` | Reports removed paths; optional NDJSON. | Honors workspace safety checks; requires confirmation unless `--force`. |
| `glint doctor` | Run health diagnostics. | `--project`, `--json`, `--fix` | NDJSON `checks` array plus optional fix actions. | Checks GPU availability, module ABI, schema cache freshness. |
| `glint modules` | Manage modular engine features. | Subcommands: `list`, `enable <name>`, `disable <name>`, `sync`, `status` | Updates `modules.lock`, prints status. | Validates dependency graph before enable; deterministic lockfile writes. |
| `glint assets` | Synchronize asset packs. | Subcommands: `sync`, `list`, `status` with flags `--pack <name>`, `--source <uri>` | Updates `assets.lock`, optional download manifest. | Integrates with templated asset packs defined in manifest. |

### 4.3 Structured Output Contract
- NDJSON payload keys use lower_snake_case.
- Each command emits a `summary` object on success and `error` object on failure.
- Example (`glint render --json`):
  ```json
  {"event":"render_started","scene":"shots/shot010.json","device":"auto"}
  {"event":"render_frame_completed","frame":1001,"duration_ms":412.6}
  {"event":"run_manifest_written","path":"renders/shot010/run.json"}
  {"event":"render_completed","status":"success","exit_code":0}
  ```

## 5. Exit Codes

| Exit Code | Value | Meaning | Applies To |
| --- | --- | --- | --- |
| `Success` | 0 | Command completed successfully. | All verbs |
| `SchemaValidationError` | 2 | Manifest or schema validation failed. | `validate`, `render`, `init` (when validating template) |
| `FileNotFound` | 3 | Referenced file/asset missing. | `render`, `validate`, `assets`, `modules` |
| `RuntimeError` | 4 | Unexpected runtime error (engine failure, provisioning issue). | All verbs |
| `UnknownFlag` | 5 | Unrecognized flag or invalid combination. | All verbs |
| `DependencyError` | 6 | Module/asset dependency violation. | `modules`, `assets`, `render` |
| `DeterminismError` | 7 | Run manifest or determinism checks failed. | `render`, `validate`, `doctor` |

All exit codes are stable across platforms and surfaced in `render_completed` summary when `--json` is supplied.

## 6. Open Questions & Follow Ups
- Confirm template catalog (`cinematic`, `product`, `blank`) and asset pack sources.
- Decide on default RNG seeding strategy for reproducible frame ordering.
- Define integration boundaries with future RPC daemon (shared schema packages).

---
Ownership: Platform Team - Milestone A (CLI Platform v1)

