// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/assets_command.h","purpose":"Declares the glint assets command for listing and synchronizing asset packs.","exports":["glint::cli::AssetsCommand"],"depends_on":["glint/cli/command_dispatcher.h","<filesystem>","<map>","<optional>","<string>"],"notes":["asset_management_cli","lockfile_updates","structured_output"]}
// Human Summary
// CLI front-end for `glint assets`, supporting list/status reporting and sync operations against assets.lock.

#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "glint/cli/command_dispatcher.h"

#include "glint/cli/services/project_manifest.h"
#include "glint/cli/services/workspace_locks.h"

/**
 * @file assets_command.h
 * @brief Interface for the `glint assets` verb.
 */

namespace glint::cli {

/**
 * @brief Implements asset management (`glint assets`) operations.
 */
class AssetsCommand : public ICommand {
public:
    AssetsCommand();

    /// @brief Execute the assets command for the provided context.
    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    enum class Mode {
        List,
        Status,
        Sync
    };

    struct ParsedArgs {
        Mode mode = Mode::Status;
        std::filesystem::path manifestPath;
        std::optional<std::string> packName;
        std::optional<std::string> sourceOverride;
    };

    struct AssetStatus {
        std::string name;
        std::string manifestVersion;
        std::string manifestSource;
        std::string manifestHash;
        bool manifestOptional = false;
        bool declared = false;
        bool hasLockEntry = false;
        std::string lockVersion;
        std::string lockStatus;
        std::string lockHash;
        bool needsSync = false;
    };

    [[nodiscard]] std::optional<ParsedArgs> parseArguments(const CommandExecutionContext& context,
                                                           CLIExitCode& errorCode,
                                                           std::string& errorMessage) const;

    [[nodiscard]] services::ProjectManifest loadManifest(const std::filesystem::path& manifestPath,
                                                         const CommandExecutionContext& context) const;

    [[nodiscard]] services::AssetRegistry loadRegistry(const services::ProjectManifest& manifest,
                                                       const CommandExecutionContext& context) const;

    [[nodiscard]] std::map<std::string, AssetStatus> buildAssetStatuses(
        const services::ProjectManifest& manifest,
        const services::AssetRegistry& registry) const;

    CLIExitCode handleList(const CommandExecutionContext& context,
                           const ParsedArgs& args,
                           const std::map<std::string, AssetStatus>& statuses) const;

    CLIExitCode handleStatus(const CommandExecutionContext& context,
                             const ParsedArgs& args,
                             const std::map<std::string, AssetStatus>& statuses) const;

    CLIExitCode handleSync(const CommandExecutionContext& context,
                           const ParsedArgs& args,
                           const services::ProjectManifest& manifest,
                           services::AssetRegistry& registry) const;

    void emitAssetEvent(const CommandExecutionContext& context,
                        const AssetStatus& status,
                        const std::string& event,
                        const std::optional<std::string>& sourceOverride = std::nullopt) const;

    static std::string formatStatusSummary(const AssetStatus& status);
};

} // namespace glint::cli
