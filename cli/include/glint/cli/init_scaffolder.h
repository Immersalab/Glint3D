// Machine Summary Block
// {"file":"cli/include/glint/cli/init_scaffolder.h","purpose":"Declares workspace scaffolding utilities for the Glint CLI init command.","exports":["glint::cli::InitScaffolder","glint::cli::InitPlan","glint::cli::InitRequest"],"depends_on":["<filesystem>","<optional>","<string>","<vector>"],"notes":["template_driven_scaffolding","deterministic_outputs","plan_then_execute"]}
// Human Summary
// Interface describing the plan/execute workflow for `glint init`, including template selection, module and asset bookkeeping, and deterministic file generation.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/**
 * @file init_scaffolder.h
 * @brief Workspace scaffolding helpers for the Glint CLI init command.
 *
 * Produces deterministic plans describing the directory tree, manifests,
 * configuration files, and lockfiles that must be generated when a user runs
 * `glint init`. The scaffolder separates planning from execution to support
 * `--dry-run`, future automation hooks, and provenance logging.
 */

namespace glint::cli {

/// @brief Types of operations emitted during scaffolding.
enum class InitOperationType {
    CreateDirectory,
    WriteFile,
    CopyTemplateFile
};

/// @brief Represents a single planned operation produced by the scaffolder.
struct InitOperation {
    InitOperationType type;                ///< Operation kind.
    std::filesystem::path sourcePath;      ///< Optional source (for template copies).
    std::filesystem::path destinationPath; ///< Destination path (absolute).
    std::string contents;                  ///< File contents (for WriteFile).
};

/// @brief High-level summary of a scaffolding plan.
struct InitPlan {
    std::vector<InitOperation> operations; ///< Ordered list of operations.
    std::vector<std::string> nextSteps;    ///< Suggested follow-up commands.
};

/// @brief Request describing the desired workspace scaffold.
struct InitRequest {
    std::filesystem::path workspaceRoot;
    std::string templateName = "blank";
    bool withSamples = false;
    bool force = false;
    bool noConfig = false;
    bool jsonOutput = false;
    bool dryRun = false;
    std::vector<std::string> modules;
    std::vector<std::string> assetPacks;
};

/// @brief Result of executing an init plan.
struct InitResult {
    InitPlan plan;
    bool executed = false;
};

/**
 * @brief Coordinates planning and execution of workspace scaffolding.
 */
class InitScaffolder {
public:
    InitScaffolder();

    /**
     * @brief Build a deterministic plan for the provided request.
     * @throws std::runtime_error on invalid inputs (missing template, unsafe directory).
     */
    InitPlan plan(const InitRequest& request) const;

    /**
     * @brief Execute a previously generated plan.
     * @return InitResult containing plan and execution flag.
     */
    InitResult execute(const InitPlan& plan, bool dryRun) const;

private:
    /// @brief Internal metadata describing an on-disk template.
    struct TemplateMetadata {
        std::string name;
        std::string description;
        std::vector<std::string> defaultModules;
        std::vector<std::string> defaultAssetPacks;
        std::filesystem::path templateRoot;
    };

    TemplateMetadata loadTemplateMetadata(const std::string& name) const;
    std::string buildBaseManifest(const InitRequest& request,
                                  const TemplateMetadata& metadata,
                                  const std::vector<std::string>& modules,
                                  const std::vector<std::string>& assetPacks) const;
    std::string buildWorkspaceConfig(const TemplateMetadata& metadata) const;
    std::string buildModulesLock(const std::vector<std::string>& modules) const;
    std::string buildAssetsLock(const std::vector<std::string>& assetPacks) const;
    void appendDirectorySkeleton(const InitRequest& request, InitPlan& plan) const;
    void appendTemplateFiles(const TemplateMetadata& metadata,
                             const InitRequest& request,
                             InitPlan& plan) const;
};

} // namespace glint::cli
