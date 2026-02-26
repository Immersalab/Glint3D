<!-- Machine Summary Block -->
{"file":"apps/qem_simplifier/README.md","purpose":"Documents the QEM tool vertical-slice scaffold and how to build/test it before real QEM implementation.","exports":["build_instructions","scope","testing_flow"],"depends_on":["apps/qem_simplifier/CMakeLists.txt","apps/qem_simplifier/docs/BACKEND_INTERFACE_DISCUSSION.md"],"notes":["external_ui_first","stubbed_qem_core","cli_render_bridge"]}
<!-- Human Summary -->
This folder contains a buildable vertical slice for the external QEM tool architecture: stubbed QEM core API, backend interfaces, a Glint CLI render bridge, and a minimal shell UI to test the flow early.

# QEM Tool Scaffold (Vertical Slice)

## What this is

- `qem_core` API headers and a baseline QEM implementation (correctness-first, deterministic).
- Backend interfaces designed to be plugin-ABI-friendly later.
- `glint_render_bridge` CLI backend (builds and executes `glint` commands).
- Minimal external UI shell (`glint_qem_studio_shell`) for interactive testing.

## What this is not (yet)

- Real QEM edge-collapse implementation.
- Runtime plugin loader/ABI host.
- GUI framework integration.

## Build (local scripts)

Windows:

```bat
apps\qem_simplifier\build-and-run.bat debug smoke
apps\qem_simplifier\build-and-run.bat release run
```

Supported modes: `configure`, `build`, `run`, `smoke`, `clean`

Note: a Unix shell helper script is not currently included in `apps/qem_simplifier`.

## Build (standalone)

```powershell
cmake -S apps/qem_simplifier -B builds/qem_simplifier/cmake
cmake --build builds/qem_simplifier/cmake --config Release
```

## Run

```powershell
./builds/qem_simplifier/cmake/Release/glint_qem_studio_shell.exe
./builds/qem_simplifier/cmake/Release/glint_qem_studio_shell.exe --config apps/qem_simplifier/qem_shell.config
```

## Quick shell commands

- `help`
- `?` (alias for `help`)
- `status`
- `st` (alias for `status`)
- `config show`
- `cfg show` (alias for `config show`)
- `config load [path]`
- `config save [path]`
- `set glint <path-to-glint.exe>`
- `set outdir <folder>`
- `set mesh <path-to-mesh>`
- `set preset <path-to-preview-preset.ops.json>`
- `set display <default|solid|wireframe|points|wireframe_overlay>`
- `simplify` (runs QEM, saves `qem_simplified.obj`, and prints progress/stats)
- `render` (stages mesh + composed JsonOps, auto-appends preset if present, then renders)
- `renderqem` (renders the saved simplified mesh)
- `open` (stages mesh + composed JsonOps, launches `glint ui --ops <staged_ops>`)
- `openqem` (opens the saved simplified mesh in Glint UI)
- `savecmp` (saves compare JsonOps for original + simplified side-by-side view)
- `rendercmp` (renders side-by-side compare view)
- `opencmp` (opens side-by-side compare view in Glint UI)
- `quit`

## Minimal config file workflow (recommended)

1. Edit `apps/qem_simplifier/qem_shell.config`:
   - `glint_executable`
   - `mesh_path` (for example `resources/models/cow.obj`)
   - `output_dir` (recommended: `apps/qem_simplifier/output`)
   - `preview_preset_ops` (recommended: `apps/qem_simplifier/output/preview_preset.ops.json`)
   - `preview_display_mode` (`wireframe_overlay` is the most useful for QEM previews)
   - On Windows, use backslashes for `glint_executable` (example: `builds\\desktop\\cmake\\Debug\\glint.exe`)
2. Run the shell with `--config apps/qem_simplifier/qem_shell.config`
3. In the shell, use:
   - `config show`
   - `render` (actual render)

## Preview preset workflow (camera + lights)

1. From the QEM shell, run `open` to open Glint with the staged mesh.
2. In Glint UI, position the camera and lights.
3. Export a preset from Glint:
   - File menu: `Export Preview Preset...`
   - or console: `export_preview_ops apps/qem_simplifier/output/preview_preset.ops.json`
4. Back in the QEM shell, run `render`.
5. After `simplify`, you can run `savecmp`, `rendercmp`, or `opencmp` to inspect original vs simplified side by side.

If `preview_preset_ops` exists, `render` automatically appends its `ops` (camera/lights) after the generated `load` op.

`preview_display_mode` is a QEM-side enum-style setting (`default|solid|wireframe|points|wireframe_overlay`) that backends map to renderer-specific controls. The current Glint backend maps `wireframe_overlay` to Glint's selected-object wireframe overlay (via staged `select` + `ops --selection-overlay`), and the other modes to JsonOps `set_render_mode`.

![HUD Wireframe](../../resources/assets/img/HUD-Wireframe.png)

## Design notes

See `docs/BACKEND_INTERFACE_DISCUSSION.md` for why backend interfaces come before a runtime plugin ABI and how to keep the current scaffolding compatible with a future true plugin interface.


