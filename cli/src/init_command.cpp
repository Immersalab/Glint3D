// Machine Summary Block
// {"file":"cli/src/init_command.cpp","purpose":"Implements the glint init CLI command, argument parsing, and output.","depends_on":["glint/cli/init_command.h","application/cli_parser.h","rapidjson/stringbuffer.h","rapidjson/writer.h"],"notes":["manual_flag_parsing","ndjson_output_support","invokes_scaffolder"]}
// Human Summary
// Parses `glint init` arguments, validates inputs, generates scaffolding plans, and emits human-readable or NDJSON output before executing the plan.

#include "glint/cli/init_command.h"

#include "application/cli_parser.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace glint::cli {

InitCommand::InitCommand() = default;

int InitCommand::run(int argc, char** argv)
{
    auto parsed = parseArguments(argc, argv);
    if (!parsed.errors.empty()) {
        for (const auto& error : parsed.errors) {
            Logger::error(error);
        }
        return static_cast<int>(CLIExitCode::UnknownFlag);
    }

    try {
        auto plan = m_scaffolder.plan(parsed.request);
        if (parsed.request.jsonOutput) {
            emitJsonPlan(plan);
        } else {
            emitHumanPlan(plan);
        }
        auto result = m_scaffolder.execute(plan, parsed.request.dryRun);
        if (parsed.request.jsonOutput) {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            writer.StartObject();
            writer.Key("event"); writer.String(result.executed ? "init_completed" : "init_planned");
            writer.Key("dry_run"); writer.Bool(parsed.request.dryRun);
            writer.Key("operations"); writer.Uint(static_cast<unsigned int>(result.plan.operations.size()));
            writer.Key("next_steps");
            writer.StartArray();
            for (const auto& step : result.plan.nextSteps) {
                writer.String(step.c_str());
            }
            writer.EndArray();
            writer.EndObject();
            std::cout << buffer.GetString() << std::endl;
        } else {
            Logger::info(result.executed ? "Workspace scaffolded successfully." : "Dry-run complete. No changes were written.");
            Logger::info("Next steps:");
            for (const auto& step : result.plan.nextSteps) {
                Logger::info("  " + step);
            }
        }
    } catch (const std::exception& ex) {
        Logger::error(ex.what());
        return static_cast<int>(CLIExitCode::RuntimeError);
    }

    return static_cast<int>(CLIExitCode::Success);
}

InitCommand::ParsedArgs InitCommand::parseArguments(int argc, char** argv) const
{
    ParsedArgs parsed;

    for (int i = 2; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--workspace" && i + 1 < argc) {
            parsed.request.workspaceRoot = argv[++i];
            continue;
        }
        if (arg == "--template" && i + 1 < argc) {
            parsed.request.templateName = argv[++i];
            continue;
        }
        if (arg == "--module" && i + 1 < argc) {
            parsed.request.modules.emplace_back(argv[++i]);
            continue;
        }
        if (arg == "--asset-pack" && i + 1 < argc) {
            parsed.request.assetPacks.emplace_back(argv[++i]);
            continue;
        }
        if (arg == "--with-samples") {
            parsed.request.withSamples = true;
            continue;
        }
        if (arg == "--force") {
            parsed.request.force = true;
            continue;
        }
        if (arg == "--no-config") {
            parsed.request.noConfig = true;
            continue;
        }
        if (arg == "--json") {
            parsed.request.jsonOutput = true;
            continue;
        }
        if (arg == "--dry-run") {
            parsed.request.dryRun = true;
            continue;
        }
        parsed.errors.emplace_back("Unknown argument: " + arg);
    }

    return parsed;
}

void InitCommand::emitJsonPlan(const InitPlan& plan) const
{
    for (const auto& op : plan.operations) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        writer.Key("event"); writer.String("init_operation");
        writer.Key("type");
        switch (op.type) {
        case InitOperationType::CreateDirectory: writer.String("create_directory"); break;
        case InitOperationType::CopyTemplateFile: writer.String("copy_template_file"); break;
        case InitOperationType::WriteFile: writer.String("write_file"); break;
        }
        writer.Key("destination"); writer.String(op.destinationPath.string().c_str());
        if (!op.sourcePath.empty()) {
            writer.Key("source"); writer.String(op.sourcePath.string().c_str());
        }
        if (!op.contents.empty()) {
            writer.Key("bytes"); writer.Uint64(op.contents.size());
        }
        writer.EndObject();
        std::cout << buffer.GetString() << std::endl;
    }
}

void InitCommand::emitHumanPlan(const InitPlan& plan) const
{
    Logger::info("Planned operations:");
    for (const auto& op : plan.operations) {
        switch (op.type) {
        case InitOperationType::CreateDirectory:
            Logger::info("  mkdir " + op.destinationPath.string());
            break;
        case InitOperationType::CopyTemplateFile:
            Logger::info("  copy template:" + op.sourcePath.string() + " -> " + op.destinationPath.string());
            break;
        case InitOperationType::WriteFile:
            Logger::info("  write " + op.destinationPath.string() + " (" + std::to_string(op.contents.size()) + " bytes)");
            break;
        }
    }
}

} // namespace glint::cli
