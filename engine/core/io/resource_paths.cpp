#include "resource_paths.h"

#include <cstdlib>
#include <filesystem>
#include <mutex>

namespace {
    std::mutex g_mutex;
    std::filesystem::path g_override;

    std::filesystem::path compiledRoot() {
#ifdef GLINT_RESOURCE_ROOT
        return std::filesystem::path{GLINT_RESOURCE_ROOT};
#else
        return std::filesystem::current_path();
#endif
    }

    std::filesystem::path envRoot() {
        if (const char* env = std::getenv("GLINT_RESOURCE_ROOT")) {
            if (*env) {
                return std::filesystem::path{env};
            }
        }
        return {};
    }
}

namespace ResourcePaths {

std::filesystem::path root() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_override.empty()) {
        return g_override;
    }

    if (auto fromEnv = envRoot(); !fromEnv.empty()) {
        return fromEnv;
    }

    return compiledRoot();
}

void setOverride(std::filesystem::path path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_override = std::move(path);
}

void clearOverride() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_override.clear();
}

std::filesystem::path resolve(const std::string& relative) {
    // std::filesystem::path handles path separator normalization automatically
    // when using the / operator, ensuring consistent separators for the platform
    auto result = root() / std::filesystem::path(relative);
    // Explicitly normalize to ensure proper path resolution
    return result.lexically_normal();
}

} // namespace ResourcePaths
