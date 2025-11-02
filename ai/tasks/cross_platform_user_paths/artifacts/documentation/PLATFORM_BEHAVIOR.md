# Platform Behavior Matrix

This document summarizes where Glint3D stores user data, configuration, and cache files across supported platforms, plus how portable mode reshapes the layout.

## Desktop Defaults

| Platform | Data Directory | Config Directory | Cache Directory | Notes |
|----------|----------------|------------------|-----------------|-------|
| **Windows** | `%APPDATA%/Glint3D` (`FOLDERID_RoamingAppData`) | `%APPDATA%/Glint3D/config` | `%LOCALAPPDATA%/Glint3D/Cache` | Uses Known Folders via `SHGetKnownFolderPath`. |
| **Linux / BSD** | `${XDG_DATA_HOME:-~/.local/share}/glint3d` | `${XDG_CONFIG_HOME:-~/.config}/glint3d` | `${XDG_CACHE_HOME:-~/.cache}/glint3d` | Honors XDG base directory variables with lowercase app name. |
| **macOS** | `~/Library/Application Support/Glint3D` | `~/Library/Preferences/Glint3D` | `~/Library/Caches/Glint3D` | Matches Apple’s Library domain guidance. |

### Fallback Logic

When platform APIs or environment variables fail to resolve, Glint3D logs a warning and falls back to `./runtime/{data,config,cache}` within the process working directory. This ensures the application never proceeds without a writable location.

## Portable Mode

Portable mode is designed for self-contained or USB deployments. When active, all directories are relative to `./runtime/`:

| Portable Path | Purpose |
|---------------|---------|
| `runtime/data` | Persistent state (history, recent files, autosaves). |
| `runtime/config` | Configuration, window layouts, key bindings. |
| `runtime/cache` | Shader and thumbnail caches; safe to purge. |

Activation triggers:

1. Environment variable `GLINT_PORTABLE` equal to `1`, `true`, or `TRUE`.
2. Presence of `runtime/.portable` file.
3. Calling `glint::enablePortableMode()`, which creates the marker file.

Portable mode takes precedence over platform defaults and is cached for process lifetime unless explicitly re-enabled.

## Directory Creation and Permissions

- All helper functions call `std::filesystem::create_directories` to guarantee the target exists before returning.
- Permission failures are surfaced via `std::filesystem::filesystem_error` messages on stderr.
- Cache sub-path helpers create intermediate folders when a nested filename is supplied (`getCachePath("tmp/session.bin")`).

## Environment Variable Reference

| Variable | Platform | Meaning |
|----------|----------|---------|
| `GLINT_PORTABLE` | All | Forces portable mode when set to truthy value. |
| `XDG_DATA_HOME` | Linux | Overrides data root (`$XDG_DATA_HOME/glint3d`). |
| `XDG_CONFIG_HOME` | Linux | Overrides config root (`$XDG_CONFIG_HOME/glint3d`). |
| `XDG_CACHE_HOME` | Linux | Overrides cache root (`$XDG_CACHE_HOME/glint3d`). |
| `USERPROFILE` / `HOMEDRIVE` + `HOMEPATH` | Windows | Fallback for locating the user home directory. |
| `HOME` | Linux / macOS | Fallback when XDG variables or macOS Library paths require a home directory. |

## Operational Checklist

1. **Initialization** – Call any accessor (`getUserDataDir`, `getConfigDir`, `getCacheDir`) early in startup to ensure directories exist and to cache the results.
2. **File Access** – Use `get*Path("filename.ext")` helpers to avoid manual path concatenation.
3. **Portable Switch** – Offer a CLI or settings toggle that calls `glint::enablePortableMode()` before reloading state.
4. **Testing** – Run `user_paths_probe` for quick regression checks when modifying path logic.
