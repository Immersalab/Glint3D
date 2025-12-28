# Glint3D

A lightweight 3D rendering engine with dual rendering pipelines (OpenGL rasterization and CPU raytracing), JSON-driven automation, and headless rendering support.

![Glint3D Interface](docs/images/interface-overview.png)

## Features

- **Dual Rendering**: Real-time OpenGL rasterization and offline CPU raytracing with BVH acceleration
- **PBR Materials**: Physically-based rendering with metallic-roughness workflow
- **JSON Operations**: Scriptable scene manipulation for automation and testing
- **Headless Rendering**: Batch rendering and golden image validation
- **Asset Pipeline**: Support for OBJ, glTF, FBX, PLY, and other formats via Assimp
- **Lighting System**: Point, directional, and spot lights with full shader integration

![Rendering Comparison](docs/images/render-comparison.png)

## Quick Start

### Building

```powershell
# Generate build files
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build builds/desktop/cmake --config Release
```

### Basic Usage

```powershell
# Interactive UI
./builds/desktop/cmake/Release/glint.exe

# Headless rendering
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/basic.json --render output.png

# Raytraced rendering
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/glass-sphere.json --render output.png --raytrace
```

## Project Structure

```
engine/
├── core/              Core rendering and scene systems
├── modules/           Optional features (raytracing, post-processing)
└── platform/desktop/  Desktop UI and native integrations

resources/
├── shaders/           GLSL shader programs
└── assets/            Models, textures, and examples

examples/json-ops/     Sample automation scripts
tests/                 Unit tests, integration tests, and golden images
```

## JSON Operations

Automate scene manipulation with JSON operations:

```json
{
  "operations": [
    { "op": "load", "path": "model.obj" },
    { "op": "add_light", "type": "point", "position": [0, 5, 0] },
    { "op": "render", "output": "result.png", "width": 1920, "height": 1080 }
  ]
}
```

Full schema available in `schemas/json_ops_v1.json`.

## Documentation

- **Build Instructions**: See `CLAUDE.md` for detailed build configurations
- **JSON Ops Reference**: Check `schemas/json_ops_v1.json` and `examples/json-ops/`
- **Testing**: Run `tests/scripts/run_all_tests.sh` for comprehensive validation
- **Dependencies**: See `docs/external_dependencies.md` for third-party libraries

## Development

The engine is designed for modularity and cross-platform support:

- **BGFX Migration Planned**: Future multi-backend support (Vulkan/DirectX/Metal/OpenGL/WebGL)
- **Modular Architecture**: Separate core, modules, and platform-specific code
- **Security**: Path validation with `--asset-root` flag
- **CI/CD**: Automated golden image testing and cross-platform builds

## License

[Add license information]

## Contributing

[Add contribution guidelines]
