// Machine Summary Block
// {"file":"cli/src/commands/clean_command.cpp","purpose":"Implements the Glint CLI clean command for workspace cleanup.","depends_on":["glint/cli/commands/clean_command.h","glint/cli/command_io.h","<filesystem>","<sstream>"],"notes":["safe_deletion","dry_run_support","selective_cleaning"]}
// Human Summary
// Provides workspace cleanup functionality with dry-run support and selective directory removal.

#include "glint/cli/commands/clean_command.h"
#include "glint/cli/command_io.h"

#include <filesystem>
#include <sstream>

namespace glint::cli {

CLIExitCode CleanCommand::run(const CommandExecutionContext& context) {
    CleanOptions options;
    std::string errorMessage;

    CLIExitCode parseResult = parseArguments(context.arguments, options, errorMessage);
    if (parseResult != CLIExitCode::Success) {
        emitCommandFailed(context, parseResult, errorMessage, "argument_error");
        return parseResult;
    }

    return executeClean(context, options);
}

CLIExitCode CleanCommand::parseArguments(const std::vector<std::string>& args,
                                        CleanOptions& options,
                                        std::string& errorMessage) const {
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "--json") {
            // Already handled by dispatcher
            continue;
        }

        if (arg == "--dry-run" || arg == "-n") {
            options.dryRun = true;
        }
        else if (arg == "--renders") {
            options.cleanRenders = true;
        }
        else if (arg == "--cache") {
            options.cleanCache = true;
        }
        else if (arg == "--all") {
            options.cleanAll = true;
        }
        else if (arg == "--verbose" || arg == "-v") {
            options.verbose = true;
        }
        else if (!arg.empty() && arg[0] == '-') {
            errorMessage = "Unknown flag: " + arg;
            return CLIExitCode::UnknownFlag;
        }
        else {
            errorMessage = "Unexpected positional argument: " + arg;
            return CLIExitCode::RuntimeError;
        }
    }

    // If no specific targets, default to --all
    if (!options.cleanRenders && !options.cleanCache && !options.cleanAll) {
        options.cleanAll = true;
    }

    return CLIExitCode::Success;
}

CLIExitCode CleanCommand::executeClean(const CommandExecutionContext& context,
                                       const CleanOptions& options) const {
    size_t totalRemoved = 0;

    if (options.dryRun) {
        emitCommandInfo(context, "Dry run mode: no files will be deleted");
    }

    // Clean renders directory
    if (options.cleanRenders || options.cleanAll) {
        std::filesystem::path rendersPath("renders");
        if (std::filesystem::exists(rendersPath)) {
            size_t removed = removeDirectory(context, rendersPath, options.dryRun);
            totalRemoved += removed;

            if (options.verbose || options.dryRun) {
                std::ostringstream oss;
                oss << "Renders: " << removed << " item(s) "
                    << (options.dryRun ? "would be " : "") << "removed";
                emitCommandInfo(context, oss.str());
            }
        } else if (options.verbose) {
            emitCommandInfo(context, "Renders: directory not found (nothing to clean)");
        }
    }

    // Clean cache directory
    if (options.cleanCache || options.cleanAll) {
        std::filesystem::path cachePath(".glint/cache");
        if (std::filesystem::exists(cachePath)) {
            size_t removed = removeDirectory(context, cachePath, options.dryRun);
            totalRemoved += removed;

            if (options.verbose || options.dryRun) {
                std::ostringstream oss;
                oss << "Cache: " << removed << " item(s) "
                    << (options.dryRun ? "would be " : "") << "removed";
                emitCommandInfo(context, oss.str());
            }
        } else if (options.verbose) {
            emitCommandInfo(context, "Cache: directory not found (nothing to clean)");
        }
    }

    // Clean lock files
    if (options.cleanAll) {
        std::filesystem::path lockPath(".glint/.workspace.lock");
        if (std::filesystem::exists(lockPath)) {
            if (!options.dryRun) {
                std::filesystem::remove(lockPath);
            }
            totalRemoved++;

            if (options.verbose || options.dryRun) {
                std::ostringstream oss;
                oss << "Lock file: " << (options.dryRun ? "would be " : "") << "removed";
                emitCommandInfo(context, oss.str());
            }
        }
    }

    // Summary
    std::ostringstream summary;
    summary << "Clean completed: " << totalRemoved << " item(s) "
            << (options.dryRun ? "would be " : "") << "removed";
    emitCommandInfo(context, summary.str());

    return CLIExitCode::Success;
}

size_t CleanCommand::removeDirectory(const CommandExecutionContext& context,
                                     const std::filesystem::path& path,
                                     bool dryRun) const {
    if (!std::filesystem::exists(path)) {
        return 0;
    }

    size_t count = 0;

    try {
        // Count items
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            count++;
        }

        // Remove if not dry run
        if (!dryRun) {
            std::filesystem::remove_all(path);
        }

    } catch (const std::filesystem::filesystem_error& e) {
        std::ostringstream oss;
        oss << "Failed to clean " << path.string() << ": " << e.what();
        emitCommandWarning(context, oss.str());
    }

    return count;
}

} // namespace glint::cli
