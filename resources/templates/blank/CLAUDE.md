# Glint CLI Agent Guide

## Quick Start
- Run `glint doctor` to verify environment health.
- Run `glint validate --project glint.project.json --strict` before renders or commits.
- Render: `glint render --project glint.project.json --scene shots/SHOT000.json` or `glint render --ops templates/cube_scene.json`.
- Use `--json` to get NDJSON output; parse line-by-line and check the final `command_completed` exit code.

## Core Commands
- `glint init [path] [--template <name>] [--with-samples] [--force] [--dry-run]` — scaffold a workspace (blank template includes starter scenes/assets).
- `glint doctor [--json]` — environment diagnostics.
- `glint validate --project glint.project.json [--strict] [--json]` — schema and asset validation.
- `glint render --project glint.project.json --scene <shot>` or `--ops <json>` — render with determinism logging.
- `glint modules list|enable|disable ...` — manage engine modules; updates `modules.lock`.
- `glint assets sync|list ...` — sync asset packs; updates `assets.lock`.
- `glint config --get/--set ...` — view/edit layered configuration.
- `glint clean --artifacts|--modules|--assets [--dry-run]` — remove caches with safety checks.

## Workspace Structure (after `glint init`)
```
workspace/
├─ glint.project.json        # Project manifest
├─ .glint/config.json        # Workspace config (defaults merged at runtime)
├─ assets/
│  ├─ assets.lock            # Asset pack manifest
│  ├─ models/ (cube.obj, cow.obj, sphere.obj)
│  └─ textures/ (cow-tex-fin.jpg)
├─ modules/modules.lock      # Module lockfile
├─ renders/                  # Outputs + run manifests
├─ shots/                    # Scene JSON ops (SHOT000.json)
└─ templates/                # Starter templates
   ├─ cube_scene.json        # Lit cube scene -> renders/cube.png
   ├─ cow_showcase.json      # Cow showcase -> renders/cow.png
   └─ README.md              # Template descriptions
```

## Starter Templates & Assets
- Templates use workspace-relative paths into `assets/models` and `assets/textures`.
- Run `glint render --ops templates/cube_scene.json` or `glint render --ops templates/cow_showcase.json` to get immediate renders.
- Assets are bundled from `resources/assets` so the templates work offline.

## Determinism & Safety
- Each render writes `renders/<name>/run.json` (args, schema hashes, module digests, seeds, timings).
- Do not edit `modules.lock` or `assets.lock` manually; use CLI verbs.
- Prefer `--dry-run` on init/clean if unsure; always check exit codes before proceeding.
