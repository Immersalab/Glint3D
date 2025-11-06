
// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/validate_command.h","purpose":"Declares the CLI command responsible for project validation.","exports":["glint::cli::ValidateCommand"],"depends_on":["glint/cli/command_dispatcher.h","glint/cli/command_io.h","glint/cli/services/project_manifest.h","glint/cli/services/workspace_locks.h","<filesystem>","<string>","<vector>"],"notes":["schema_validation","module_lock_checks","ndjson_reporting"]}
// Human Summary
// Front-end for `glint validate`, providing argument parsing, manifest checks, and structured output.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "glint/cli/command_dispatcher.h"
#include "glint/cli/command_io.h"
#include "glint/cli/services/project_manifest.h"
#include "glint/cli/services/workspace_locks.h"

/**
 * @file validate_command.h
 * @brief CLI command implementation for manifest validation.
 */

namespace glint::cli {

/**
 * @brief Validates project manifests, scenes, modules, and asset locks.
 */
class ValidateCommand : public ICommand {
public:
    ValidateCommand();

    /// @brief Execute the command with the supplied context.
    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    struct ParsedArgs {
        std::filesystem::path manifestPath;
        bool strict = false;
        bool validateModules = false;
        bool validateAssets = false;
        std::optional<std::string> sceneId;
    };

    struct ValidationReport {
        size_t scenesValidated = 0;
        size_t modulesValidated = 0;
        size_t assetsValidated = 0;
    };

    [[nodiscard]] std::optional<ParsedArgs> parseArguments(const CommandExecutionContext& context,
                                                           CLIExitCode& errorCode,
                                                           std::string& errorMessage) const;
    [[nodiscard]] ValidationReport validateProject(const CommandExecutionContext& context,
                                                   const ParsedArgs& args,
                                                   CLIExitCode& exitCode) const;

    void emitPhaseEvent(const CommandExecutionContext& context,
                        const std::string& phase,
                        const std::string& status) const;
};

} // namespace glint::cli
