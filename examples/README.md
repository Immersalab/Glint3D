# Glint3D Examples

This directory contains example files and configurations for Glint3D.

## JSON Ops v1.3 - Core Operations

The following operations are supported:

### Object Management
- **`load`** - Load a model from file with optional transform
- **`duplicate`** - Create a copy of an existing object with optional transform deltas
- **`remove`** / **`delete`** - Remove an object from the scene (aliased operations)
- **`select`** - Select an object for editing
- **`transform`** - Apply transformations to an object

### Camera Control
- **`set_camera`** - Set camera position, target, and lens parameters
- **`set_camera_preset`** - Apply predefined camera positions (front, back, left, right, top, bottom, iso_fl, iso_br)
- **`orbit_camera`** - Orbit the camera around a center point by yaw/pitch angles
- **`frame_object`** - Frame a specific object in the viewport

### Lighting
- **`add_light`** - Add point, directional, or spot lights to the scene

### Materials & Appearance
- **`set_material`** - Modify object material properties
- **`set_background`** - Set background color or skybox
- **`exposure`** - Adjust scene exposure
- **`tone_map`** - Configure tone mapping (linear, reinhard, filmic, aces)

### Rendering
- **`render_image`** - Render the scene to a PNG file

## Example Files

### New Core Operations (v1.3)
- **`duplicate-test.json`** - Demonstrates object duplication
- **`remove-objects-test.json`** - Shows object removal (both `remove` and `delete`)
- **`select-object-test.json`** - Object selection example
- **`camera-preset-test.json`** - Using camera presets
- **`orbit-camera-test.json`** - Orbiting camera around objects
- **`frame-object-test.json`** - Framing objects in viewport
- **`background-test.json`** - Setting background colors
- **`exposure-test.json`** - Exposure control
- **`tone-map-test.json`** - Tone mapping configuration
- **`comprehensive-new-ops-test.json`** - Multiple new operations combined

### Existing Examples
- **`three-point-lighting.json`** - Classic three-point lighting setup
- **`studio-turntable.json`** - Studio lighting with turntable animation
- **`directional-light-test.json`** - Directional lighting setup
- **`spot-light-test.json`** - Spot light configuration
- **`isometric-hero.json`** - Isometric camera setup for hero shots
- **`turntable-cli.json`** - Command-line turntable rendering setup

### Negative Tests
The `negative-tests/` directory contains examples of invalid JSON operations that should fail validation:
- **`duplicate-missing-source.json`** - Missing required 'source' field
- **`camera-preset-invalid-preset.json`** - Invalid preset name
- **`tone-map-invalid-type.json`** - Invalid tone mapping type
- And more validation failure cases

## Usage

### Command Line
```bash
# Desktop (Visual Studio build)
./builds/vs/x64/Release/glint.exe --ops examples/json-ops/comprehensive-new-ops-test.json --render output.png --w 1280 --h 720

# Desktop (CMake build)
./builds/desktop/cmake/glint --ops examples/json-ops/duplicate-test.json --render output.png

# With denoising (if OIDN available)
./builds/vs/x64/Release/glint.exe --ops examples/json-ops/studio-turntable.json --render output.png --denoise
```

### Schema Validation
All operations are validated against the schema in `schemas/json_ops_v1.json`. The schema includes:
- Required field validation
- Type checking for all parameters  
- Value range validation (e.g., FOV between 0.1-179.9 degrees)
- Enum validation for preset names and tone mapping types

### Error Messages
Operations return clear error messages pointing to the offending field:
- `"duplicate: missing 'source'"` - Missing required field
- `"set_camera_preset: unknown preset 'invalid_preset_name'"` - Invalid enum value
- `"tone_map: gamma must be > 0"` - Value out of range

## JSON Ops Documentation

For detailed information about JSON Operations syntax and available commands, see:
- `docs/json_ops_v1.md` - Documentation
- `schemas/json_ops_v1.json` - JSON schema v1.3

## Notes

- Some operations (exposure, tone_map, set_background) may require additional implementation in the RenderSystem class
- The `remove` operation is an alias for `delete` - both perform identical functionality  
- Camera preset names support both underscore and hyphen variants: `iso_fl` and `iso-fl`
- All vector parameters use 3-element arrays: `[x, y, z]`
- Rotation values are specified in degrees, not radians
