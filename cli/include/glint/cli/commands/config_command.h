
// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/config_command.h","purpose":"Declares the CLI command for inspecting and mutating configuration layers.","exports":["glint::cli::ConfigCommand"],"depends_on":["glint/cli/command_dispatcher.h","glint/cli/command_io.h","glint/cli/config_resolver.h","glint/cli/services/project_manifest.h","<filesystem>","<optional>","<string>","<vector>"],"notes":["config_precedence","ndjson_diff_events","workspace_config_writes"]}
// Human Summary
// Implements the `glint config` verb, supporting reads, writes, and provenance inspection across configuration scopes.

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "glint/cli/command_dispatcher.h"
#include "glint/cli/command_io.h"
#include "glint/cli/config_resolver.h"
#include "glint/cli/services/project_manifest.h"

/**
 * @file config_command.h
 * @brief CLI command for configuration inspection and mutation.
 */

namespace glint::cli {

/**
 * @brief Exposes configuration inspection (`--get`, `--show`) and mutation (`--set`, `--unset`).
 */
class ConfigCommand : public ICommand {
public:
    ConfigCommand();

    /// @brief Execute the config command.
    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    enum class Operation {
        Show,
        Get,
        Set,
        Unset
    };

    struct ParsedArgs {
        Operation operation = Operation::Show;
        std::optional<std::string> key;
        std::optional<std::string> value; ///< JSON literal or string for --set.
        std::string scope = "workspace";  ///< workspace | project | global.
        std::optional<std::string> sceneId;
        bool explain = false;
        std::filesystem::path manifestPath;
    };

    std::optional<ParsedArgs> parseArguments(const CommandExecutionContext& context,
                                             CLIExitCode& errorCode,
                                             std::string& errorMessage) const;

    CLIExitCode executeShow(const CommandExecutionContext& context,
                            const ParsedArgs& args,
                            const services::ProjectManifest& manifest) const;
    CLIExitCode executeGet(const CommandExecutionContext& context,
                           const ParsedArgs& args,
                           const services::ProjectManifest& manifest) const;
    CLIExitCode executeSet(const CommandExecutionContext& context,
                           const ParsedArgs& args,
                           const services::ProjectManifest& manifest) const;
    CLIExitCode executeUnset(const CommandExecutionContext& context,
                             const ParsedArgs& args,
                             const services::ProjectManifest& manifest) const;
};

} // namespace glint::cli
