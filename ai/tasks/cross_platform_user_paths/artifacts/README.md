# Artifacts - Cross-Platform User Data Directory Management

This directory contains all artifacts generated during the implementation of the cross-platform user data directory management system.

## Overview

The cross-platform user paths system provides OS-appropriate storage locations for:
- **User Data**: Persistent application data (history, recent files)
- **Config**: Application settings and preferences
- **Cache**: Temporary/expendable data (thumbnails, shader cache)

It follows platform conventions:
- **Windows**: Known Folders API (APPDATA, LOCALAPPDATA)
- **Linux**: XDG Base Directory Specification
- **macOS**: Apple Library paths

## Artifact Structure

### code/
Source code implementations and modifications:
- `engine/include/user_paths.h` - Public API header
- `engine/src/user_paths.cpp` - Implementation
- `CMakeLists.txt` modifications for build integration

### tests/
Testing documentation and results:
- Runtime verification results
- Portable mode test results
- Cross-platform compatibility results
- Edge case testing logs

### documentation/
Generated documentation:
- `API_REFERENCE.md` - Complete API documentation
- `PLATFORM_BEHAVIOR.md` - OS-specific behavior details
- `PORTABLE_MODE_GUIDE.md` - Portable mode setup and usage
- Usage examples and integration guides

### validation/
Validation artifacts:
- Build logs
- Test execution results
- Platform-specific validation reports
- Performance measurements

## Current Status

### ✅ Completed Artifacts

1. **Source Code**
   - `engine/include/user_paths.h` (124 lines) - Complete API with documentation
   - `engine/src/user_paths.cpp` (304 lines) - Full cross-platform implementation
   - CMakeLists.txt integration (2 lines added)

2. **Build Integration**
   - Successfully compiles on Windows
   - Linked into both glint.exe and glint_core.lib
   - All linker errors resolved

### 🔄 In Progress Artifacts

1. **Runtime Verification**
   - Application launches successfully
   - Directory creation pending verification
   - File persistence testing in progress

### 🔲 Pending Artifacts

1. **Documentation**
   - API_REFERENCE.md
   - PLATFORM_BEHAVIOR.md
   - PORTABLE_MODE_GUIDE.md
   - Integration examples

2. **Test Results**
   - Portable mode validation
   - Cross-platform compatibility matrix
   - Performance benchmarks
   - Edge case coverage report

3. **Validation Reports**
   - Acceptance criteria verification
   - Code review checklist
   - Security audit (path traversal protection)

## Key Implementation Details

### Platform-Specific Paths

**Windows**:
- Data: `%APPDATA%/Glint3D/` (e.g., `C:\Users\User\AppData\Roaming\Glint3D\`)
- Config: `%APPDATA%/Glint3D/config/`
- Cache: `%LOCALAPPDATA%/Glint3D/Cache/`

**Linux**:
- Data: `~/.local/share/glint3d/`
- Config: `~/.config/glint3d/`
- Cache: `~/.cache/glint3d/`

**macOS**:
- Data: `~/Library/Application Support/Glint3D/`
- Config: `~/Library/Preferences/Glint3D/`
- Cache: `~/Library/Caches/Glint3D/`

**Portable Mode**:
- Data: `./runtime/data/`
- Config: `./runtime/config/`
- Cache: `./runtime/cache/`

### API Functions

```cpp
// Directory getters
std::filesystem::path getUserDataDir();
std::filesystem::path getConfigDir();
std::filesystem::path getCacheDir();

// File path helpers
std::filesystem::path getDataPath(const std::string& filename);
std::filesystem::path getConfigPath(const std::string& filename);
std::filesystem::path getCachePath(const std::string& filename);

// Portable mode management
bool isPortableMode();
void enablePortableMode();
```

### Integration Points

1. **imgui_ui_layer.cpp**
   - Line 26: `getDataPath("history.txt")` - Command history persistence
   - Line 38: `getDataPath("history.txt")` - History saving
   - Line 101: `getConfigPath("imgui.ini")` - ImGui window layout persistence

2. **ui_bridge.cpp**
   - Line 875: `getDataPath("recent.txt")` - Recent files loading
   - Line 887: `getDataPath("recent.txt")` - Recent files saving

## Next Steps

1. **Runtime Verification** (Phase 4)
   - Launch application and verify directory creation
   - Test file persistence across sessions
   - Validate platform-specific paths

2. **Portable Mode Testing** (Phase 5)
   - Create .portable marker file
   - Verify fallback to ./runtime/* directories
   - Test environment variable activation

3. **Documentation Generation** (Phase 7)
   - Generate API reference
   - Document platform behaviors
   - Create usage examples

4. **Cross-Platform Validation** (Phase 6)
   - Test on Windows 10/11
   - Test on Linux with various distros
   - Test on macOS

## Related Files

- Task specification: `../task.json`
- Execution checklist: `../checklist.md`
- Progress log: `../progress.ndjson`
- Project documentation: `CLAUDE.md` (line 493 reference to user paths)

## Acceptance Criteria Summary

**Build Phase** (✅ COMPLETE):
- [x] Build succeeds without errors
- [x] CMake integration complete
- [x] All linker errors resolved
- [x] Missing includes added

**Runtime Phase** (🔄 IN PROGRESS):
- [ ] Application launches without crashes
- [ ] Platform-specific directories created correctly
- [ ] File persistence works across sessions
- [ ] Portable mode activates correctly
- [ ] All three directory types work independently

**Validation Phase** (🔲 PENDING):
- [ ] Cross-platform testing complete
- [ ] Documentation generated
- [ ] Edge cases handled
- [ ] Performance validated
- [ ] Ready for production use
