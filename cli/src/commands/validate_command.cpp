
// Machine Summary Block
// {"file":"cli/src/commands/validate_command.cpp","purpose":"Implements the glint validate command with manifest, module, and asset checks.","depends_on":["glint/cli/commands/validate_command.h","glint/cli/command_io.h","glint/cli/services/project_manifest.h","glint/cli/services/workspace_locks.h","<filesystem>","<sstream>"],"notes":["schema_validation","lockfile_consistency","ndjson_events"]}
// Human Summary
// Performs project validation by loading the manifest, checking scene paths, verifying lockfiles, and emitting structured NDJSON events.

#include "glint/cli/commands/validate_command.h"

#include <filesystem>
#include <sstream>

namespace glint::cli {

namespace {

constexpr const char* kDefaultManifestName = "glint.project.json";
constexpr const char* kValidationPhaseManifest = "manifest";
constexpr const char* kValidationPhaseScenes = "scenes";
constexpr const char* kValidationPhaseModules = "modules";
constexpr const char* kValidationPhaseAssets = "assets";
constexpr const char* kSupportedSchemaVersion = "1.0.0";

std::filesystem::path defaultManifestPath()
{
    return std::filesystem::current_path() / kDefaultManifestName;
}

bool fileExists(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

} // namespace

ValidateCommand::ValidateCommand() = default;

CLIExitCode ValidateCommand::run(const CommandExecutionContext& context)
{
    CLIExitCode errorCode = CLIExitCode::UnknownFlag;
    std::string errorMessage;
    auto parsed = parseArguments(context, errorCode, errorMessage);
    if (!parsed.has_value()) {
        emitCommandFailed(context, errorCode, errorMessage);
        return errorCode;
    }

    try {
        CLIExitCode exitCode = CLIExitCode::Success;
        ValidationReport report = validateProject(context, *parsed, exitCode);
        if (exitCode == CLIExitCode::Success) {
            std::ostringstream summary;
            summary << "Validated " << report.scenesValidated << " scene(s)";
            if (parsed->validateModules) {
                summary << ", " << report.modulesValidated << " module(s)";
            }
            if (parsed->validateAssets) {
                summary << ", " << report.assetsValidated << " asset pack(s)";
            }
            summary << ".";
            emitCommandInfo(context, summary.str());
        }
        return exitCode;
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }
}

std::optional<ValidateCommand::ParsedArgs> ValidateCommand::parseArguments(
    const CommandExecutionContext& context,
    CLIExitCode& errorCode,
    std::string& errorMessage) const
{
    ParsedArgs args;
    args.manifestPath = context.globals.projectPath.empty()
        ? defaultManifestPath()
        : std::filesystem::path(context.globals.projectPath);

    const auto& tokens = context.arguments;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        if (token == "--json") {
            continue; // Global flag already handled.
        }
        if (token == "--strict") {
            args.strict = true;
            continue;
        }
        if (token == "--modules") {
            args.validateModules = true;
            continue;
        }
        if (token == "--assets") {
            args.validateAssets = true;
            continue;
        }
        if (token == "--scene") {
            if (i + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --scene (expected scene identifier)";
                return std::nullopt;
            }
            args.sceneId = tokens[++i];
            continue;
        }
        if (token == "--project") {
            if (i + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --project (expected path to glint.project.json)";
                return std::nullopt;
            }
            args.manifestPath = tokens[++i];
            continue;
        }

        errorCode = CLIExitCode::UnknownFlag;
        errorMessage = "Unknown argument for glint validate: " + token;
        return std::nullopt;
    }

