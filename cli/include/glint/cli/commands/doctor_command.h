
// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/doctor_command.h","purpose":"Declares the CLI diagnostics command that checks CLI prerequisites.","exports":["glint::cli::DoctorCommand"],"depends_on":["glint/cli/command_dispatcher.h","glint/cli/command_io.h","glint/cli/services/project_manifest.h","glint/cli/services/workspace_locks.h","<filesystem>","<optional>","<string>","<vector>"],"notes":["workspace_health_checks","optional_fixups","ndjson_reporting"]}
// Human Summary
// Provides the `glint doctor` command which runs health checks and optional fix-ups for CLI workspaces.

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
 * @file doctor_command.h
 * @brief CLI diagnostics entry point.
 */

namespace glint::cli {

/**
 * @brief Runs workspace diagnostics and optional repairs.
 */
class DoctorCommand : public ICommand {
public:
    DoctorCommand();

    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    struct ParsedArgs {
        std::filesystem::path manifestPath;
        bool attemptFix = false;
    };

    struct CheckResult {
        std::string name;
        std::string status;
        std::string message;
    };

    std::optional<ParsedArgs> parseArguments(const CommandExecutionContext& context,
                                             CLIExitCode& errorCode,
                                             std::string& errorMessage) const;

    CLIExitCode runChecks(const CommandExecutionContext& context,
                          const ParsedArgs& args) const;

    void emitCheck(const CommandExecutionContext& context,
                   const CheckResult& result) const;
};

} // namespace glint::cli
