// Machine Summary Block
// {"file":"engine/core/io/user_paths.cpp","purpose":"Implements user path helpers with platform-specific overrides and caching.","depends_on":["user_paths.h","<filesystem>","<windows.h>","<shlobj.h>","<cstdlib>"],"notes":["lazy_initializes_directories","honors_portable_mode"]}
// Human Summary
// Resolves and caches user data, config, and cache directories across supported platforms.
#include "user_paths.h"
#include <cstdlib>
#include <iostream>
#include <fstream>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
    #include <knownfolders.h>
#endif

namespace glint {

namespace {
    // Cached paths to avoid repeated filesystem/environment lookups
    std::filesystem::path g_userDataDir;
    std::filesystem::path g_configDir;
    std::filesystem::path g_cacheDir;
    bool g_pathsInitialized = false;
    bool g_portableMode = false;
    bool g_portableModeChecked = false;

    /**
     * @brief Get Windows Known Folder path using SHGetKnownFolderPath
     */
    #ifdef _WIN32
    std::filesystem::path getWindowsKnownFolder(const KNOWNFOLDERID& folderId)
    {
        PWSTR path = nullptr;
        HRESULT hr = SHGetKnownFolderPath(folderId, 0, nullptr, &path);

        if (SUCCEEDED(hr) && path) {
            std::filesystem::path result(path);
            CoTaskMemFree(path);
            return result;
        }

        return {}; // Return empty path on failure
    }
    #endif

    /**
     * @brief Get environment variable as string
     */
    std::string getEnvVar(const char* name)
    {
        const char* val = std::getenv(name);
        return val ? std::string(val) : std::string();
    }

    /**
     * @brief Get home directory across platforms
     */
    std::filesystem::path getHomeDir()
    {
        #ifdef _WIN32
            // Use USERPROFILE on Windows
            std::string home = getEnvVar("USERPROFILE");
            if (!home.empty()) {
                return std::filesystem::path(home);
            }

            // Fallback: HOMEDRIVE + HOMEPATH
            std::string drive = getEnvVar("HOMEDRIVE");
            std::string path = getEnvVar("HOMEPATH");
            if (!drive.empty() && !path.empty()) {
                return std::filesystem::path(drive + path);
            }
        #else
            // Unix-like: use HOME
            std::string home = getEnvVar("HOME");
            if (!home.empty()) {
                return std::filesystem::path(home);
            }
        #endif

        return {}; // Return empty if can't determine home
    }

    /**
     * @brief Initialize platform-specific paths (called once)
     */
    void initializePaths()
    {
        if (g_pathsInitialized) return;
        g_pathsInitialized = true;

        // Check for portable mode first
        if (isPortableMode()) {
            g_userDataDir = std::filesystem::path("runtime") / "data";
            g_configDir = std::filesystem::path("runtime") / "config";
            g_cacheDir = std::filesystem::path("runtime") / "cache";
            return;
        }

        #ifdef _WIN32
            // Windows: Use Known Folders API
            std::filesystem::path appData = getWindowsKnownFolder(FOLDERID_RoamingAppData);
            std::filesystem::path localAppData = getWindowsKnownFolder(FOLDERID_LocalAppData);

            if (!appData.empty()) {
                g_userDataDir = appData / "Glint3D";
                g_configDir = appData / "Glint3D" / "config";
            }

            if (!localAppData.empty()) {
                g_cacheDir = localAppData / "Glint3D" / "Cache";
            }

        #elif defined(__APPLE__)
            // macOS: Use ~/Library/{Application Support, Preferences, Caches}
            std::filesystem::path home = getHomeDir();
            if (!home.empty()) {
                g_userDataDir = home / "Library" / "Application Support" / "Glint3D";
                g_configDir = home / "Library" / "Preferences" / "Glint3D";
                g_cacheDir = home / "Library" / "Caches" / "Glint3D";
            }

        #else
            // Linux: Follow XDG Base Directory Specification
            std::filesystem::path home = getHomeDir();

            // XDG_DATA_HOME (default: ~/.local/share)
            std::string xdgData = getEnvVar("XDG_DATA_HOME");
            if (!xdgData.empty()) {
                g_userDataDir = std::filesystem::path(xdgData) / "glint3d";
            } else if (!home.empty()) {
                g_userDataDir = home / ".local" / "share" / "glint3d";
            }

            // XDG_CONFIG_HOME (default: ~/.config)
            std::string xdgConfig = getEnvVar("XDG_CONFIG_HOME");
            if (!xdgConfig.empty()) {
                g_configDir = std::filesystem::path(xdgConfig) / "glint3d";
            } else if (!home.empty()) {
                g_configDir = home / ".config" / "glint3d";
            }

            // XDG_CACHE_HOME (default: ~/.cache)
            std::string xdgCache = getEnvVar("XDG_CACHE_HOME");
            if (!xdgCache.empty()) {
                g_cacheDir = std::filesystem::path(xdgCache) / "glint3d";
            } else if (!home.empty()) {
                g_cacheDir = home / ".cache" / "glint3d";
            }
        #endif

        // Fallback to ./runtime/ if platform paths couldn't be determined
        if (g_userDataDir.empty()) {
            std::cerr << "[UserPaths] Warning: Could not determine platform data directory, using ./runtime/data/\n";
            g_userDataDir = std::filesystem::path("runtime") / "data";
        }
        if (g_configDir.empty()) {
            std::cerr << "[UserPaths] Warning: Could not determine platform config directory, using ./runtime/config/\n";
            g_configDir = std::filesystem::path("runtime") / "config";
        }
        if (g_cacheDir.empty()) {
            std::cerr << "[UserPaths] Warning: Could not determine platform cache directory, using ./runtime/cache/\n";
            g_cacheDir = std::filesystem::path("runtime") / "cache";
        }
    }

