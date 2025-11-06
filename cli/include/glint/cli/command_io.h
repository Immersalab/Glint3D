// Machine Summary Block
// {"file":"cli/include/glint/cli/command_io.h","purpose":"Declares helpers for emitting command lifecycle events and logging.","exports":["glint::cli::emitCommandStarted","glint::cli::emitCommandCompleted","glint::cli::emitCommandFailed","glint::cli::emitCommandInfo","glint::cli::emitCommandWarning"],"depends_on":["glint/cli/command_dispatcher.h","glint/cli/ndjson_emitter.h","application/cli_parser.h","<string>"],"notes":["ndjson_event_helpers","logger_bridge","shared_command_utilities"]}
// Human Summary
// Shared utilities that bridge Logger output with NDJSON emissions to keep CLI command lifecycle events consistent.

#pragma once

#include <string>

#include "glint/cli/command_dispatcher.h"

#include "application/cli_parser.h"

/**
 * @file command_io.h
 * @brief Helpers for emitting structured command lifecycle events.
 */

namespace glint::cli {

/// @brief Emit a `command_started` event (or informational log in human mode).
void emitCommandStarted(const CommandExecutionContext& context);

/// @brief Emit a `command_completed` event and summary.
void emitCommandCompleted(const CommandExecutionContext& context, CLIExitCode exitCode);

/// @brief Emit a `command_failed` event with a reason and optional status string.
void emitCommandFailed(const CommandExecutionContext& context,
                       CLIExitCode exitCode,
                       const std::string& message,
                       const std::string& status = "error");

/// @brief Emit a structured informational message.
void emitCommandInfo(const CommandExecutionContext& context, const std::string& message);

/// @brief Emit a structured warning.
void emitCommandWarning(const CommandExecutionContext& context, const std::string& message);

} // namespace glint::cli
