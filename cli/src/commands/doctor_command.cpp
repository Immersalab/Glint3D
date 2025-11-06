
// Machine Summary Block
// {"file":"cli/src/commands/doctor_command.cpp","purpose":"Implements workspace diagnostics and optional fix-ups for the CLI doctor command.","depends_on":["glint/cli/commands/doctor_command.h","glint/cli/command_io.h","rapidjson/document.h","rapidjson/stringbuffer.h","rapidjson/writer.h","<fstream>","<sstream>"],"notes":["health_checks","lockfile_generation","ndjson_doctor_events"]}
// Human Summary
// Executes health checks for manifests, lockfiles, and configuration files, optionally repairing missing assets while emitting structured results.

#include "glint/cli/commands/doctor_command.h"

#include <fstream>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace glint::cli {

namespace {

const char* kModulesLockName = "modules.lock";
const char* kAssetsLockName = "assets.lock";
const char* kWorkspaceConfigName = "config.json";

struct SummaryCounters {
    size_t passed = 0;
    size_t warnings = 0;
    size_t failed = 0;
    size_t fixes = 0;
};

std::filesystem::path defaultManifestPath()
{
    return std::filesystem::current_path() / "glint.project.json";
}

void writeEmptyArrayDoc(const std::filesystem::path& path,
                        const char* arrayName)
{
    rapidjson::Document document;
    document.SetObject();
    auto& allocator = document.GetAllocator();
    document.AddMember(rapidjson::Value("schema_version", allocator),
                       rapidjson::Value("1.0.0", allocator),
                       allocator);
    rapidjson::Value array(rapidjson::kArrayType);
    document.AddMember(rapidjson::Value(arrayName, allocator), array, allocator);

    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("Failed to create lockfile: " + path.string());
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    stream << buffer.GetString() << '\n';
    if (!stream) {
        throw std::runtime_error("Failed to write lockfile: " + path.string());
    }
}

void writeEmptyObjectDoc(const std::filesystem::path& path)
{
    rapidjson::Document document;
    document.SetObject();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("Failed to create config file: " + path.string());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    stream << buffer.GetString() << '\n';
}

} // namespace

DoctorCommand::DoctorCommand() = default;

CLIExitCode DoctorCommand::run(const CommandExecutionContext& context)
{
    CLIExitCode errorCode = CLIExitCode::UnknownFlag;
    std::string errorMessage;
    auto args = parseArguments(context, errorCode, errorMessage);
    if (!args.has_value()) {
        emitCommandFailed(context, errorCode, errorMessage);
        return errorCode;
    }

    try {
        return runChecks(context, *args);
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }
}

std::optional<DoctorCommand::ParsedArgs> DoctorCommand::parseArguments(
    const CommandExecutionContext& context,
    CLIExitCode& errorCode,
    std::string& errorMessage) const
{
    ParsedArgs args;
    args.manifestPath = context.globals.projectPath.empty()
        ? defaultManifestPath()
        : std::filesystem::path(context.globals.projectPath);

    for (size_t i = 0; i < context.arguments.size(); ++i) {
        const std::string& token = context.arguments[i];
        if (token == "--json") {
            continue;
        }
        if (token == "--project") {
            if (i + 1 >= context.arguments.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --project (expected path to glint.project.json)";
                return std::nullopt;
            }
            args.manifestPath = context.arguments[++i];
            continue;
        }
        if (token == "--fix") {
            args.attemptFix = true;
            continue;
        }

        errorCode = CLIExitCode::UnknownFlag;
        errorMessage = "Unknown argument for glint doctor: " + token;
        return std::nullopt;
    }

    return args;
}

