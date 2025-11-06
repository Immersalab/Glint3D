// Machine Summary Block
// {"file":"cli/include/glint/cli/command_dispatcher.h","purpose":"Declares the dispatcher and shared context for Glint CLI verbs.","exports":["glint::cli::GlobalOptions","glint::cli::CommandExecutionContext","glint::cli::ICommand","glint::cli::CommandDispatcher"],"depends_on":["glint/cli/ndjson_emitter.h","application/cli_parser.h","<memory>","<optional>","<string>","<vector>"],"notes":["phase2_cli_scaffolding","structured_output","global_flag_handling"]}
// Human Summary
// Defines the command dispatcher that parses global flags, prepares execution context, and routes verbs to their handlers.

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "glint/cli/ndjson_emitter.h"

#include "application/cli_parser.h"

/**
 * @file command_dispatcher.h
 * @brief CLI command routing and shared execution context helpers.
 */

namespace glint::cli {

/// @brief Global flags applicable to all CLI verbs.
struct GlobalOptions {
    bool jsonOutput = false;              ///< Emit NDJSON events when true.
    LogLevel logLevel = LogLevel::Info;   ///< Verbose logging control.
    std::string projectPath;              ///< Optional project manifest override.
    std::string configPath;               ///< Optional CLI config override.
};

/// @brief Shared context passed to individual command implementations.
struct CommandExecutionContext {
    std::string verb;                             ///< Command name (e.g., "render").
    GlobalOptions globals;                        ///< Snapshot of global options.
    NdjsonEmitter* emitter = nullptr;             ///< NDJSON emitter (non-owning, may be null when jsonOutput is false).
    std::vector<std::string> arguments;           ///< Remaining argv tokens after global parsing.
};

/**
 * @brief Interface implemented by all CLI verbs.
 */
class ICommand {
public:
    virtual ~ICommand() = default;

    /**
     * @brief Execute the command.
     * @param context Shared flags, emitter, and remaining arguments.
     * @return CLIExitCode describing the outcome.
     */
    virtual CLIExitCode run(const CommandExecutionContext& context) = 0;
};

/**
 * @brief Routes `glint <verb>` invocations to command implementations.
 *
 * The dispatcher accepts raw `argc`/`argv`, parses recognized global flags, and
 * hands off execution to the appropriate `ICommand`. Unknown verbs fall back to
 * the legacy CLI parser handled elsewhere in the application.
 */
class CommandDispatcher {
public:
    /**
     * @brief Attempt to dispatch the supplied command line.
     * @param argc Argument count.
     * @param argv Argument values.
     * @return Exit code when a supported verb was executed, or `std::nullopt` if the dispatcher does not handle the command.
     */
    std::optional<int> tryRun(int argc, char** argv) const;

private:
    struct ParsedArgs {
        std::string verb;
        GlobalOptions globals;
        std::vector<std::string> commandArgs;
    };

    struct ParseOutcome {
        bool recognized = false;          ///< True when the verb matches a supported command.
        bool success = false;             ///< True when parsing succeeded and arguments are ready.
        CLIExitCode errorCode = CLIExitCode::UnknownFlag;
        std::string errorMessage;
        ParsedArgs parsed;
    };

    static bool isSupportedVerb(const std::string& verb);
    static ParseOutcome parseArguments(int argc, char** argv);
    static std::unique_ptr<ICommand> createCommand(const std::string& verb);
    static CLIExitCode executeCommand(ICommand& command,
                                      const ParsedArgs& parsed,
                                      NdjsonEmitter* emitter);
};

} // namespace glint::cli
