### `glint ui`

Launches the Glint UI bound to the active workspace so scenes/models/assets load automatically.

**Usage:**
```bash
glint ui [--workspace <path>]
```

**Behavior:**
- Auto-detects the active workspace (same resolution as other commands: CLI flag > env > manifest).
- Starts the UI with that workspace preloaded (scenes, assets, modules).
- Presents **Open Workspace** in-app to change workspaces and **Save Workspace** to persist changes; defaults to the detected one.
- Workspace state (objects, transforms, camera) is persisted to `<workspace>/.glint/workspace_state.json` and restored automatically when launching within that workspace.
- Legacy UI boot logic is removed; only workspace-aware boot is supported.

**Flags:**
| Flag | Description | Default |
|------|-------------|---------|
| `--workspace <path>` | Explicit workspace root containing `glint.project.json` | Auto-detect |

**Examples:**
```bash
# Launch UI with current workspace
glint ui

# Launch UI with a specific workspace
glint ui --workspace D:/shows/demo
```