CLIExitCode DoctorCommand::runChecks(const CommandExecutionContext& context,
                                     const ParsedArgs& args) const
{
    SummaryCounters counters;
    CLIExitCode exitCode = CLIExitCode::Success;
    std::vector<CheckResult> results;
    services::ProjectManifest manifest;
    bool manifestLoaded = false;

    if (!std::filesystem::exists(args.manifestPath)) {
        emitCheck(context, {"manifest", "failed", "Project manifest not found at " + args.manifestPath.string()});
        counters.failed++;
        return CLIExitCode::FileNotFound;
    }

    try {
        manifest = services::ProjectManifestLoader::load(args.manifestPath);
        manifestLoaded = true;
        emitCheck(context, {"manifest", "passed", "Project manifest parsed successfully"});
        counters.passed++;
    } catch (const std::exception& ex) {
        emitCheck(context, {"manifest", "failed", ex.what()});
        counters.failed++;
        return CLIExitCode::RuntimeError;
    }

    // Modules lock check
    std::filesystem::path modulesLock = manifest.modulesDirectory / kModulesLockName;
    if (std::filesystem::exists(modulesLock)) {
        try {
            services::ModuleRegistry registry = services::ModuleRegistry::load(manifest.workspaceRoot);
            std::ostringstream oss;
            oss << registry.modules().size() << " module(s) registered.";
            emitCheck(context, {"modules_lock", "passed", oss.str()});
            counters.passed++;
        } catch (const std::exception& ex) {
            emitCheck(context, {"modules_lock", "failed", ex.what()});
            counters.failed++;
            exitCode = CLIExitCode::RuntimeError;
        }
    } else if (args.attemptFix) {
        try {
            writeEmptyArrayDoc(modulesLock, "modules");
            emitCheck(context, {"modules_lock", "warning", "modules.lock was missing and has been created"});
            counters.warnings++;
            counters.fixes++;
        } catch (const std::exception& ex) {
            emitCheck(context, {"modules_lock", "failed", ex.what()});
            counters.failed++;
            exitCode = CLIExitCode::RuntimeError;
        }
    } else {
        emitCheck(context, {"modules_lock", "warning", "modules.lock missing (run with --fix to scaffold)"});
        counters.warnings++;
    }

    // Assets lock check
    std::filesystem::path assetsLock = manifest.assetsDirectory / kAssetsLockName;
    if (std::filesystem::exists(assetsLock)) {
        try {
            services::AssetRegistry registry = services::AssetRegistry::load(manifest.workspaceRoot);
            std::ostringstream oss;
            oss << registry.assets().size() << " asset pack(s) tracked.";
            emitCheck(context, {"assets_lock", "passed", oss.str()});
            counters.passed++;
        } catch (const std::exception& ex) {
            emitCheck(context, {"assets_lock", "failed", ex.what()});
            counters.failed++;
            exitCode = CLIExitCode::RuntimeError;
        }
    } else if (args.attemptFix) {
        try {
            writeEmptyArrayDoc(assetsLock, "packs");
            emitCheck(context, {"assets_lock", "warning", "assets.lock was missing and has been created"});
            counters.warnings++;
            counters.fixes++;
        } catch (const std::exception& ex) {
            emitCheck(context, {"assets_lock", "failed", ex.what()});
            counters.failed++;
            exitCode = CLIExitCode::RuntimeError;
        }
    } else {
        emitCheck(context, {"assets_lock", "warning", "assets.lock missing (run with --fix to scaffold)"});
        counters.warnings++;
    }

    // Workspace config check
    std::filesystem::path workspaceConfig = manifest.configDirectory / kWorkspaceConfigName;
    if (std::filesystem::exists(workspaceConfig)) {
        emitCheck(context, {"workspace_config", "passed", ".glint/config.json present"});
        counters.passed++;
    } else if (args.attemptFix) {
        try {
            writeEmptyObjectDoc(workspaceConfig);
            emitCheck(context, {"workspace_config", "warning", "Created empty .glint/config.json"});
            counters.warnings++;
            counters.fixes++;
        } catch (const std::exception& ex) {
            emitCheck(context, {"workspace_config", "failed", ex.what()});
            counters.failed++;
            exitCode = CLIExitCode::RuntimeError;
        }
    } else {
        emitCheck(context, {"workspace_config", "warning", ".glint/config.json missing (run with --fix to create)"});
        counters.warnings++;
    }

    // Engine module sanity check
    if (manifestLoaded) {
        if (manifest.engineModules.empty()) {
            emitCheck(context, {"engine_modules", "failed", "No core engine modules declared in manifest"});
            counters.failed++;
            exitCode = CLIExitCode::RuntimeError;
        } else {
            std::ostringstream oss;
            oss << "Engine modules: " << manifest.engineModules.size();
            emitCheck(context, {"engine_modules", "passed", oss.str()});
            counters.passed++;
        }
    }

    if (context.globals.jsonOutput && context.emitter) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("doctor_summary");
            writer.Key("passed"); writer.Uint(static_cast<unsigned int>(counters.passed));
            writer.Key("warnings"); writer.Uint(static_cast<unsigned int>(counters.warnings));
            writer.Key("failed"); writer.Uint(static_cast<unsigned int>(counters.failed));
            writer.Key("fixes_applied"); writer.Uint(static_cast<unsigned int>(counters.fixes));
        });
    } else {
        std::ostringstream oss;
        oss << "Doctor summary - passed: " << counters.passed
            << ", warnings: " << counters.warnings
            << ", failed: " << counters.failed
            << ", fixes: " << counters.fixes;
        Logger::info(oss.str());
    }

    return exitCode;
}

void DoctorCommand::emitCheck(const CommandExecutionContext& context,
                              const CheckResult& result) const
{
    if (context.globals.jsonOutput && context.emitter) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("doctor_check");
            writer.Key("check"); writer.String(result.name.c_str());
            writer.Key("status"); writer.String(result.status.c_str());
            writer.Key("message"); writer.String(result.message.c_str());
        });
    } else {
        std::string line = "[" + result.status + "] " + result.name + ": " + result.message;
        if (result.status == "failed") {
            Logger::error(line);
        } else if (result.status == "warning") {
            Logger::warn(line);
        } else {
            Logger::info(line);
        }
    }
}

} // namespace glint::cli
