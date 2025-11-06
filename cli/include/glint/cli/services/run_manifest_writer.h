// Machine Summary Block
// {"file":"cli/include/glint/cli/services/run_manifest_writer.h","purpose":"Declares utilities for capturing and writing render run manifests.","exports":["glint::cli::services::RunManifestWriter","glint::cli::services::RunManifestOptions","glint::cli::services::FrameRecord","glint::cli::services::ModuleRecord","glint::cli::services::AssetRecord","glint::cli::services::DeterminismMetadata","glint::cli::services::PlatformMetadata","glint::cli::services::EngineMetadata","glint::cli::services::CliInvocationMetadata"],"depends_on":["application/cli_parser.h","<filesystem>","<optional>","<string>","<vector>"],"notes":["determinism_logging","schema_versioned_payload","utf8_lf_io"]}
// Human Summary
// An aggregate writer for `renders/<name>/run.json` files that records CLI invocation details, platform/engine metadata, determinism inputs, and frame outputs.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "application/cli_parser.h"

/**
 * @file run_manifest_writer.h
 * @brief Facilities for writing deterministic render run manifests.
 */

namespace glint::cli::services {

/// @brief Describes the CLI invocation that triggered a run manifest.
struct CliInvocationMetadata {
    std::string command;                       ///< Primary CLI verb (e.g., "render").
    std::vector<std::string> arguments;        ///< argv tokens excluding the executable.
    bool jsonMode = false;                     ///< Whether the CLI was operating in `--json` mode.
    std::string projectPath;                   ///< Path to the active `glint.project.json`.
};

/// @brief Captures host platform information for reproducibility.
struct PlatformMetadata {
    std::string operatingSystem;
    std::string cpu;
    std::string gpu;
    std::string driverVersion;
    std::string kernel;
};

/// @brief Declares module metadata persisted in the manifest.
struct ModuleRecord {
    std::string name;
    std::string version;
    std::string hash;
    bool enabled = true;
};

/// @brief Declares asset pack metadata persisted in the manifest.
struct AssetRecord {
    std::string name;
    std::string version;
    std::string hash;
    std::string status; ///< e.g., "installed", "pending".
};

/// @brief Aggregated engine metadata included in the manifest.
struct EngineMetadata {
    std::string version;
    std::vector<ModuleRecord> modules;
    std::vector<AssetRecord> assets;
};

/// @brief Describes determinism-critical data captured alongside the run.
struct DeterminismMetadata {
    int64_t rngSeed = 0;
    std::vector<int> frames;
    std::string configDigest;
    std::string sceneDigest;
    std::string templateName;
    std::string gitRevision;
    std::vector<std::string> shaderHashes;
};

/// @brief Individual frame statistics emitted in the run manifest.
struct FrameRecord {
    int frame = 0;
    double durationMs = 0.0;
    std::string output;
};

/// @brief Options supplied when finalising the run manifest.
struct RunManifestOptions {
    std::string schemaVersion = "1.0.0";
    std::string runId;                                       ///< Unique identifier for the run.
    std::string timestampUtc;                                ///< ISO 8601 timestamp; generated automatically when empty.
    CliInvocationMetadata cli;
    PlatformMetadata platform;
    EngineMetadata engine;
    DeterminismMetadata determinism;
    std::filesystem::path outputDirectory;                   ///< Workspace-relative render output directory.
    std::vector<FrameRecord> frames;
    std::vector<std::string> warnings;
    CLIExitCode exitCode = CLIExitCode::Success;
};

/**
 * @brief Writes `renders/<name>/run.json` files that capture reproducibility metadata.
 *
 * The writer gathers metadata via the `RunManifestOptions` structure and emits a
 * deterministic JSON payload (UTF-8, LF newlines) aligned with the platform spec.
 */
class RunManifestWriter {
public:
    /// @brief Initialise the writer with the destination manifest path.
    explicit RunManifestWriter(std::filesystem::path manifestPath);

    /**
     * @brief Persist the run manifest to disk.
     * @param options Aggregated metadata describing the render session.
     * @throws std::runtime_error when the manifest cannot be written.
     */
    void write(const RunManifestOptions& options) const;

private:
    std::filesystem::path m_manifestPath;
};

} // namespace glint::cli::services
