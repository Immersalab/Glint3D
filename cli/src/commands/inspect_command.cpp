// Machine Summary Block
// {"file":"cli/src/commands/inspect_command.cpp","purpose":"Implements the Glint CLI inspect command for scene and manifest introspection.","depends_on":["glint/cli/commands/inspect_command.h","glint/cli/command_io.h","<filesystem>","<fstream>","<sstream>","rapidjson/document.h","rapidjson/error/en.h"],"notes":["scene_metadata_extraction","manifest_validation","json_parsing"]}
// Human Summary
// Provides introspection capabilities for scene files, project manifests, and run manifests with structured output support.

#include "glint/cli/commands/inspect_command.h"
#include "glint/cli/command_io.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace glint::cli {

namespace {

std::string readFileToString(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

std::string getFileExtension(const std::string& path)
{
    std::filesystem::path p(path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

} // namespace

CLIExitCode InspectCommand::run(const CommandExecutionContext& context)
{
    InspectOptions options;
    std::string errorMessage;

    CLIExitCode parseResult = parseArguments(context.arguments, options, errorMessage);
    if (parseResult != CLIExitCode::Success) {
        emitCommandFailed(context, parseResult, errorMessage, "argument_error");
        return parseResult;
    }

    // Dispatch based on target type
    switch (options.targetType) {
        case InspectTarget::Scene:
            return inspectScene(context, options);
        case InspectTarget::ProjectManifest:
            return inspectProjectManifest(context, options);
        case InspectTarget::RunManifest:
            return inspectRunManifest(context, options);
        default:
            emitCommandFailed(context, CLIExitCode::RuntimeError,
                            "Unable to determine inspection target type",
                            "unknown_target");
            return CLIExitCode::RuntimeError;
    }
}

CLIExitCode InspectCommand::parseArguments(const std::vector<std::string>& args,
                                          InspectOptions& options,
                                          std::string& errorMessage) const
{
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "--json") {
            // Already handled by dispatcher
            continue;
        }

        if (arg == "--verbose" || arg == "-v") {
            options.verbose = true;
        }
        else if (!arg.empty() && arg[0] == '-') {
            errorMessage = "Unknown flag: " + arg;
            return CLIExitCode::UnknownFlag;
        }
        else {
            // Positional argument (target path)
            if (options.targetPath.empty()) {
                options.targetPath = arg;
            } else {
                errorMessage = "Multiple target paths specified";
                return CLIExitCode::RuntimeError;
            }
        }
    }

    if (options.targetPath.empty()) {
        errorMessage = "Missing required target path";
        return CLIExitCode::UnknownFlag;
    }

    // Verify file exists
    if (!std::filesystem::exists(options.targetPath)) {
        errorMessage = "Target file not found: " + options.targetPath;
        return CLIExitCode::FileNotFound;
    }

    // Determine target type
    options.targetType = determineTargetType(options.targetPath);

    return CLIExitCode::Success;
}

InspectCommand::InspectTarget InspectCommand::determineTargetType(const std::string& path) const
{
    std::filesystem::path p(path);
    std::string filename = p.filename().string();

    // Check for project manifest
    if (filename == "glint.project.json") {
        return InspectTarget::ProjectManifest;
    }

    // Check for run manifest
    if (filename == "run.json" && p.parent_path().parent_path().filename() == "renders") {
        return InspectTarget::RunManifest;
    }

    // Check for scene files
    std::string ext = getFileExtension(path);
    if (ext == ".obj" || ext == ".glb" || ext == ".gltf" || ext == ".fbx" ||
        ext == ".dae" || ext == ".ply" || ext == ".stl") {
        return InspectTarget::Scene;
    }

    return InspectTarget::Unknown;
}

CLIExitCode InspectCommand::inspectScene(const CommandExecutionContext& context,
                                        const InspectOptions& options) const
{
    try {
        std::filesystem::path path(options.targetPath);
        auto fileSize = std::filesystem::file_size(path);

        std::ostringstream oss;
        oss << "Scene file: " << path.filename().string() << "\n";
        oss << "Format: " << getFileExtension(options.targetPath).substr(1) << "\n";
        oss << "Size: " << fileSize << " bytes\n";

        // TODO: Load scene and extract detailed metadata
        // - Vertex count
        // - Triangle count
        // - Material count
        // - Texture count
        // - Bounding box
        // - Animation data (if present)

        oss << "Note: Detailed scene analysis requires engine integration";

        emitCommandInfo(context, oss.str());
        return CLIExitCode::Success;

    } catch (const std::exception& e) {
        emitCommandFailed(context, CLIExitCode::RuntimeError,
                        std::string("Scene inspection failed: ") + e.what(),
                        "inspection_error");
        return CLIExitCode::RuntimeError;
    }
}

CLIExitCode InspectCommand::inspectProjectManifest(const CommandExecutionContext& context,
                                                   const InspectOptions& options) const
{
    try {
        std::string content = readFileToString(options.targetPath);

        rapidjson::Document doc;
        rapidjson::ParseResult parseResult = doc.Parse(content.c_str());

        if (!parseResult) {
            std::ostringstream oss;
            oss << "JSON parse error: "
                << rapidjson::GetParseError_En(parseResult.Code())
                << " at offset " << parseResult.Offset();
            emitCommandFailed(context, CLIExitCode::SchemaValidationError,
                            oss.str(), "json_parse_error");
            return CLIExitCode::SchemaValidationError;
        }

        std::ostringstream oss;
        oss << "Project manifest: " << options.targetPath << "\n";

        if (doc.HasMember("name") && doc["name"].IsString()) {
            oss << "Project name: " << doc["name"].GetString() << "\n";
        }

        if (doc.HasMember("version") && doc["version"].IsString()) {
            oss << "Version: " << doc["version"].GetString() << "\n";
        }

        if (doc.HasMember("schema_version") && doc["schema_version"].IsString()) {
            oss << "Schema version: " << doc["schema_version"].GetString() << "\n";
        }

        // TODO: Validate against schema
        // TODO: Check for required fields
        // TODO: Report warnings for deprecated fields

        emitCommandInfo(context, oss.str());
        return CLIExitCode::Success;

    } catch (const std::exception& e) {
        emitCommandFailed(context, CLIExitCode::RuntimeError,
                        std::string("Project manifest inspection failed: ") + e.what(),
                        "inspection_error");
        return CLIExitCode::RuntimeError;
    }
}

CLIExitCode InspectCommand::inspectRunManifest(const CommandExecutionContext& context,
                                              const InspectOptions& options) const
{
    try {
        std::string content = readFileToString(options.targetPath);

        rapidjson::Document doc;
        rapidjson::ParseResult parseResult = doc.Parse(content.c_str());

        if (!parseResult) {
            std::ostringstream oss;
            oss << "JSON parse error: "
                << rapidjson::GetParseError_En(parseResult.Code())
                << " at offset " << parseResult.Offset();
            emitCommandFailed(context, CLIExitCode::SchemaValidationError,
                            oss.str(), "json_parse_error");
            return CLIExitCode::SchemaValidationError;
        }

        std::ostringstream oss;
        oss << "Run manifest: " << options.targetPath << "\n";

        if (doc.HasMember("run_id") && doc["run_id"].IsString()) {
            oss << "Run ID: " << doc["run_id"].GetString() << "\n";
        }

        if (doc.HasMember("timestamp_utc") && doc["timestamp_utc"].IsString()) {
            oss << "Timestamp: " << doc["timestamp_utc"].GetString() << "\n";
        }

        if (doc.HasMember("cli") && doc["cli"].IsObject()) {
            const auto& cli = doc["cli"];
            if (cli.HasMember("command") && cli["command"].IsString()) {
                oss << "Command: " << cli["command"].GetString() << "\n";
            }
        }

        if (doc.HasMember("platform") && doc["platform"].IsObject()) {
            const auto& platform = doc["platform"];
            if (platform.HasMember("os") && platform["os"].IsString()) {
                oss << "Platform: " << platform["os"].GetString() << "\n";
            }
        }

        if (doc.HasMember("engine") && doc["engine"].IsObject()) {
            const auto& engine = doc["engine"];
            if (engine.HasMember("version") && engine["version"].IsString()) {
                oss << "Engine version: " << engine["version"].GetString() << "\n";
            }
        }

        if (doc.HasMember("outputs") && doc["outputs"].IsObject()) {
            const auto& outputs = doc["outputs"];
            if (outputs.HasMember("frames") && outputs["frames"].IsArray()) {
                oss << "Frame count: " << outputs["frames"].Size() << "\n";
            }
        }

        // TODO: Validate against schema
        // TODO: Verify checksums
        // TODO: Check determinism fields

        emitCommandInfo(context, oss.str());
        return CLIExitCode::Success;

    } catch (const std::exception& e) {
        emitCommandFailed(context, CLIExitCode::RuntimeError,
                        std::string("Run manifest inspection failed: ") + e.what(),
                        "inspection_error");
        return CLIExitCode::RuntimeError;
    }
}

} // namespace glint::cli
