// Machine Summary Block
// {"file":"cli/include/glint/cli/init_command.h","purpose":"Declares the glint init CLI command entry point.","exports":["glint::cli::InitCommand"],"depends_on":["glint/cli/init_scaffolder.h","glint/cli/config_resolver.h","<vector>","<string>"],"notes":["manual_arg_parsing","ndjson_output","uses_scaffolder_plan"]}
// Human Summary
// Front-end command that parses `glint init` arguments, resolves configuration, generates scaffolding plans, and emits structured output.

#pragma once

#include "glint/cli/config_resolver.h"
#include "glint/cli/init_scaffolder.h"

#include <string>
#include <vector>

/**
 * @file init_command.h
 * @brief CLI entry point for `glint init`.
 */

namespace glint::cli {

/**
 * @brief Handles argument parsing and execution for `glint init`.
 */
class InitCommand {
public:
    InitCommand();

    /**
     * @brief Execute the command using standard argc/argv inputs.
     * @return Process exit code (0 success, otherwise CLIExitCode-compatible).
     */
    int run(int argc, char** argv);

private:
    /// @brief Holds parsed arguments and validation errors.
    struct ParsedArgs {
        InitRequest request;
        std::vector<std::string> errors;
    };

    ParsedArgs parseArguments(int argc, char** argv) const;
    void emitJsonPlan(const InitPlan& plan) const;
    void emitHumanPlan(const InitPlan& plan) const;

    ConfigResolver m_configResolver;
    InitScaffolder m_scaffolder;
};

} // namespace glint::cli
