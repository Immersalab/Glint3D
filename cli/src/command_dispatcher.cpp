// Machine Summary Block
// {"file":"cli/src/command_dispatcher.cpp","purpose":"Implements the Glint CLI command dispatcher and stub verbs.","depends_on":["glint/cli/command_dispatcher.h","glint/cli/init_command.h","glint/cli/command_io.h","glint/cli/commands/validate_command.h","glint/cli/commands/config_command.h","glint/cli/commands/doctor_command.h","glint/cli/ndjson_emitter.h","<algorithm>","<iostream>","<optional>","<sstream>"],"notes":["phase2_scaffolding","ndjson_events","verb_registration"]}
// Human Summary
// Parses global flags, instantiates command handlers, and wires placeholder implementations for the remaining CLI verbs while reusing the existing init command.

#include "glint/cli/command_dispatcher.h"

#include "glint/cli/init_command.h"
#include "glint/cli/command_io.h"
#include "glint/cli/commands/validate_command.h"
#include "glint/cli/commands/config_command.h"
#include "glint/cli/commands/doctor_command.h"
#include "glint/cli/commands/modules_command.h"
#include "glint/cli/commands/assets_command.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <sstream>

namespace glint::cli {

namespace {

enum class GlobalFlagResult {
    NotGlobal,
    Consumed,
    Error
};

std::optional<LogLevel> parseVerbosityLevel(const std::string& value)
{
    if (value == "quiet") return LogLevel::Quiet;
    if (value == "warn") return LogLevel::Warn;
    if (value == "info") return LogLevel::Info;
    if (value == "debug") return LogLevel::Debug;
    return std::nullopt;
}

GlobalFlagResult parseGlobalFlag(const std::vector<std::string>& tokens,
                                 size_t& index,
                                 GlobalOptions& globals,
                                 CLIExitCode& errorCode,
                                 std::string& errorMessage)
{
    const std::string& token = tokens[index];
    if (token == "--verbosity") {
        if (index + 1 >= tokens.size()) {
            errorCode = CLIExitCode::UnknownFlag;
            errorMessage = "Missing value for --verbosity (expected quiet|warn|info|debug)";
            return GlobalFlagResult::Error;
        }
        auto level = parseVerbosityLevel(tokens[index + 1]);
        if (!level.has_value()) {
            errorCode = CLIExitCode::UnknownFlag;
            errorMessage = "Invalid verbosity level: " + tokens[index + 1];
            return GlobalFlagResult::Error;
        }
        globals.logLevel = *level;
        index += 1;
        return GlobalFlagResult::Consumed;
    }

    if (token == "--project") {
        if (index + 1 >= tokens.size()) {
            errorCode = CLIExitCode::UnknownFlag;
            errorMessage = "Missing value for --project (expected path to glint.project.json)";
            return GlobalFlagResult::Error;
        }
        globals.projectPath = tokens[index + 1];
        index += 1;
        return GlobalFlagResult::Consumed;
    }

    if (token == "--config") {
        if (index + 1 >= tokens.size()) {
            errorCode = CLIExitCode::UnknownFlag;
            errorMessage = "Missing value for --config (expected path to .glint/config.json)";
            return GlobalFlagResult::Error;
        }
        globals.configPath = tokens[index + 1];
        index += 1;
        return GlobalFlagResult::Consumed;
    }

    return GlobalFlagResult::NotGlobal;
}

std::string formatArguments(const std::vector<std::string>& args)
{
    if (args.empty()) {
        return "(no additional arguments)";
    }

    std::ostringstream oss;
    oss << "arguments: ";
    bool first = true;
    for (const auto& arg : args) {
        if (!first) {
            oss << ' ';
        }
        oss << arg;
        first = false;
    }
    return oss.str();
}

class InitCommandAdapter : public ICommand {
public:
    CLIExitCode run(const CommandExecutionContext& context) override
    {
        std::vector<std::string> storage;
        storage.reserve(context.arguments.size() + 2);
        storage.emplace_back("glint");
        storage.emplace_back("init");
        storage.insert(storage.end(), context.arguments.begin(), context.arguments.end());

        std::vector<char*> argv;
        argv.reserve(storage.size());
        for (auto& token : storage) {
            argv.push_back(token.data());
        }

        InitCommand command;
        int exitCode = command.run(static_cast<int>(argv.size()), argv.data());
        CLIExitCode code = static_cast<CLIExitCode>(exitCode);
        if (code != CLIExitCode::Success) {
            std::ostringstream oss;
            oss << "glint init failed with exit code "
                << static_cast<int>(code)
                << " (" << CLIParser::exitCodeToString(code) << ")";
            emitCommandFailed(context, code, oss.str(), "command_failed");
        }
        return code;
    }
};

class StubCommand : public ICommand {
public:
    explicit StubCommand(std::string /*verb*/) {}

