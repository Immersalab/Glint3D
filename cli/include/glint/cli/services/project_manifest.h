// Machine Summary Block
// {"file":"cli/include/glint/cli/services/project_manifest.h","purpose":"Defines data structures and loader utilities for glint.project.json manifests.","exports":["glint::cli::services::SceneDescriptor","glint::cli::services::ModuleDescriptor","glint::cli::services::AssetDescriptor","glint::cli::services::DeterminismDescriptor","glint::cli::services::ProjectManifest","glint::cli::services::ProjectManifestLoader"],"depends_on":["<filesystem>","<optional>","<string>","<unordered_map>","<vector>"],"notes":["project_manifest_schema","workspace_resolution","manifest_validation"]}
// Human Summary
// Provides a strongly-typed representation of `glint.project.json` and a loader that resolves workspace-relative paths.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file project_manifest.h
 * @brief Structures and utilities for working with Glint project manifests.
 */

namespace glint::cli::services {

/// @brief Describes a renderable scene declared in the project manifest.
struct SceneDescriptor {
    std::string id;
    std::filesystem::path path;
    std::optional<std::filesystem::path> thumbnail;
    std::optional<std::filesystem::path> defaultOutput;
};

/// @brief Describes a module declared in the project manifest.
struct ModuleDescriptor {
    std::string name;
    bool enabled = true;
    bool optional = false;
    std::vector<std::string> dependsOn;
};

/// @brief Describes an asset pack declared in the project manifest.
struct AssetDescriptor {
    std::string name;
    std::string version;
    std::string source;
    bool optional = false;
    std::string hash;
};

/// @brief Captures determinism-related metadata from the manifest.
struct DeterminismDescriptor {
    int64_t rngSeed = 0;
    std::optional<std::filesystem::path> modulesLock;
    std::optional<std::filesystem::path> assetsLock;
    bool captureProvenance = true;
};

/// @brief Strongly-typed representation of the entire project manifest.
struct ProjectManifest {
    std::filesystem::path manifestPath;
    std::filesystem::path workspaceRoot;
    std::filesystem::path rendersDirectory;
    std::filesystem::path modulesDirectory;
    std::filesystem::path assetsDirectory;
    std::filesystem::path configDirectory;
    std::string schemaVersion;
    std::string projectName;
    std::string projectSlug;
    std::string projectVersion;
    std::optional<std::string> description;
    std::optional<std::string> defaultTemplate;
    bool requiresGpu = false;
    std::vector<std::string> engineModules;
    std::vector<SceneDescriptor> scenes;
    std::vector<ModuleDescriptor> modules;
    std::vector<AssetDescriptor> assets;
    DeterminismDescriptor determinism;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> configurationOverrides;
};

/**
 * @brief Loader utility that parses and validates `glint.project.json`.
 */
class ProjectManifestLoader {
public:
    ProjectManifestLoader() = delete;

    /**
     * @brief Load and validate the manifest from disk.
     * @param manifestPath Absolute or relative path to `glint.project.json`.
     * @throws std::runtime_error on missing fields or parse errors.
     */
    static ProjectManifest load(const std::filesystem::path& manifestPath);
};

} // namespace glint::cli::services
