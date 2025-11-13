// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/watch_command.h","purpose":"Declares the watch command handler (stub) for the Glint CLI platform.","exports":["glint::cli::WatchCommand"],"depends_on":["glint/cli/command_dispatcher.h"],"notes":["stub_implementation","file_graph_monitoring","future_feature"]}
// Human Summary
// Stub implementation for `glint watch` command that monitors file changes and triggers rebuilds. Full implementation gated pending file system watcher integration.

#pragma once

#include "glint/cli/command_dispatcher.h"

/**
 * @file watch_command.h
 * @brief Stub command handler for `glint watch`.
 */

namespace glint::cli {

/**
 * @brief Stub implementation of the `glint watch` command.
 *
 * This command will eventually:
 * - Monitor project files for changes (scenes, ops, configs)
 * - Trigger automatic re-renders on file modification
 * - Support configurable watch patterns and debouncing
 * - Emit NDJSON events for file system changes
 *
 * **Current Status**: Stub implementation. Returns RuntimeError with guidance.
 */
class WatchCommand : public ICommand {
public:
    /**
     * @brief Execute the watch command stub.
     * @param context Execution context with arguments and output emitter.
     * @return RuntimeError exit code with informational message.
     */
    CLIExitCode run(const CommandExecutionContext& context) override;
};

} // namespace glint::cli
