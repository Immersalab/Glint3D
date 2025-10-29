#pragma once

#include <filesystem>
#include <string>

/**
 * @file user_paths.h
 * @brief Cross-platform user data, config, and cache directory management
 *
 * Provides platform-specific paths following OS conventions:
 * - Linux: XDG Base Directory Specification
 * - macOS: Apple File System Programming Guide
 * - Windows: Known Folders (KNOWNFOLDERID)
 *
 * Directory Structure:
 * - User Data: history, recent files, application state
 * - Config: preferences, settings, UI layout
 * - Cache: temporary files, thumbnails, safe to delete
 *
 * Portable Mode: If runtime/.portable exists or --portable flag is set,
 * all paths fallback to ./runtime/ for self-contained deployments.
 */

namespace glint {

/**
 * @brief Get the user data directory for persistent application data
 *
 * Platform-specific locations:
 * - Windows: %APPDATA%/Glint3D/
 * - Linux:   ~/.local/share/glint3d/
 * - macOS:   ~/Library/Application Support/Glint3D/
 * - Fallback: ./runtime/data/
 *
 * Used for: history, recent files, user-generated content
 *
 * @return Path to user data directory (created if doesn't exist)
 */
std::filesystem::path getUserDataDir();

/**
 * @brief Get the config directory for application settings
 *
 * Platform-specific locations:
 * - Windows: %APPDATA%/Glint3D/config/
 * - Linux:   ~/.config/glint3d/
 * - macOS:   ~/Library/Preferences/Glint3D/
 * - Fallback: ./runtime/config/
 *
 * Used for: preferences, settings, keybindings, UI layouts
 *
 * @return Path to config directory (created if doesn't exist)
 */
std::filesystem::path getConfigDir();

/**
 * @brief Get the cache directory for temporary/expendable data
 *
 * Platform-specific locations:
 * - Windows: %LOCALAPPDATA%/Glint3D/Cache/
 * - Linux:   ~/.cache/glint3d/
 * - macOS:   ~/Library/Caches/Glint3D/
 * - Fallback: ./runtime/cache/
 *
 * Used for: thumbnails, shader cache, temporary renders
 * Safe to delete - application will regenerate as needed
 *
 * @return Path to cache directory (created if doesn't exist)
 */
std::filesystem::path getCacheDir();

/**
 * @brief Check if running in portable mode
 *
 * Portable mode is enabled when:
 * - ./runtime/.portable file exists, OR
 * - GLINT_PORTABLE environment variable is set
 *
 * In portable mode, all paths use ./runtime/ subdirectories
 * instead of platform-specific user directories.
 *
 * @return true if portable mode is active
 */
bool isPortableMode();

/**
 * @brief Enable portable mode by creating marker file
 *
 * Creates ./runtime/.portable to persist portable mode across sessions.
 * Useful for bundled/USB stick deployments.
 */
void enablePortableMode();

/**
 * @brief Get a specific file path within user data directory
 *
 * Example: getDataPath("history.txt") → ~/.local/share/glint3d/history.txt
 *
 * @param filename Filename within data directory
 * @return Full path to file (parent directory created if needed)
 */
std::filesystem::path getDataPath(const std::string& filename);

/**
 * @brief Get a specific file path within config directory
 *
 * Example: getConfigPath("settings.ini") → ~/.config/glint3d/settings.ini
 *
 * @param filename Filename within config directory
 * @return Full path to file (parent directory created if needed)
 */
std::filesystem::path getConfigPath(const std::string& filename);

/**
 * @brief Get a specific file path within cache directory
 *
 * Example: getCachePath("thumbnails") → ~/.cache/glint3d/thumbnails/
 *
 * @param filename Filename or subdirectory within cache directory
 * @return Full path (parent directory created if needed)
 */
std::filesystem::path getCachePath(const std::string& filename);

} // namespace glint
