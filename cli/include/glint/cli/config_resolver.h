// Machine Summary Block
// {"file":"cli/include/glint/cli/config_resolver.h","purpose":"Declares layered configuration resolution utilities for the Glint CLI platform.","exports":["glint::cli::ConfigResolver","glint::cli::ConfigResolveRequest","glint::cli::ConfigSnapshot"],"depends_on":["rapidjson/document.h","<filesystem>","<string>","<unordered_map>"],"notes":["layered_config_precedence","provenance_tracking","deterministic_merge"]}
// Human Summary
// Interface for merging configuration layers (global, workspace, manifest, flags) into a resolved snapshot with provenance tracking for the CLI.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <rapidjson/document.h>

/**
 * @file config_resolver.h
 * @brief Layered configuration resolution for the Glint CLI platform.
 *
 * Combines configuration sources (built-in defaults, global config,
 * environment variables, workspace files, manifest entries, and CLI overrides)
 * into a single deterministic snapshot. Produces provenance metadata for each
 * resolved key so the CLI can expose `glint config --explain`.
 */

namespace glint::cli {

/// @brief Enumerates configuration layers from lowest to highest precedence.
enum class ConfigLayer {
    BuiltInDefaults = 0,
    GlobalConfig,
    Environment,
    WorkspaceConfig,
    ManifestDefaults,
    ManifestOverrides,
    CommandContext,
    CliFlags
};

/// @brief Maps dot-delimited keys to JSON fragments encoded as strings.
using ConfigValueMap = std::unordered_map<std::string, std::string>;

/// @brief Describes a single provenance record for a resolved key.
struct ConfigProvenanceRecord {
    ConfigLayer layer;              ///< Which layer supplied the value.
    std::string source;             ///< Human-readable source (file path, ENV, etc.).
    std::string json;               ///< Value contribution serialized as JSON.
};

/// @brief Request parameters for configuration resolution.
struct ConfigResolveRequest {
    std::filesystem::path workspaceRoot;                     ///< Workspace root directory.
    std::optional<std::filesystem::path> manifestPath;       ///< Optional explicit manifest path.
    std::string sceneId;                                     ///< Scene identifier for overrides.
    std::unordered_map<std::string, std::string> commandContext; ///< Derived values from command parsing.
    std::unordered_map<std::string, std::string> cliOverrides;   ///< Explicit CLI overrides (`--set key=value`).
    bool includeEnvironment = true;                          ///< Toggle environment variable layer.
    bool strict = false;                                     ///< Emit errors on unknown keys if true.
};

/// @brief Resolved configuration snapshot and provenance.
struct ConfigSnapshot {
    rapidjson::Document document; ///< Resolved configuration tree.
    std::unordered_map<std::string, std::vector<ConfigProvenanceRecord>> provenance; ///< Provenance per key.
};

/// @brief Resolves configuration layers into a deterministic snapshot.
class ConfigResolver {
public:
    ConfigResolver();

    /**
     * @brief Resolve configuration according to Glint precedence rules.
     * @param request Resolution parameters (workspace, manifest path, overrides).
     * @return ConfigSnapshot containing merged document and provenance metadata.
     * @throws std::runtime_error When strict mode is enabled and invalid data is encountered.
     */
    ConfigSnapshot resolve(const ConfigResolveRequest& request) const;

private:
    using Allocator = rapidjson::Document::AllocatorType;

    rapidjson::Document buildBuiltInDefaults() const;
    rapidjson::Document loadGlobalConfig() const;
    rapidjson::Document loadWorkspaceConfig(const std::filesystem::path& workspaceRoot) const;
    rapidjson::Document loadManifestConfig(const std::optional<std::filesystem::path>& manifestPath,
                                           const std::string& sceneId,
                                           bool overrides) const;
    rapidjson::Document loadEnvironmentVariables() const;
    rapidjson::Document buildMapDocument(const std::unordered_map<std::string, std::string>& map) const;

    static void mergeDocuments(rapidjson::Document& target,
                               const rapidjson::Document& source,
                               std::unordered_map<std::string, std::vector<ConfigProvenanceRecord>>& provenance,
                               ConfigLayer layer,
                               const std::string& sourceLabel,
                               Allocator& allocator);

    static void mergeValues(rapidjson::Value& target,
                            const rapidjson::Value& source,
                            const std::string& keyPath,
                            std::unordered_map<std::string, std::vector<ConfigProvenanceRecord>>& provenance,
                            ConfigLayer layer,
                            const std::string& sourceLabel,
                            Allocator& allocator);

    static void appendProvenance(const std::string& keyPath,
                                 const rapidjson::Value& value,
                                 ConfigLayer layer,
                                 const std::string& sourceLabel,
                                 std::unordered_map<std::string, std::vector<ConfigProvenanceRecord>>& provenance);
};

} // namespace glint::cli
