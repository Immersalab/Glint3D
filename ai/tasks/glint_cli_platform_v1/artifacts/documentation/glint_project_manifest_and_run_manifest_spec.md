<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/artifacts/documentation/glint_project_manifest_and_run_manifest_spec.md","purpose":"Defines the glint.project.json schema and run manifest payload for deterministic CLI operations.","exports":["project_manifest_schema","run_manifest_contract"],"depends_on":["ai/tasks/glint_cli_platform_v1/task.json","schemas/json_ops_v1.json"],"notes":["cli_manifest_spec","determinism_contract","workspace_layering"]}
<!-- Human Summary -->
Spec describing the project manifest fields, configuration precedence hooks, and the renders/<name>/run.json determinism payload expected from Glint CLI v1.

# Glint Project Manifest & Run Manifest Specification

## 1. Overview
- `glint.project.json` anchors every CLI workspace; it lists project metadata, scene bundles, module selections, and asset packs.
- `renders/<name>/run.json` captures reproducibility data for each render invocation.
- Both files are versioned via `schema_version` (semantic, e.g., `"1.0.0"`), enabling forward-compatible validation.

## 2. Project Manifest (`glint.project.json`)

### 2.1 Required Structure
```json
{
  "schema_version": "1.0.0",
  "project": {
    "name": "Example Project",
    "slug": "example_project",
    "version": "0.1.0",
    "description": "Short blurb for humans.",
    "default_template": "cinematic"
  },
  "workspace": {
    "root": ".",
    "assets_dir": "assets",
    "renders_dir": "renders",
    "modules_dir": "modules",
    "config_dir": ".glint"
  },
  "engine": {
    "core_version": "3.0.0",
    "modules": ["core", "raytracing"],
    "requires_gpu": false
  },
  "scenes": [
    {
      "id": "shot010",
      "path": "shots/shot010.json",
      "thumbnail": "shots/shot010.png",
      "default_output": "renders/shot010"
    }
  ],
  "assets": [
    {
      "pack": "studio-core",
      "version": "1.2.0",
      "source": "https://artifacts.example.com/packs/studio-core.zip",
      "hash": "sha256:...",
      "optional": false
    }
  ],
  "modules": [
    {
      "name": "raytracing",
      "enabled": true,
      "optional": true,
      "depends_on": ["core"]
    }
  ],
  "configuration": {
    "defaults": {
      "render.device": "auto",
      "render.samples": 64
    },
    "overrides": {
      "SHOT010": {
        "render.samples": 128
      }
    }
  },
  "determinism": {
    "rng_seed": 123456789,
    "lockfiles": {
      "modules": "modules.lock",
      "assets": "assets.lock"
    },
    "provenance": {
      "capture": true,
      "artifacts": ["renders/<name>/run.json"]
    }
  }
}
```

### 2.2 Field Reference

| Section | Field | Type | Required | Notes |
| --- | --- | --- | --- | --- |
| root | `schema_version` | string | Yes | Semantic version of this manifest schema. Used for validation. |
| `project` | `name` | string | Yes | Human-readable project title. |
|  | `slug` | string | Yes | Filesystem-safe identifier; used in default output names. |
|  | `version` | string | Yes | Project semantic version. |
|  | `description` | string | No | Optional summary for humans. |
|  | `default_template` | string | No | Template key used when scaffolding new scenes. |
| `workspace` | `root` | string | Yes | Relative path of the workspace root; defaults to `"."`. |
|  | `assets_dir` | string | Yes | Relative location of assets. |
|  | `renders_dir` | string | Yes | Relative location for render outputs. |
|  | `modules_dir` | string | Yes | Directory storing module payloads. |
|  | `config_dir` | string | Yes | Directory that contains `.glint/config.json` and caches. |
| `engine` | `core_version` | string | Yes | Expected Glint engine semantic version; used for compatibility checks. |
|  | `modules` | array[string] | Yes | Baseline modules required to run the project. |
|  | `requires_gpu` | boolean | No | Flags if GPU is mandatory for renders. |
| `scenes` | array | Yes | Each entry describes a renderable scene bundle. |
| `assets` | array | No | Declares asset packs needed by the project. |
| `modules` | array | No | Module overrides beyond the baseline engine list; aligned with `modules.lock`. |
| `configuration` | object | No | Project-level defaults and overrides resolved before `.glint/config.json`. |
| `determinism` | object | Yes | Declares how determinism is enforced. |

### 2.3 Scene Entry
- `id` (string, required): unique within the manifest.
- `path` (string, required): relative path to a scene descriptor or JSON ops file.
- `thumbnail` (string, optional): preview image path.
- `default_output` (string, optional): suggested renders directory.

### 2.4 Asset Pack Entry
- `pack` (string, required): canonical asset pack name.
- `version` (string, required): version pinned via `assets.lock`.
- `source` (uri|string, required): download location or local registry reference.
- `hash` (string, optional but recommended): checksum for integrity validation.
- `optional` (bool, default false): toggles whether missing pack blocks validation.

### 2.5 Module Entry
- `name` (string, required): module identifier.
- `enabled` (bool, default true): whether CLI should enable module on init.
- `optional` (bool, default false): gating indicator for optional modules.
- `depends_on` (array[string], optional): ensures dependency order and validated by `glint modules`.
- `abi` (string, optional): ABI compatibility tag (e.g., `"desktop-v2"`).