    return args;
}

ValidateCommand::ValidationReport ValidateCommand::validateProject(
    const CommandExecutionContext& context,
    const ParsedArgs& args,
    CLIExitCode& exitCode) const
{
    ValidationReport report;

    if (!fileExists(args.manifestPath)) {
        exitCode = CLIExitCode::FileNotFound;
        std::ostringstream oss;
        oss << "Project manifest not found: " << args.manifestPath.string();
        emitCommandFailed(context, exitCode, oss.str(), "manifest_missing");
        return report;
    }

    services::ProjectManifest manifest = services::ProjectManifestLoader::load(args.manifestPath);
    emitPhaseEvent(context, kValidationPhaseManifest, "loaded");

    if (args.strict && manifest.schemaVersion != kSupportedSchemaVersion) {
        exitCode = CLIExitCode::SchemaValidationError;
        std::ostringstream oss;
        oss << "Unsupported manifest schema_version '" << manifest.schemaVersion
            << "' (expected " << kSupportedSchemaVersion << ")";
        emitCommandFailed(context, exitCode, oss.str(), "schema_version_mismatch");
        return report;
    }

    // Validate scenes
    emitPhaseEvent(context, kValidationPhaseScenes, "started");
    for (const auto& scene : manifest.scenes) {
        if (args.sceneId.has_value() && scene.id != *args.sceneId) {
            continue;
        }
        if (!fileExists(scene.path)) {
            exitCode = CLIExitCode::FileNotFound;
            std::ostringstream oss;
            oss << "Scene file not found for '" << scene.id << "': " << scene.path.string();
            emitCommandFailed(context, exitCode, oss.str(), "scene_missing");
            return report;
        }
        report.scenesValidated += 1;

        if (context.globals.jsonOutput && context.emitter) {
            context.emitter->emit([&](auto& writer) {
                writer.Key("event"); writer.String("validation_scene_validated");
                writer.Key("scene_id"); writer.String(scene.id.c_str());
                writer.Key("path"); writer.String(scene.path.generic_string().c_str());
            });
        } else {
            Logger::info("Validated scene " + scene.id + " (" + scene.path.string() + ")");
        }
    }
    emitPhaseEvent(context, kValidationPhaseScenes, "completed");

    // Validate modules against modules.lock
    if (args.validateModules) {
        emitPhaseEvent(context, kValidationPhaseModules, "started");
        services::ModuleRegistry moduleRegistry = services::ModuleRegistry::load(manifest.workspaceRoot);

        std::vector<std::string> requiredModules = manifest.engineModules;
        for (const auto& module : manifest.modules) {
            if (module.enabled) {
                requiredModules.emplace_back(module.name);
            }
        }

        for (const auto& moduleName : requiredModules) {
            auto moduleEntry = moduleRegistry.find(moduleName);
            if (!moduleEntry.has_value() || !moduleEntry->enabled) {
                exitCode = CLIExitCode::DependencyError;
                std::ostringstream oss;
                oss << "Module '" << moduleName << "' not enabled in modules.lock";
                emitCommandFailed(context, exitCode, oss.str(), "module_missing");
                return report;
            }
            ++report.modulesValidated;
            if (context.globals.jsonOutput && context.emitter) {
                context.emitter->emit([&](auto& writer) {
                    writer.Key("event"); writer.String("validation_module_validated");
                    writer.Key("module"); writer.String(moduleName.c_str());
                    writer.Key("status"); writer.String("enabled");
                });
            } else {
                Logger::info("Module '" + moduleName + "' enabled");
            }
        }
        emitPhaseEvent(context, kValidationPhaseModules, "completed");
    }

    // Validate asset packs against assets.lock
    if (args.validateAssets) {
        emitPhaseEvent(context, kValidationPhaseAssets, "started");
        services::AssetRegistry assetRegistry = services::AssetRegistry::load(manifest.workspaceRoot);

        for (const auto& asset : manifest.assets) {
            auto assetEntry = assetRegistry.find(asset.name);
            if (!assetEntry.has_value()) {
                exitCode = CLIExitCode::DependencyError;
                std::ostringstream oss;
                oss << "Asset pack '" << asset.name << "' not present in assets.lock";
                emitCommandFailed(context, exitCode, oss.str(), "asset_missing");
                return report;
            }
            ++report.assetsValidated;

            if (context.globals.jsonOutput && context.emitter) {
                context.emitter->emit([&](auto& writer) {
                    writer.Key("event"); writer.String("validation_asset_validated");
                    writer.Key("asset_pack"); writer.String(asset.name.c_str());
                    writer.Key("status"); writer.String(assetEntry->status.c_str());
                });
            } else {
                Logger::info("Asset pack '" + asset.name + "' status: " + assetEntry->status);
            }
        }
        emitPhaseEvent(context, kValidationPhaseAssets, "completed");
    }

    exitCode = CLIExitCode::Success;
    return report;
}

void ValidateCommand::emitPhaseEvent(const CommandExecutionContext& context,
                                     const std::string& phase,
                                     const std::string& status) const
{
    if (context.globals.jsonOutput && context.emitter) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("validation_phase");
            writer.Key("phase"); writer.String(phase.c_str());
            writer.Key("status"); writer.String(status.c_str());
        });
    } else {
        Logger::info("Validation " + phase + ": " + status);
    }
}

} // namespace glint::cli
