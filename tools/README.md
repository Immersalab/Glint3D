# Glint3D Tools Directory

This directory contains build scripts, testing tools, and utilities for the Glint3D project.

## Build Scripts

### `build-and-run.sh` / `build-and-run.bat`
Primary build script for desktop applications.
- Cross-platform CMake compilation
- Debug and Release build modes  
- Usage: `./build-and-run.sh [debug|release] [args...]`

### `build-web.sh` / `build-web.bat` 
Complete web build pipeline (WASM engine + React frontend).
- Emscripten WASM compilation
- Asset copying and package building
- Usage: `./tools/build-web.sh [Debug|Release]` or `npm run web:build`

### `create_basic_ico.py`, `png_to_ico.cpp`
Asset management tools for Windows icon generation.

### `dev_web_ui.ps1`, `texc.sh/.ps1`
Development and texture compression utilities.

---

## Golden Image Testing Tools

Automated visual regression testing using golden reference images.

## Overview

The golden image testing system compares rendered outputs against reference images using:
- **SSIM (Structural Similarity Index)** for perceptual similarity
- **Per-channel pixel difference** for exact matching
- **Heatmap generation** for visualizing differences
- **Diff images** showing side-by-side comparisons

## Acceptance Criteria

- **Desktop**: SSIM ≥ 0.995 OR per-channel Δ ≤ 2 LSB
- **Web vs Desktop**: SSIM ≥ 0.990
- **CI artifacts**: Diff images + heatmaps uploaded on failure

## Tools

### `golden_image_compare.py`
Main comparison tool that validates rendered images against golden references.

#### Dependencies
```bash
pip install -r requirements.txt
```

#### Usage
```bash
# Compare single image pair
python golden_image_compare.py rendered.png golden.png

# Batch compare directories
python golden_image_compare.py --batch renders/ goldens/ --output results/

# Web vs desktop comparison with JSON output
python golden_image_compare.py rendered.png golden.png \
  --type web_vs_desktop --json-output results.json --verbose
```

#### Output Artifacts
On comparison failure, generates:
- **Diff image**: Side-by-side comparison with enhanced differences
- **SSIM heatmap**: Visualization of similarity across the image
- **JSON report**: Detailed metrics and file paths

### `generate_goldens.py`
Creates golden reference images by rendering test scenes.

#### Usage
```bash
# Generate goldens using Release build
python generate_goldens.py builds/desktop/cmake/Release/glint.exe

# Generate with custom settings
python generate_goldens.py builds/desktop/cmake/Debug/glint.exe \
  --asset-root engine/assets --width 512 --height 384 \
  --summary generation_report.json
```

## Test Scene Structure

Test scenes are located in `examples/json-ops/golden-tests/`:

- `basic-lighting.json` - Point light with sphere
- `directional-lighting.json` - Sun-like directional light  
- `spot-lighting.json` - Focused cone light with falloff
- `camera-presets.json` - Camera positioning consistency
- `tone-mapping.json` - Exposure and tone mapping controls

## Golden Image Management

### Initial Generation
```bash
# Build the engine first
cmake --build builds/desktop/cmake --config Release

# Generate initial golden set
python tools/generate_goldens.py builds/desktop/cmake/Release/glint.exe \
  --asset-root engine/assets --golden-dir examples/golden
```

### Updating Goldens
When rendering changes are intentional:

1. **Verify changes**: Run comparison to confirm expected differences
2. **Regenerate**: Use `generate_goldens.py` to create new references
3. **Validate**: Run full test suite to ensure no regressions
4. **Commit**: Add updated golden images to repository

### CI Integration
The golden testing system integrates with CI via:
- Automated rendering of test scenes
- SSIM comparison against references
- Artifact upload on failures
- Pass/fail status for build pipeline

## Troubleshooting

### Common Issues

**"Module not found" errors**
```bash
pip install -r tools/requirements.txt
```

**Platform rendering differences**  
Golden images may differ slightly between platforms. Use appropriate thresholds:
- Same platform: SSIM ≥ 0.995
- Cross-platform: SSIM ≥ 0.990

**Missing assets**
Ensure `--asset-root` points to directory containing required models:
- `models/sphere.obj`
- `models/cube.obj` 
- `models/plane.obj`

**Render failures**
Check engine logs and verify:
- Scene JSON syntax is valid
- All referenced assets exist
- Engine binary is correct version
- Sufficient disk space for outputs

### Debug Mode
Use `--verbose` flag for detailed output:
```bash
python golden_image_compare.py rendered.png golden.png --verbose
```

## Development

### Adding New Test Scenes

1. Create JSON scene in `examples/json-ops/golden-tests/`
2. Run `generate_goldens.py` to create reference
3. Add to CI workflow if needed
4. Document expected visual behavior

### Modifying Thresholds
Update acceptance criteria in `golden_image_compare.py`:
```python
DESKTOP_SSIM_THRESHOLD = 0.995
DESKTOP_CHANNEL_DIFF_THRESHOLD = 2
WEB_VS_DESKTOP_SSIM_THRESHOLD = 0.990
```

### Custom Metrics
Extend `ImageComparisonResult` class to add new comparison metrics or validation criteria.