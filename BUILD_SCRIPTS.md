# Build and Launch Scripts

Convenient scripts to build and run Glint3D with a single command.

## Windows (Batch Script)

### Usage
```cmd
# Launch UI (Debug build)
build-and-run.bat

# Launch UI (Release build)
build-and-run.bat release

# Run with arguments (Debug)
build-and-run.bat debug --help
build-and-run.bat --ops examples/json-ops/cube-basic.json --render test.png

# Run with arguments (Release)
build-and-run.bat release --ops examples/json-ops/cube-basic.json --render test.png --w 800 --h 600
```

## Cross-Platform (Bash Script)

### Usage
```bash
# Make executable (first time)
chmod +x build-and-run.sh

# Launch UI (Debug build)
./build-and-run.sh

# Launch UI (Release build)  
./build-and-run.sh release

# Run with arguments (Debug)
./build-and-run.sh debug --help
./build-and-run.sh --ops examples/json-ops/cube-basic.json --render test.png

# Run with arguments (Release)
./build-and-run.sh release --ops examples/json-ops/cube-basic.json --render test.png --w 800 --h 600
```

## What the Scripts Do

1. **Generate CMake Project Files** - Creates/updates Visual Studio projects in `builds/desktop/cmake/`
2. **Build** - Compiles the project using CMake (automatically detects VS/make/ninja)
3. **Launch** - Runs the executable with any provided arguments

## Script Features

- **Configuration Selection**: Choose Debug or Release build
- **Argument Passing**: All arguments after config are passed to the application
- **Error Handling**: Stops on build failures with clear error messages
- **Cross-Platform**: Bash script works on Windows (Git Bash/MSYS2), Linux, and macOS
- **Automatic Paths**: Correctly handles different executable locations per platform

## Examples

```bash
# Quick UI launch
./build-and-run.sh

# Test unified file dialog
./build-and-run.sh debug

# Render a scene headlessly
./build-and-run.sh release --ops examples/json-ops/pbr-sphere-grid.json --render output.png --w 1920 --h 1080

# Run with strict JSON validation
./build-and-run.sh --ops scene.json --strict-schema --log debug
```

## Manual Build Alternative

If you prefer manual control:

```bash
# Generate project files
cmake -S . -B builds/desktop/cmake

# Build (choose config)
cmake --build builds/desktop/cmake --config Debug -j
cmake --build builds/desktop/cmake --config Release -j

# Run
./builds/desktop/cmake/Debug/glint.exe
./builds/desktop/cmake/Release/glint.exe
```

---

*These scripts are the recommended way to build and test Glint3D during development.*