// Machine Summary Block
// {"file":"cli/src/command_io.cpp","purpose":"Implements command lifecycle event helpers bridging logger and NDJSON output.","depends_on":["glint/cli/command_io.h","glint/cli/ndjson_emitter.h","<sstream>"],"notes":["shared_event_helpers","ndjson_bridge","logger_passthrough"]}
// Human Summary
// Provides reusable helpers for commands to emit start/completion/failure events consistently in both human-readable and NDJSON modes.

#include "glint/cli/command_io.h"

#include <sstream>

namespace glint::cli {

namespace {

bool usesJson(const CommandExecutionContext& context)
{
    return context.globals.jsonOutput && context.emitter != nullptr;
}

} // namespace

void emitCommandStarted(const CommandExecutionContext& context)
{
    if (usesJson(context)) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("command_started");
            writer.Key("command"); writer.String(context.verb.c_str());
        });
    } else {
        Logger::info("Running glint " + context.verb);
    }
}

void emitCommandCompleted(const CommandExecutionContext& context, CLIExitCode exitCode)
{
    if (usesJson(context)) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("command_completed");
            writer.Key("command"); writer.String(context.verb.c_str());
            writer.Key("exit_code"); writer.Int(static_cast<int>(exitCode));
            writer.Key("exit_code_name"); writer.String(CLIParser::exitCodeToString(exitCode));
        });
    } else {
        std::ostringstream oss;
        oss << "glint " << context.verb << " completed with exit code "
            << static_cast<int>(exitCode)
            << " (" << CLIParser::exitCodeToString(exitCode) << ")";
        Logger::info(oss.str());
    }
}

void emitCommandFailed(const CommandExecutionContext& context,
                       CLIExitCode exitCode,
                       const std::string& message,
                       const std::string& status)
{
    if (usesJson(context)) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("command_failed");
            writer.Key("command"); writer.String(context.verb.c_str());
            writer.Key("status"); writer.String(status.c_str());
            writer.Key("exit_code"); writer.Int(static_cast<int>(exitCode));
            writer.Key("message"); writer.String(message.c_str());
        });
    } else {
        Logger::error(message);
    }
}

void emitCommandInfo(const CommandExecutionContext& context, const std::string& message)
{
    if (usesJson(context)) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("command_info");
            writer.Key("command"); writer.String(context.verb.c_str());
            writer.Key("message"); writer.String(message.c_str());
        });
    } else {
        Logger::info(message);
    }
}

void emitCommandWarning(const CommandExecutionContext& context, const std::string& message)
{
    if (usesJson(context)) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("command_warning");
            writer.Key("command"); writer.String(context.verb.c_str());
            writer.Key("message"); writer.String(message.c_str());
        });
    } else {
        Logger::warn(message);
    }
}

} // namespace glint::cli
