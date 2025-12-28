// Machine Summary Block
// {"file":"cli/src/commands/help_command.cpp","purpose":"Implements the help command for CLI usage display.","depends_on":["glint/cli/commands/help_command.h","application/help_text.h","<iostream>"],"notes":["delegates_to_print_cli_help","new_cli_platform_help"]}
// Human Summary
// Implements the help command by delegating to the centralized print_cli_help() function.

#include "glint/cli/commands/help_command.h"
#include "application/help_text.h"
#include <iostream>

namespace glint::cli {

CLIExitCode HelpCommand::run(const CommandExecutionContext& context)
{
    // Delegate to the centralized help text printer
    print_cli_help();
    return CLIExitCode::Success;
}

} // namespace glint::cli
