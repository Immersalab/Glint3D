// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/help_command.h","purpose":"Declares the help command for displaying CLI usage and command reference.","exports":["glint::cli::HelpCommand"],"depends_on":["glint/cli/command_dispatcher.h"],"notes":["cli_help_display","command_reference_bridge"]}
// Human Summary
// Provides the help command that displays comprehensive CLI usage information and command reference.

#pragma once

#include "glint/cli/command_dispatcher.h"

namespace glint::cli {

/**
 * @brief Displays CLI help information and command reference.
 *
 * The help command shows:
 * - Command usage and syntax
 * - Available commands (core, management, and legacy)
 * - Global flags
 * - Exit codes
 * - Quick examples
 * - Documentation references
 */
class HelpCommand : public ICommand {
public:
    /**
     * @brief Execute the help command.
     * @param context Command execution context with arguments
     * @return Always returns CLIExitCode::Success
     */
    CLIExitCode run(const CommandExecutionContext& context) override;
};

} // namespace glint::cli
