// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/profile_command.h","purpose":"Declares the profile command handler (stub) for the Glint CLI platform.","exports":["glint::cli::ProfileCommand"],"depends_on":["glint/cli/command_dispatcher.h"],"notes":["stub_implementation","performance_profiling","future_feature"]}
// Human Summary
// Stub implementation for `glint profile` command that captures detailed render performance metrics. Full implementation gated pending profiling instrumentation.

#pragma once

#include "glint/cli/command_dispatcher.h"

/**
 * @file profile_command.h
 * @brief Stub command handler for `glint profile`.
 */

namespace glint::cli {

/**
 * @brief Stub implementation of the `glint profile` command.
 *
 * This command will eventually:
 * - Capture detailed render performance metrics
 * - Profile BVH construction, raytracing, shader execution
 * - Generate flamegraphs and timeline visualizations
 * - Export profiling data in standard formats (Chrome Trace, JSON)
 * - Support sampling and instrumentation modes
 *
 * **Current Status**: Stub implementation. Returns RuntimeError with guidance.
 */
class ProfileCommand : public ICommand {
public:
    /**
     * @brief Execute the profile command stub.
     * @param context Execution context with arguments and output emitter.
     * @return RuntimeError exit code with informational message.
     */
    CLIExitCode run(const CommandExecutionContext& context) override;
};

} // namespace glint::cli
