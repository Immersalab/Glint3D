# User Paths API Reference

This document describes the public API exposed through `engine/include/user_paths.h` for retrieving Glint3D file-system locations in a cross-platform manner.

## Overview

The helpers provide lazily-initialised, cached paths for the three application data domains:

- **Data** – persistent user-facing state (history, recent files, session data).
- **Config** – application and UI preferences.
- **Cache** – temporary or reproducible assets (shader cache, thumbnails).

Portable mode redirects all domains to a local `runtime/` tree when the `GLINT_PORTABLE` environment variable is set or a `runtime/.portable` marker file is present.

## Function Summary

| Function | Signature | Notes |
|----------|-----------|-------|
| `getUserDataDir` | `std::filesystem::path glint::getUserDataDir();` | Creates and returns the platform data directory. |
| `getConfigDir` | `std::filesystem::path glint::getConfigDir();` | Creates and returns the platform config directory. |
| `getCacheDir` | `std::filesystem::path glint::getCacheDir();` | Creates and returns the platform cache directory. |
| `isPortableMode` | `bool glint::isPortableMode();` | Detects portable mode (env var or marker file). |
| `enablePortableMode` | `void glint::enablePortableMode();` | Writes `runtime/.portable` and reinitialises cached paths. |
| `getDataPath` | `std::filesystem::path glint::getDataPath(const std::string& filename);` | Resolves a file inside the data directory (auto-creates parents). |
| `getConfigPath` | `std::filesystem::path glint::getConfigPath(const std::string& filename);` | Resolves a file inside the config directory (auto-creates parents). |
| `getCachePath` | `std::filesystem::path glint::getCachePath(const std::string& filename);` | Resolves a file or subpath inside the cache directory (auto-creates parents). |

### Path Accessors

Each accessor:

1. Calls an internal `initializePaths()` once per process to compute platform-specific roots.
2. Ensures the resulting directory exists (`std::filesystem::create_directories`).
3. Returns the cached `std::filesystem::path`.

Repeated calls are inexpensive because the computation is cached after the first successful initialisation.

### File Helpers

`getDataPath`, `getConfigPath`, and `getCachePath` compose the respective directory root with the provided file or subdirectory. If the relative path contains additional folders (e.g., `logs/session.json`), those parents are created before returning.

### Portable Mode Controls

Portable mode is active when either condition is true:

- The process environment contains `GLINT_PORTABLE=1` (case-insensitive for `"true"`).
- A `runtime/.portable` file exists relative to the current working directory.

`isPortableMode()` caches the detection result until `enablePortableMode()` is called, which:

1. Ensures `runtime/` exists.
2. Creates or overwrites the marker file with a diagnostic message.
3. Marks the cached state dirty and re-initialises directories in portable layout.

## Usage Examples

```cpp
#include "user_paths.h"
#include <fstream>

void GlintApp::saveHistory(const std::string& contents)
{
    const auto historyPath = glint::getDataPath("history.txt");
    std::ofstream historyStream(historyPath, std::ios::out | std::ios::trunc);
    historyStream << contents;
}

void GlintApp::loadImguiLayout()
{
    const auto imguiIni = glint::getConfigPath("imgui.ini");
    ImGui::LoadIniSettingsFromDisk(imguiIni.string().c_str());
}
```

## Error Handling

Directory creation failures are caught and logged to `std::cerr` with a `[UserPaths]` prefix. The functions still return the intended path, allowing the caller to surface the error or retry.

If no platform directory can be determined (e.g., missing environment variables or API failures), the helper falls back to `./runtime/<domain>` and logs a warning.

## Testing Hooks

The `tools/user_paths_probe.cpp` utility (built as `user_paths_probe`) exercises the API without launching the full renderer. It prints scenario summaries in JSON to verify directory creation, portable mode detection, and caching stability.
