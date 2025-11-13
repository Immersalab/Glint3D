// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/clean_command.h","purpose":"Declares the clean command handler for the Glint CLI platform.","exports":["glint::cli::CleanCommand"],"depends_on":["glint/cli/command_dispatcher.h"],"notes":["workspace_cleanup","cache_management","safe_deletion"]}
// Human Summary
// Handles the `glint clean` command for removing build artifacts, caches, and temporary files from the workspace.

#pragma once

#include "glint/cli/command_dispatcher.h"

#include <filesystem>

/**
 * @file clean_command.h
 * @brief Command handler for `glint clean`.
 */

namespace glint::cli {

/**
 * @brief Implements the `glint clean` command for workspace cleanup.
 *
 * This command:
 * - Removes render output directories (renders/)
 * - Clears cache directories (.glint/cache/)
 * - Deletes temporary files and lock files
 * - Supports selective cleaning with flags
 * - Provides dry-run mode for safety
 * - Respects .gitignore patterns
 */
class CleanCommand : public ICommand {
public:
    /**
     * @brief Execute the clean command.
     * @param context Execution context with arguments and output emitter.
     * @return Exit code indicating success or failure.
     */
    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    struct CleanOptions {
        bool dryRun = false;
        bool cleanRenders = false;
        bool cleanCache = false;
        bool cleanAll = false;
        bool verbose = false;
    };

    /**
     * @brief Parse command-line arguments into clean options.
     * @param args Command arguments.
     * @param options Output options structure.
     * @param errorMessage Output error message if parsing fails.
     * @return Exit code (Success or error code).
     */
    CLIExitCode parseArguments(const std::vector<std::string>& args,
                               CleanOptions& options,
                               std::string& errorMessage) const;

    /**
     * @brief Execute the clean operation.
     * @param context Execution context.
     * @param options Parsed clean options.
     * @return Exit code indicating success or failure.
     */
    CLIExitCode executeClean(const CommandExecutionContext& context,
                             const CleanOptions& options) const;

    /**
     * @brief Remove a directory and report results.
     * @param context Execution context.
     * @param path Directory path to remove.
     * @param dryRun If true, only report what would be removed.
     * @return Number of files/directories removed (or would be removed).
     */
    size_t removeDirectory(const CommandExecutionContext& context,
                          const std::filesystem::path& path,
                          bool dryRun) const;
};

} // namespace glint::cli
