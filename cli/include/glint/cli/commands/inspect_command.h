// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/inspect_command.h","purpose":"Declares the inspect command handler for the Glint CLI platform.","exports":["glint::cli::InspectCommand"],"depends_on":["glint/cli/command_dispatcher.h"],"notes":["scene_introspection","json_structured_output","manifest_inspection"]}
// Human Summary
// Handles the `glint inspect` command for examining scene files, project manifests, and run manifests without performing renders.

#pragma once

#include "glint/cli/command_dispatcher.h"

/**
 * @file inspect_command.h
 * @brief Command handler for `glint inspect`.
 */

namespace glint::cli {

/**
 * @brief Implements the `glint inspect` command for scene and manifest introspection.
 *
 * This command:
 * - Inspects scene files (obj, glb, gltf, etc.) for metadata
 * - Examines project manifests (glint.project.json)
 * - Analyzes run manifests (renders/<name>/run.json)
 * - Outputs structured data via `--json` flag
 * - Provides human-readable summaries by default
 */
class InspectCommand : public ICommand {
public:
    /**
     * @brief Execute the inspect command.
     * @param context Execution context with arguments and output emitter.
     * @return Exit code indicating success or failure.
     */
    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    enum class InspectTarget {
        Scene,
        ProjectManifest,
        RunManifest,
        Unknown
    };

    struct InspectOptions {
        std::string targetPath;
        InspectTarget targetType = InspectTarget::Unknown;
        bool verbose = false;
    };

    /**
     * @brief Parse command-line arguments into inspect options.
     * @param args Command arguments.
     * @param options Output options structure.
     * @param errorMessage Output error message if parsing fails.
     * @return Exit code (Success or error code).
     */
    CLIExitCode parseArguments(const std::vector<std::string>& args,
                               InspectOptions& options,
                               std::string& errorMessage) const;

    /**
     * @brief Determine the type of target to inspect.
     * @param path Path to the target file.
     * @return Target type.
     */
    InspectTarget determineTargetType(const std::string& path) const;

    /**
     * @brief Inspect a scene file.
     * @param context Execution context.
     * @param options Inspect options.
     * @return Exit code.
     */
    CLIExitCode inspectScene(const CommandExecutionContext& context,
                            const InspectOptions& options) const;

    /**
     * @brief Inspect a project manifest.
     * @param context Execution context.
     * @param options Inspect options.
     * @return Exit code.
     */
    CLIExitCode inspectProjectManifest(const CommandExecutionContext& context,
                                       const InspectOptions& options) const;

    /**
     * @brief Inspect a run manifest.
     * @param context Execution context.
     * @param options Inspect options.
     * @return Exit code.
     */
    CLIExitCode inspectRunManifest(const CommandExecutionContext& context,
                                   const InspectOptions& options) const;
};

} // namespace glint::cli
