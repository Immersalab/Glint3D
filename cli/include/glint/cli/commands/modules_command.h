// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/modules_command.h","purpose":"Declares the glint modules command for listing and mutating module state.","exports":["glint::cli::ModulesCommand"],"depends_on":["glint/cli/command_dispatcher.h","<optional>","<string>","<vector>"],"notes":["module_management_cli","dependency_validation","lockfile_mutation"]}
// Human Summary
// CLI front-end for `glint modules`, providing list/enable/disable subcommands with dependency checks and lockfile persistence.

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>

#include "glint/cli/command_dispatcher.h"

#include "glint/cli/services/project_manifest.h"
#include "glint/cli/services/workspace_locks.h"

/**
 * @file modules_command.h
 * @brief Interface for the `glint modules` verb.
 */

namespace glint::cli {

/**
 * @brief Implements module management (`glint modules`) operations.
 */
class ModulesCommand : public ICommand {
public:
    ModulesCommand();

    /// @brief Execute the modules command for the provided context.
    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    enum class Mode {
        List,
        Enable,
        Disable
    };

    struct ParsedArgs {
        Mode mode = Mode::List;
        std::string moduleName;
    };

    struct ModuleStatus {
        std::string name;
        bool optional = false;
        bool defaultEnabled = true;
        bool enabled = false;
        bool declared = false;
        bool isCore = false;
        bool locked = false;
        std::vector<std::string> dependsOn;
        std::string version;
        std::string hash;
    };

    [[nodiscard]] std::optional<ParsedArgs> parseArguments(const CommandExecutionContext& context,
                                                           CLIExitCode& errorCode,
                                                           std::string& errorMessage) const;

    [[nodiscard]] std::filesystem::path resolveManifestPath(const CommandExecutionContext& context) const;

    [[nodiscard]] std::map<std::string, ModuleStatus> buildModuleStatuses(
        const services::ProjectManifest& manifest,
        const services::ModuleRegistry& registry) const;

    CLIExitCode handleList(const CommandExecutionContext& context,
                           const std::map<std::string, ModuleStatus>& statuses) const;

    CLIExitCode handleEnable(const CommandExecutionContext& context,
                             const std::string& moduleName,
                             std::map<std::string, ModuleStatus>& statuses,
                             services::ModuleRegistry& registry) const;

    CLIExitCode handleDisable(const CommandExecutionContext& context,
                              const std::string& moduleName,
                              std::map<std::string, ModuleStatus>& statuses,
                              services::ModuleRegistry& registry) const;

    void emitModuleEvent(const CommandExecutionContext& context,
                         const ModuleStatus& status,
                         const std::string& event) const;

    static std::string formatStatusSummary(const ModuleStatus& status);
};

} // namespace glint::cli
