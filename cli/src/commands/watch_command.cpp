// Machine Summary Block
// {"file":"cli/src/commands/watch_command.cpp","purpose":"Implements the watch command stub with gated semantics documentation.","depends_on":["glint/cli/commands/watch_command.h","glint/cli/command_io.h"],"notes":["stub_implementation","future_feature","clear_user_guidance"]}
// Human Summary
// Stub that clearly communicates watch command is not yet implemented and provides guidance on alternative workflows.

#include "glint/cli/commands/watch_command.h"
#include "glint/cli/command_io.h"

#include <sstream>

namespace glint::cli {

CLIExitCode WatchCommand::run(const CommandExecutionContext& context) {
    std::ostringstream oss;
    oss << "glint watch: Command not yet implemented\n\n";
    oss << "This command will provide file system monitoring with automatic re-renders.\n";
    oss << "Planned features:\n";
    oss << "  - Watch project files for changes (scenes, ops, configs)\n";
    oss << "  - Trigger automatic renders on modification\n";
    oss << "  - Configurable watch patterns and debounce delays\n";
    oss << "  - NDJSON event stream for integration with tools\n\n";
    oss << "Workaround: Use external file watchers (e.g., watchexec, entr, nodemon)\n";
    oss << "Example with watchexec:\n";
    oss << "  watchexec -w scenes/ -w ops/ -- glint render --ops ops/main.json --output out.png\n\n";
    oss << "Status: Gated pending file system watcher library integration\n";
    oss << "Tracking: https://github.com/glint3d/glint/issues/TODO";

    emitCommandFailed(context, CLIExitCode::RuntimeError, oss.str(), "not_implemented");
    return CLIExitCode::RuntimeError;
}

} // namespace glint::cli
