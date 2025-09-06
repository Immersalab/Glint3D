# Help System Update - JSON Operations v1.3

This document summarizes the updates made to the Glint3D help system to include all current JSON Operations v1.3 keywords and functionality.

## Updated Help Systems

### 1. Command-Line Help (`--help`)
- **Location**: `engine/src/main.cpp` - `print_help()` function
- **Updates**: 
  - Updated JSON Ops version from v1 to v1.3
  - Added comprehensive operation categories
  - Included practical usage examples
  - Added documentation references

**New Content:**
```
JSON Operations v1.3 (Core Operations):
  Object:     load, duplicate, remove/delete, select, transform
  Camera:     set_camera, set_camera_preset, orbit_camera, frame_object
  Lighting:   add_light (point/directional/spot)
  Materials:  set_material, set_background, exposure, tone_map
  Rendering:  render_image
```

### 2. Console Help Commands
- **Location**: `engine/src/ui_bridge.cpp` - `handleConsoleCommand()` function
- **Updates**:
  - Enhanced existing `help` command
  - Added new `json_ops` command for detailed operation reference
  - Categorized operations by functionality

**New Commands:**
- `help` - Shows console commands and mentions `json_ops`
- `json_ops` - Shows detailed JSON Operations v1.3 reference with categories

### 3. ImGui Help Menu & Dialogs
- **Location**: `engine/src/imgui_ui_layer.cpp` and `engine/include/imgui_ui_layer.h`
- **Updates**:
  - Added three help dialogs: Controls, JSON Operations, and About
  - Implemented interactive help system with collapsible sections
  - Added comprehensive JSON Operations reference with examples

**New Help Dialogs:**
- **Controls Help** (F1) - Camera movement, UI controls, object interaction
- **JSON Operations** - Complete v1.3 reference with categorized operations and examples
- **About Glint3D** - Engine information, features, and technology stack

## Coverage of JSON Operations v1.3

All core operations are now documented across all help systems:

### Object Management
- `load` - Load models from file with optional transforms
- `duplicate` - Create copies with position/scale/rotation offsets  
- `remove`/`delete` - Remove objects (aliased operations)
- `select` - Select objects for editing
- `transform` - Apply transformations

### Camera Control
- `set_camera` - Set position, target, lens parameters
- `set_camera_preset` - Apply predefined views (front, back, left, right, top, bottom, iso_fl, iso_br)
- `orbit_camera` - Rotate camera around center by yaw/pitch
- `frame_object` - Frame specific objects in viewport

### Lighting
- `add_light` - Add point/directional/spot lights with full parameter support

### Materials & Appearance
- `set_material` - Modify object material properties
- `set_background` - Set background color or skybox
- `exposure` - Adjust scene exposure
- `tone_map` - Configure tone mapping (linear/reinhard/filmic/aces)

### Rendering
- `render_image` - Render scene to PNG with size control

## Implementation Details

### Schema Validation
- Updated to JSON Ops schema v1.3
- All operations validated with clear error messages
- Proper field validation and type checking

### Error Handling
- Clear error messages pointing to specific field issues
- Example: `"duplicate: missing 'source'"`, `"tone_map: gamma must be > 0"`

### Documentation Integration
- References to `examples/README.md` for detailed guides
- Links to `schemas/json_ops_v1.json` for validation
- Examples directory with comprehensive test files

## Usage Examples

### Command Line
```bash
# Get help
glint --help

# Get JSON ops reference
glint --ops examples/json-ops/duplicate-test.json --render output.png
```

### Console (In-App)
```
> help
> json_ops
```

### UI Menu
- **Help > Controls** - Shows camera and UI controls
- **Help > JSON Operations** - Complete operations reference
- **Help > About Glint3D** - Engine information

## Benefits

1. **Comprehensive Coverage** - All JSON Operations v1.3 keywords documented
2. **Multiple Access Points** - CLI, console, and GUI help systems
3. **Categorized Information** - Operations grouped by functionality
4. **Practical Examples** - Real usage examples with proper syntax
5. **Interactive Reference** - Collapsible sections in GUI for easy navigation
6. **Schema Integration** - Direct references to validation schemas

The help system now provides complete coverage of all JSON Operations v1.3 functionality across all user interfaces, making it easy for developers to discover and use the full power of the Glint3D engine.