    /**
     * @brief Ensure directory exists, create if needed
     */
    void ensureDirectoryExists(const std::filesystem::path& dir)
    {
        if (!std::filesystem::exists(dir)) {
            try {
                std::filesystem::create_directories(dir);
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "[UserPaths] Error creating directory " << dir << ": " << e.what() << "\n";
            }
        }
    }

} // anonymous namespace

bool isPortableMode()
{
    if (g_portableModeChecked) {
        return g_portableMode;
    }

    g_portableModeChecked = true;

    // Check for GLINT_PORTABLE environment variable
    std::string envPortable = getEnvVar("GLINT_PORTABLE");
    if (!envPortable.empty() && (envPortable == "1" || envPortable == "true" || envPortable == "TRUE")) {
        g_portableMode = true;
        return true;
    }

    // Check for ./runtime/.portable marker file
    std::filesystem::path portableMarker = std::filesystem::path("runtime") / ".portable";
    if (std::filesystem::exists(portableMarker)) {
        g_portableMode = true;
        return true;
    }

    g_portableMode = false;
    return false;
}

void enablePortableMode()
{
    std::filesystem::path runtimeDir = "runtime";
    std::filesystem::path portableMarker = runtimeDir / ".portable";

    try {
        // Create runtime directory if it doesn't exist
        if (!std::filesystem::exists(runtimeDir)) {
            std::filesystem::create_directories(runtimeDir);
        }

        // Create .portable marker file
        if (!std::filesystem::exists(portableMarker)) {
            std::ofstream marker(portableMarker);
            if (marker.is_open()) {
                marker << "This file marks Glint3D as running in portable mode.\n";
                marker << "All user data, config, and cache will be stored in ./runtime/\n";
                marker.close();
            }
        }

        // Update cached state
        g_portableMode = true;
        g_portableModeChecked = true;

        // Force path reinitialization if already initialized
        if (g_pathsInitialized) {
            g_pathsInitialized = false;
            initializePaths();
        }

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[UserPaths] Error enabling portable mode: " << e.what() << "\n";
    }
}

std::filesystem::path getUserDataDir()
{
    initializePaths();
    ensureDirectoryExists(g_userDataDir);
    return g_userDataDir;
}

std::filesystem::path getConfigDir()
{
    initializePaths();
    ensureDirectoryExists(g_configDir);
    return g_configDir;
}

std::filesystem::path getCacheDir()
{
    initializePaths();
    ensureDirectoryExists(g_cacheDir);
    return g_cacheDir;
}

std::filesystem::path getDataPath(const std::string& filename)
{
    std::filesystem::path dataDir = getUserDataDir();
    std::filesystem::path filePath = dataDir / filename;

    // Ensure parent directory exists
    std::filesystem::path parentDir = filePath.parent_path();
    if (parentDir != dataDir) {
        ensureDirectoryExists(parentDir);
    }

    return filePath;
}

std::filesystem::path getConfigPath(const std::string& filename)
{
    std::filesystem::path configDir = getConfigDir();
    std::filesystem::path filePath = configDir / filename;

    // Ensure parent directory exists
    std::filesystem::path parentDir = filePath.parent_path();
    if (parentDir != configDir) {
        ensureDirectoryExists(parentDir);
    }

    return filePath;
}

std::filesystem::path getCachePath(const std::string& filename)
{
    std::filesystem::path cacheDir = getCacheDir();
    std::filesystem::path filePath = cacheDir / filename;

    // Ensure parent directory exists
    std::filesystem::path parentDir = filePath.parent_path();
    if (parentDir != cacheDir) {
        ensureDirectoryExists(parentDir);
    }

    return filePath;
}

} // namespace glint
