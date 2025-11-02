# Runtime Verification Report

Date: 2025-10-29  
Tooling: `builds/desktop/cmake/Debug/user_paths_probe.exe`

## Overview

The `user_paths_probe` diagnostic executable exercises the public helpers from `user_paths.h` without launching the full renderer. Each run prints a single JSON object describing the resolved directories, file helper outputs, and cache consistency for a given scenario.

Artifacts generated:

- `runtime_probe_results.ndjson` – three JSON lines (default, env-based portable, file-based portable).
- `artifacts/tests/runtime/runtime/` – temporary directories produced during portable-mode probing (kept for inspection).

## Test Commands

Executed from `ai/tasks/cross_platform_user_paths/artifacts/tests`:

```powershell
& 'D:\ahoqp1\Repositories\Glint3D\builds\desktop\cmake\Debug\user_paths_probe.exe' default | `
    Out-File -FilePath 'runtime_probe_results.ndjson' -Encoding utf8
& 'D:\ahoqp1\Repositories\Glint3D\builds\desktop\cmake\Debug\user_paths_probe.exe' env_portable | `
    Out-File -FilePath 'runtime_probe_results.ndjson' -Encoding utf8 -Append
& 'D:\ahoqp1\Repositories\Glint3D\builds\desktop\cmake\Debug\user_paths_probe.exe' file_portable | `
    Out-File -FilePath 'runtime_probe_results.ndjson' -Encoding utf8 -Append
```

## Key Findings

### Default Desktop Mode

- `getUserDataDir`, `getConfigDir`, and `getCacheDir` resolve to `%APPDATA%/Glint3D`, `%APPDATA%/Glint3D/config`, and `%LOCALAPPDATA%/Glint3D/Cache` respectively.
- All directories exist after the first call (`exists: true`).
- History and ImGui files already present in roaming profile were detected.
- Repeated calls returned identical paths (`consistent_calls: true`).

### Portable Mode via `GLINT_PORTABLE`

- Setting `GLINT_PORTABLE=1` forced all directories into the local `runtime/` tree.
- Helper functions created the tree and subsequent file helpers wrote sample files (`history.txt`, `recent.txt`, `imgui.ini`, `cache/tmp/probe.bin`) inside `runtime/`.
- `isPortableMode()` reported `true`, matching expectations.

### Portable Mode via Marker File

- Creating `runtime/.portable` with no environment variable produced the same paths as the env-var scenario.
- File helpers remained portable-aware; sample files persisted in the runtime tree.

## Acceptance Criteria Coverage

- **Directories auto-create** – Confirmed across all scenarios.
- **Path stability** – Verified by `consistent_calls` flag.
- **Portable mode triggers** – Both `GLINT_PORTABLE` and marker file paths tested.
- **Data/config/cache functions** – Practically exercised via helper calls and sample file writes.

Outstanding items requiring a full application run (not covered here):

- Desktop UI launch stability.
- ImGui layout persistence within the renderer context.
- Recent file command integration.

## Next Steps

1. Launch `glint.exe` in debug mode to verify runtime writes occur during normal workflows.
2. Replicate `user_paths_probe` on Linux/macOS runners to validate non-Windows code paths.
3. Expand automated regression suite to parse `runtime_probe_results.ndjson` and flag deviations.
