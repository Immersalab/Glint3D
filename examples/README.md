# Examples

This directory contains example files and configurations for Glint3D.

## JSON Ops Examples

The `json-ops/` directory contains sample JSON Operations files that demonstrate various rendering scenarios:

- **`three-point-lighting.json`** - Classic three-point lighting setup
- **`studio-turntable.json`** - Studio lighting with turntable animation
- **`isometric-hero.json`** - Isometric camera setup for hero shots
- **`turntable-cli.json`** - Command-line turntable rendering setup

## Usage

You can use these examples with the headless CLI:

```bash
# Desktop (Visual Studio build)
./builds/vs/x64/Release/glint3d.exe --ops examples/json-ops/three-point-lighting.json --render output.png --w 1280 --h 720

# Desktop (CMake build)
./builds/desktop/cmake/glint3d --ops examples/json-ops/studio-turntable.json --render output.png

# With denoising (if OIDN available)
./builds/vs/x64/Release/glint3d.exe --ops examples/json-ops/studio-turntable.json --render output.png --denoise
```

## JSON Ops Documentation

For detailed information about JSON Operations syntax and available commands, see:
- `docs/json_ops_v1.md` - Documentation
- `docs/json_ops_v1.json` - JSON schema