### 2.6 Configuration Resolution
Resolution order (highest priority first):
1. CLI flag (`--set render.samples=256`)
2. Command context (`glint render --frames`)
3. Project overrides per scene (`configuration.overrides[SCENE_ID]`)
4. `configuration.defaults` from `glint.project.json`
5. Workspace `.glint/config.json`
6. Environment variables (`GLINT_*`)
7. Global config (`%APPDATA%/Glint3D/config.json` or `$XDG_CONFIG_HOME/glint/config.json`)

### 2.7 Determinism Block
- `rng_seed`: default RNG seed when CLI call omits `--seed`; ensures deterministic sample ordering.
- `lockfiles.modules` / `lockfiles.assets`: relative lockfile locations; CLI updates them deterministically (sorted entries, LF endings).
- `provenance.capture`: if false, CLI warns when provenance is disabled.
- `provenance.artifacts`: list containing `"renders/<name>/run.json"` plus optional custom payloads.

### 2.8 Validation Rules
- Unknown top-level keys cause `SchemaValidationError` in `glint validate --strict`.
- `modules` array must not shadow entries in `engine.modules` unless `enabled` differs; CLI merges using module name as key.
- Asset packs flagged `optional: false` must resolve and verify hash.
- `workspace` directories must be relative; absolute paths rejected to maintain portability.

## 3. Run Manifest (`renders/<name>/run.json`)

### 3.1 Purpose
Documents every render invocation for reproducibility: inputs, environment probes, module/asset digests, and timing metrics.

### 3.2 Required Structure
```json
{
  "schema_version": "1.0.0",
  "project": {
    "manifest_path": "glint.project.json",
    "name": "Example Project",
    "version": "0.1.0",
    "scene_id": "shot010"
  },
  "command": {
    "verb": "render",
    "timestamp_utc": "2025-11-05T21:35:12.482Z",
    "argv": ["glint", "render", "--project", "glint.project.json", "--scene", "shot010"],
    "flags": {
      "project": "glint.project.json",
      "scene": "shot010",
      "json": true,
      "headless": true
    },
    "exit_code": 0,
    "duration_ms": 4231.7
  },
  "engine": {
    "core_version": "3.0.0",
    "modules": [
      {"name": "core", "version": "3.0.0", "hash": "sha256:..."},
      {"name": "raytracing", "version": "2.1.0", "hash": "sha256:..."}
    ],
    "schema_versions": {
      "json_ops": "1.2.0",
      "project_manifest": "1.0.0"
    }
  },
  "environment": {
    "hostname": "build-agent-42",
    "os": "Windows 11.0.22631",
    "cpu": "AMD Ryzen 9 7950X",
    "gpu": [
      {"name": "NVIDIA RTX 4090", "driver": "551.23", "cuda": "12.3"}
    ],
    "ram_gb": 128,
    "timezone": "America/New_York"
  },
  "settings": {
    "seed": 123456789,
    "device": "gpu",
    "resolution": {"width": 2048, "height": 858},
    "samples": 128,
    "tone_mapping": {"curve": "aces", "exposure": 0.5, "gamma": 2.2}
  },
  "outputs": {
    "primary": "renders/shot010/output.png",
    "auxiliary": [
      {"kind": "depth", "path": "renders/shot010/output_depth.exr"},
      {"kind": "normals", "path": "renders/shot010/output_normals.exr"}
    ],
    "checksums": [
      {"path": "renders/shot010/output.png", "hash": "sha256:..."}
    ]
  },
  "warnings": [
    "Module 'denoiser' disabled: GPU CUDA version mismatch."
  ]
}
```

### 3.3 Field Reference
- `command.timestamp_utc`: ISO 8601 w/ milliseconds UTC.
- `command.flags`: canonical map of parsed flags; booleans for toggles.
- `engine.modules[*].hash`: deterministic SHA-256 of module payload (binary + manifest).
- `environment.gpu`: ordered by detection priority; include driver/cuda when present.
- `settings.seed`: resolved seed (explicit flag or manifest default).
- `outputs.checksums`: at least SHA-256 for each produced artifact. CLI writes actual values post-render.
- `warnings`: array of human-readable warnings; empty array if none.

### 3.4 Determinism Guarantees
- File is written atomically via temp file rename.
- Keys serialized in alphabetical order when using `--json` deterministic mode.
- Floating-point values (e.g., durations) formatted with 3 decimal places.
- Arrays maintain lexical order: modules sorted by `name`, auxiliary outputs sorted by `kind`.
- CLI must refuse to overwrite an existing `run.json` unless `--resume` is provided; on resume, append `"resumed_from":"<timestamp>"` to `command`.

### 3.5 Validation Rules
- Missing `schema_version`, `project`, `command`, `engine`, `environment`, or `settings` causes `DeterminismError`.
- `command.exit_code` must match process exit status.
- `settings.seed` must be numeric; CLI verifies and normalizes.
- Checksums required if output file exists; absence triggers warning.

## 4. Relationship with Schemas
- JSON Schema definitions will live under `schemas/glint_project_manifest_v1.json` and `schemas/run_manifest_v1.json` in a follow-up code task.
- `glint validate --project` loads the manifest, validates against schema, and ensures lockfiles and asset packs align.
- Run manifest validation occurs during `glint render` and `glint doctor --json`.

## 5. Open Decisions
- Finalize GPU capability taxonomy (Vulkan/DirectX metadata) for `environment.gpu`.
- Decide on optional `"pipeline"` block capturing upstream job IDs for integration with studios.
- Determine retention policy for archived run manifests (rotation vs. archival bucket).

---
Ownership: Platform Team - Applies to Milestone A (CLI Platform v1)