    CLIExitCode run(const CommandExecutionContext& context) override
    {
        if (!context.arguments.empty()) {
            emitCommandInfo(context, formatArguments(context.arguments));
        }
        std::string message = "glint " + context.verb
            + " scaffolding is present but implementation is not yet complete.";
        emitCommandFailed(context, CLIExitCode::RuntimeError, message, "not_implemented");
        return CLIExitCode::RuntimeError;
    }
};

} // namespace

std::optional<int> CommandDispatcher::tryRun(int argc, char** argv) const
{
    auto outcome = parseArguments(argc, argv);
    if (!outcome.recognized) {
        return std::nullopt;
    }
    if (!outcome.success) {
        Logger::error(outcome.errorMessage);
        return static_cast<int>(outcome.errorCode);
    }

    Logger::setLevel(outcome.parsed.globals.logLevel);

    std::unique_ptr<ICommand> command = createCommand(outcome.parsed.verb);
    if (!command) {
        return std::nullopt;
    }

    std::optional<NdjsonEmitter> emitter;
    if (outcome.parsed.globals.jsonOutput) {
        emitter.emplace(std::cout);
    }

    CLIExitCode code = executeCommand(*command,
                                      outcome.parsed,
                                      emitter ? &*emitter : nullptr);
    return static_cast<int>(code);
}

bool CommandDispatcher::isSupportedVerb(const std::string& verb)
{
    static const std::vector<std::string> verbs = {
        "init",
        "validate",
        "inspect",
        "render",
        "config",
        "clean",
        "doctor",
        "modules",
        "assets"
    };
    return std::find(verbs.begin(), verbs.end(), verb) != verbs.end();
}

CommandDispatcher::ParseOutcome CommandDispatcher::parseArguments(int argc, char** argv)
{
    ParseOutcome outcome;
    if (argc < 2) {
        return outcome;
    }

    std::vector<std::string> tokens;
    tokens.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        tokens.emplace_back(argv[i]);
    }

    GlobalOptions globals;
    std::vector<std::string> prefixArgs;
    size_t commandIndex = std::string::npos;

    CLIExitCode errorCode = CLIExitCode::UnknownFlag;
    std::string errorMessage;

    size_t i = 1;
    while (i < tokens.size()) {
        const std::string& token = tokens[i];
        if (!token.empty() && token[0] != '-') {
            commandIndex = i;
            break;
        }

        auto flagResult = parseGlobalFlag(tokens, i, globals, errorCode, errorMessage);
        if (flagResult == GlobalFlagResult::Error) {
            outcome.recognized = true;
            outcome.success = false;
            outcome.errorCode = errorCode;
            outcome.errorMessage = errorMessage;
            return outcome;
        }
        if (flagResult == GlobalFlagResult::Consumed) {
            ++i;
            continue;
        }

        // Not a recognized global flag (e.g., --json before command); treat as prefix arg.
        prefixArgs.emplace_back(token);
        ++i;
    }

    if (commandIndex == std::string::npos) {
        return outcome;
    }

    const std::string& verb = tokens[commandIndex];
    outcome.recognized = isSupportedVerb(verb);
    if (!outcome.recognized) {
        return outcome;
    }

    std::vector<std::string> commandArgs = prefixArgs;
    i = commandIndex + 1;
    while (i < tokens.size()) {
        auto flagResult = parseGlobalFlag(tokens, i, globals, errorCode, errorMessage);
        if (flagResult == GlobalFlagResult::Error) {
            outcome.success = false;
            outcome.errorCode = errorCode;
            outcome.errorMessage = errorMessage;
            return outcome;
        }
        if (flagResult == GlobalFlagResult::Consumed) {
            ++i;
            continue;
        }

        commandArgs.emplace_back(tokens[i]);
        ++i;
    }

    // Inspect command arguments for --json flag to toggle NDJSON output.
    if (std::find(commandArgs.begin(), commandArgs.end(), "--json") != commandArgs.end()) {
        globals.jsonOutput = true;
    }

    outcome.success = true;
    outcome.parsed.verb = verb;
    outcome.parsed.globals = globals;
    outcome.parsed.commandArgs = std::move(commandArgs);
    return outcome;
}

std::unique_ptr<ICommand> CommandDispatcher::createCommand(const std::string& verb)
{
    if (verb == "init") {
        return std::make_unique<InitCommandAdapter>();
    }
    if (verb == "validate") {
        return std::make_unique<ValidateCommand>();
    }
    if (verb == "config") {
        return std::make_unique<ConfigCommand>();
    }
    if (verb == "doctor") {
        return std::make_unique<DoctorCommand>();
    }
    if (verb == "modules") {
        return std::make_unique<ModulesCommand>();
    }
    if (verb == "assets") {
        return std::make_unique<AssetsCommand>();
    }
    return std::make_unique<StubCommand>(verb);
}

CLIExitCode CommandDispatcher::executeCommand(ICommand& command,
                                              const ParsedArgs& parsed,
                                              NdjsonEmitter* emitter)
{
    CommandExecutionContext context;
    context.verb = parsed.verb;
    context.globals = parsed.globals;
    context.emitter = emitter;
    context.arguments = parsed.commandArgs;

    emitCommandStarted(context);
    CLIExitCode exitCode = command.run(context);
    emitCommandCompleted(context, exitCode);
    return exitCode;
}

} // namespace glint::cli
