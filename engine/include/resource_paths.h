#pragma once

#include <filesystem>
#include <string>

namespace ResourcePaths {

// Returns the active resource root. Respects an explicit override,
// then the GLINT_RESOURCE_ROOT environment variable, then the compiled default.
std::filesystem::path root();

// Allow command-line or runtime code to override the resource root.
void setOverride(std::filesystem::path path);

// Clear any override so the compiled/default root is used again.
void clearOverride();

// Resolve a relative path (using '/' separators) against the active root.
std::filesystem::path resolve(const std::string& relative);

} // namespace ResourcePaths
