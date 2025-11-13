<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/artifacts/documentation/glint_cli_command_reference.md","purpose":"Complete reference documentation for Glint CLI commands, flags, and structured output.","exports":[],"depends_on":["cli/src/command_dispatcher.cpp","cli/src/commands/*.cpp"],"notes":["command_reference_v1","exit_code_table","json_output_envelope"]}
<!-- Human Summary -->
Authoritative command reference for the Glint CLI platform v1, documenting all verbs, flags, exit codes, and structured output formats.

# Glint CLI Command Reference v1.0

## Table of Contents

1. [Global Flags](#global-flags)
2. [Command Index](#command-index)
3. [Exit Codes](#exit-codes)
4. [Structured Output](#structured-output)
5. [Command Details](#command-details)

---

## Global Flags

Global flags can be specified before or after the command verb.

| Flag | Description | Type | Default |
|------|-------------|------|---------|
| `--verbosity <level>` | Set logging level | `quiet\|warn\|info\|debug` | `info` |
| `--project <path>` | Path to `glint.project.json` | path | Auto-detect |
| `--config <path>` | Path to `.glint/config.json` | path | `.glint/config.json` |

**Example:**
```bash
glint --verbosity debug render --input scene.obj --output out.png
glint render --input scene.obj --output out.png --verbosity warn
```

---

## Command Index

### Core Commands

| Command | Status | Description |
|---------|--------|-------------|
| `init` | ✓ Implemented | Initialize a new Glint project workspace |
| `render` | ✓ Implemented | Render scenes with determinism logging |
| `inspect` | ✓ Implemented | Inspect scenes, manifests, and project files |
| `validate` | ✓ Implemented | Validate project manifests and configurations |
| `clean` | ✓ Implemented | Remove build artifacts and caches |

### Management Commands

| Command | Status | Description |
|---------|--------|-------------|
| `modules` | ✓ Implemented | Manage engine modules (list/enable/disable) |
| `assets` | ✓ Implemented | Synchronize and manage asset packs |
| `config` | ✓ Implemented | View and edit configuration settings |
| `doctor` | ✓ Implemented | Diagnose environment and dependencies |

### Future Commands

| Command | Status | Description |
|---------|--------|-------------|
| `watch` | ⏸ Stub | Monitor files and trigger auto-renders |
| `profile` | ⏸ Stub | Capture detailed performance metrics |
| `convert` | ⏸ Stub | Convert assets between formats |

---

## Exit Codes

All Glint CLI commands return deterministic exit codes following this table:

| Code | Name | Description | Recovery Action |
|------|------|-------------|-----------------|
| **0** | `Success` | Command completed successfully | None (success) |
| **2** | `SchemaValidationError` | JSON schema validation failed | Fix manifest structure |
| **3** | `FileNotFound` | Required input file not found | Check file path |
| **4** | `RuntimeError` | General runtime error occurred | Check error message details |
| **5** | `UnknownFlag` | Unrecognized command-line flag | Use `--help` for valid flags |
| **6** | `DependencyError` | Missing required dependency | Run `glint doctor` |
| **7** | `DeterminismError` | Determinism validation failed | Check RNG seeds, input hashes |

### Exit Code Usage Examples

```bash
# Check exit code in shell
glint render --input scene.obj --output out.png
if [ $? -eq 0 ]; then
  echo "Render succeeded"
fi

# Use in scripts
glint validate project.json || { echo "Validation failed"; exit 1; }

# Structured output includes exit code
glint render --json --input scene.obj --output out.png > output.ndjson
# output.ndjson contains: {"event":"command_completed","exit_code":0,"exit_code_name":"Success"}
```

---

## Structured Output

All commands support `--json` flag for machine-readable NDJSON output.

### Output Envelope

Every NDJSON line follows this envelope structure:

```json
{
  "event": "command_started|command_completed|info|warning|error",
  "timestamp": "2025-11-12T00:00:00.000Z",
  "verb": "render",
  "status": "success|in_progress|error",
  "data": {}
}
```

### Event Types

| Event | When Emitted | Fields |
|-------|--------------|--------|
| `command_started` | Command execution begins | `verb`, `timestamp` |
| `command_completed` | Command execution ends | `verb`, `exit_code`, `exit_code_name`, `duration_ms` |
| `info` | Informational message | `message` |
| `warning` | Non-fatal warning | `message`, `code` |
| `error` | Fatal error | `message`, `code`, `exit_code` |

### Example NDJSON Output

```bash
$ glint render --json --input sphere.obj --output out.png
```

```json
{"event":"command_started","timestamp":"2025-11-12T14:30:00.000Z","verb":"render"}
{"event":"info","message":"Loading scene: sphere.obj"}
{"event":"info","message":"Rendering 1 frame at 800x600"}
{"event":"info","message":"Run manifest written to: renders/default/run.json"}
{"event":"command_completed","verb":"render","exit_code":0,"exit_code_name":"Success","duration_ms":1234.56}
```

### Parsing NDJSON in Scripts

**Bash:**
```bash
glint render --json --input scene.obj --output out.png | while IFS= read -r line; do
  event=$(echo "$line" | jq -r '.event')
  if [ "$event" == "error" ]; then
    echo "Error: $(echo "$line" | jq -r '.message')"
    exit 1
  fi
done
```

**Python:**
```python
import json
import subprocess

proc = subprocess.Popen(
    ['glint', 'render', '--json', '--input', 'scene.obj', '--output', 'out.png'],
    stdout=subprocess.PIPE,
    text=True
)

for line in proc.stdout:
    event = json.loads(line)
    if event['event'] == 'error':
        print(f"Error: {event['message']}")
        break
```

---

## Command Details

### `glint init`

Initialize a new Glint project workspace.

**Usage:**
```bash
glint init [OPTIONS] [directory]
```

**Options:**
| Flag | Description | Default |
|------|-------------|---------|
| `--template <name>` | Use template (basic\|pbr\|arch_viz) | `basic` |
| `--force` | Overwrite existing files | `false` |

**Examples:**
```bash
# Initialize in current directory
glint init

# Initialize with PBR template
glint init --template pbr my_project

# Force initialization in non-empty directory
glint init --force
```

**Output:**
- `glint.project.json` - Project manifest
- `.glint/config.json` - Local configuration
- `scenes/` - Example scenes
- `renders/` - Output directory
- `ops/` - JSON Ops examples

---

### `glint render`

Render scenes with deterministic provenance logging.

**Usage:**
```bash
glint render --input <scene> --output <image> [OPTIONS]
```

**Required Flags:**
| Flag | Description |
|------|-------------|
| `--input <path>` or `-i` | Input scene file (.obj, .glb, .gltf) |
| `--output <path>` or `-o` | Output image path (.png, .jpg, .exr) |

**Optional Flags:**
| Flag | Description | Default |
|------|-------------|---------|
| `--ops <path>` | JSON Ops file for scene manipulation | None |
| `--width <pixels>` or `-w` | Render width (1-16384) | `800` |
| `--height <pixels>` or `-h` | Render height (1-16384) | `600` |
| `--raytrace` | Use CPU raytracer (enables refraction) | `false` |
| `--denoise` | Apply AI denoising (requires OIDN) | `false` |
| `--name <string>` | Render name for output directory | `default` |
| `--no-manifest` | Skip run manifest generation | `false` |

**Examples:**
```bash
# Basic render
glint render --input scene.obj --output output.png

# High-resolution raytraced render
glint render -i glass.obj -o glass.png --width 1920 --height 1080 --raytrace --denoise

# Apply JSON Ops and render
glint render --ops operations.json --output result.png --name turntable_001
```

**Output Artifacts:**
- `<output_path>` - Rendered image
- `renders/<name>/run.json` - Determinism manifest
- `renders/<name>/run.json.sha256` - Checksum file

---

### `glint inspect`

Inspect scenes, manifests, and project files.

**Usage:**
```bash
glint inspect <target> [OPTIONS]
```

**Supported Targets:**
- Scene files: `.obj`, `.glb`, `.gltf`, `.fbx`, `.ply`, `.stl`
- Project manifest: `glint.project.json`
- Run manifest: `renders/<name>/run.json`

**Options:**
| Flag | Description |
|------|-------------|
| `--verbose` or `-v` | Show detailed metadata |

**Examples:**
```bash
# Inspect scene file
glint inspect models/sphere.obj

# Inspect project manifest
glint inspect glint.project.json --verbose

# Inspect run manifest
glint inspect renders/output_001/run.json
```

---

### `glint validate`

Validate project manifests and configurations.

**Usage:**
```bash
glint validate [target] [OPTIONS]
```

**Examples:**
```bash
# Validate current project
glint validate

# Validate specific manifest
glint validate glint.project.json

# Validate with strict schema checking
glint validate --strict
```

---

### `glint clean`

Remove build artifacts, caches, and temporary files.

**Usage:**
```bash
glint clean [OPTIONS]
```

**Options:**
| Flag | Description |
|------|-------------|
| `--renders` | Clean renders/ directory only |
| `--cache` | Clean .glint/cache/ only |
| `--all` | Clean all artifacts (default) |
| `--dry-run` or `-n` | Show what would be removed without deleting |
| `--verbose` or `-v` | Show detailed deletion report |

**Examples:**
```bash
# Clean everything (dry run)
glint clean --dry-run

# Clean only render outputs
glint clean --renders

# Clean all with verbose output
glint clean --all --verbose
```

---

### `glint modules`

Manage engine modules.

**Usage:**
```bash
glint modules <subcommand>
```

**Subcommands:**
- `list` - List all available modules
- `enable <name>` - Enable a module
- `disable <name>` - Disable a module

**Examples:**
```bash
# List modules and their status
glint modules list

# Enable raytracing module
glint modules enable raytracing

# Disable gizmos module
glint modules disable gizmos
```

---

### `glint assets`

Synchronize and manage asset packs.

**Usage:**
```bash
glint assets <subcommand>
```

**Subcommands:**
- `sync` - Synchronize assets with project manifest
- `list` - List installed asset packs
- `add <name>` - Add an asset pack
- `remove <name>` - Remove an asset pack

**Examples:**
```bash
# Sync assets
glint assets sync

# List installed assets
glint assets list
```

---

### `glint config`

View and edit configuration settings.

**Usage:**
```bash
glint config <key> [value]
```

**Examples:**
```bash
# View all config
glint config

# Get specific value
glint config render.default_width

# Set value
glint config render.default_width 1920
```

---

### `glint doctor`

Diagnose environment and dependencies.

**Usage:**
```bash
glint doctor
```

**Checks:**
- Engine version and modules
- Required dependencies (OpenGL, Assimp, OIDN)
- Project manifest validity
- File permissions and paths
- Configuration conflicts

**Example Output:**
```
Glint Doctor v1.0
==================

✓ Engine version: 0.3.0
✓ OpenGL: 4.6 (NVIDIA GeForce RTX 3080)
⚠ Assimp: Not found (glTF/FBX support disabled)
✓ Project manifest: Valid
✓ Configuration: No conflicts

1 warning, 0 errors
```

---

### Stub Commands

The following commands are currently stubs and will return `RuntimeError` (exit code 4) with guidance on workarounds:

#### `glint watch`

Monitor files and trigger auto-renders (not yet implemented).

**Workaround:**
```bash
# Use watchexec (https://github.com/watchexec/watchexec)
watchexec -w scenes/ -w ops/ -- glint render --ops ops/main.json --output out.png
```

#### `glint profile`

Capture detailed performance metrics (not yet implemented).

**Workaround:**
- Use external profilers: Visual Studio Profiler, Intel VTune, Tracy
- Check `run.json` for basic timing: `frames[].duration_ms`

#### `glint convert`

Convert assets between formats (not yet implemented).

**Workaround:**
- Use Blender CLI for model conversion
- Use ImageMagick for texture conversion

---

## Configuration Precedence

Configuration values are resolved in this order (highest to lowest priority):

1. **CLI flags** - `--verbosity debug`
2. **Project manifest** - `glint.project.json`
3. **Local config** - `.glint/config.json`
4. **Environment variables** - `GLINT_VERBOSITY=debug`
5. **Global config** - `~/.glint/config.json`
6. **Built-in defaults**

---

## Environment Variables

| Variable | Description | Example |
|----------|-------------|---------|
| `GLINT_VERBOSITY` | Default log level | `debug` |
| `GLINT_PROJECT` | Default project path | `/path/to/project` |
| `GLINT_CONFIG` | Global config path | `~/.glint/config.json` |

---

*End of document.*
