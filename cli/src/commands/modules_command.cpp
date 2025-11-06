// Machine Summary Block
// {"file":"cli/src/commands/modules_command.cpp","purpose":"Implements the glint modules command for listing and mutating module state with dependency validation.","depends_on":["glint/cli/commands/modules_command.h","glint/cli/command_io.h","<algorithm>","<filesystem>","<sstream>"],"notes":["module_management_cli","lockfile_persistence","json_event_emission"]}
// Human Summary
// Handles `glint modules list|enable|disable`, merging manifest data with lockfile state, enforcing dependencies, and emitting structured output.

#include "glint/cli/commands/modules_command.h"

#include <algorithm>
#include <sstream>

#include "glint/cli/command_io.h"

namespace glint::cli {

namespace {

constexpr const char* kDefaultManifestName = "glint.project.json";

std::filesystem::path defaultManifestPath()
{
    return std::filesystem::current_path() / kDefaultManifestName;
}

bool usesJson(const CommandExecutionContext& context)
{
    return context.globals.jsonOutput && context.emitter != nullptr;
}

} // namespace

ModulesCommand::ModulesCommand() = default;

CLIExitCode ModulesCommand::run(const CommandExecutionContext& context)
{
    CLIExitCode errorCode = CLIExitCode::UnknownFlag;
    std::string errorMessage;
    auto parsed = parseArguments(context, errorCode, errorMessage);
    if (!parsed.has_value()) {
        emitCommandFailed(context, errorCode, errorMessage);
        return errorCode;
    }

    const std::filesystem::path manifestPath = resolveManifestPath(context);
    if (!std::filesystem::exists(manifestPath)) {
        std::ostringstream oss;
        oss << "Unable to locate project manifest at " << manifestPath.string();
        emitCommandFailed(context, CLIExitCode::FileNotFound, oss.str());
        return CLIExitCode::FileNotFound;
    }

    services::ProjectManifest manifest;
    try {
        manifest = services::ProjectManifestLoader::load(manifestPath);
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }

    services::ModuleRegistry registry;
    try {
        registry = services::ModuleRegistry::load(manifest.workspaceRoot);
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }

    auto statuses = buildModuleStatuses(manifest, registry);

    switch (parsed->mode) {
    case Mode::List:
        return handleList(context, statuses);
    case Mode::Enable:
        return handleEnable(context, parsed->moduleName, statuses, registry);
    case Mode::Disable:
        return handleDisable(context, parsed->moduleName, statuses, registry);
    }

    emitCommandFailed(context, CLIExitCode::RuntimeError, "Unhandled modules command mode");
    return CLIExitCode::RuntimeError;
}

std::optional<ModulesCommand::ParsedArgs> ModulesCommand::parseArguments(
    const CommandExecutionContext& context,
    CLIExitCode& errorCode,
    std::string& errorMessage) const
{
    std::vector<std::string> tokens;
    tokens.reserve(context.arguments.size());
    for (const auto& argument : context.arguments) {
        if (argument == "--json") {
            continue;
        }
        tokens.emplace_back(argument);
    }

    ParsedArgs args;
    if (tokens.empty()) {
        args.mode = Mode::List;
        return args;
    }

    const std::string& subcommand = tokens.front();
    if (subcommand == "list") {
        if (tokens.size() > 1) {
            errorCode = CLIExitCode::UnknownFlag;
            errorMessage = "Unexpected argument for 'glint modules list'";
            return std::nullopt;
        }
        args.mode = Mode::List;
        return args;
    }

    if (subcommand == "enable" || subcommand == "disable") {
        if (tokens.size() < 2) {
            errorCode = CLIExitCode::UnknownFlag;
            errorMessage = "Missing module name for 'glint modules " + subcommand + "'";
            return std::nullopt;
        }
        if (tokens.size() > 2) {
            errorCode = CLIExitCode::UnknownFlag;
            errorMessage = "Too many arguments for 'glint modules " + subcommand + "'";
            return std::nullopt;
        }

        args.mode = subcommand == "enable" ? Mode::Enable : Mode::Disable;
        args.moduleName = tokens[1];
        return args;
    }

    errorCode = CLIExitCode::UnknownFlag;
    errorMessage = "Unknown modules subcommand: " + subcommand;
    return std::nullopt;
}

std::filesystem::path ModulesCommand::resolveManifestPath(const CommandExecutionContext& context) const
{
    if (!context.globals.projectPath.empty()) {
        return std::filesystem::path(context.globals.projectPath);
    }
    return defaultManifestPath();
}

std::map<std::string, ModulesCommand::ModuleStatus> ModulesCommand::buildModuleStatuses(
    const services::ProjectManifest& manifest,
    const services::ModuleRegistry& registry) const
{
    std::map<std::string, ModuleStatus> statuses;

    for (const auto& name : manifest.engineModules) {
        ModuleStatus status;
        status.name = name;
        status.isCore = true;
        status.optional = false;
        status.defaultEnabled = true;
        status.enabled = true;
        status.declared = true;
        statuses.emplace(name, std::move(status));
    }

    for (const auto& module : manifest.modules) {
        ModuleStatus& status = statuses[module.name];
        status.name = module.name;
        status.declared = true;
        status.optional = module.optional;
        status.defaultEnabled = module.enabled;
        status.dependsOn = module.dependsOn;
        if (!status.isCore) {
            status.enabled = module.enabled;
        }
    }

    for (const auto& entry : registry.modules()) {
        ModuleStatus& status = statuses[entry.name];
        status.name = entry.name;
        status.locked = true;
        status.enabled = entry.enabled;
        status.version = entry.version;
        status.hash = entry.hash;
    }

    for (auto& [_, status] : statuses) {
        if (!status.locked) {
            if (status.isCore) {
                status.enabled = true;
            } else {
                status.enabled = status.defaultEnabled;
            }
        }
    }

    return statuses;
}

CLIExitCode ModulesCommand::handleList(const CommandExecutionContext& context,
                                       const std::map<std::string, ModuleStatus>& statuses) const
{
    size_t enabled = std::count_if(statuses.begin(), statuses.end(), [](const auto& pair) {
        return pair.second.enabled;
    });
    size_t disabled = statuses.size() - enabled;

    if (usesJson(context)) {
        for (const auto& [_, status] : statuses) {
            emitModuleEvent(context, status, "modules_state");
        }
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("modules_summary");
            writer.Key("total"); writer.Uint(static_cast<unsigned>(statuses.size()));
            writer.Key("enabled"); writer.Uint(static_cast<unsigned>(enabled));
            writer.Key("disabled"); writer.Uint(static_cast<unsigned>(disabled));
        });
    } else {
        std::ostringstream oss;
        oss << "Modules (" << statuses.size() << " total, "
            << enabled << " enabled, " << disabled << " disabled)";
        for (const auto& [_, status] : statuses) {
            oss << "\n - " << formatStatusSummary(status);
        }
        emitCommandInfo(context, oss.str());
    }

    return CLIExitCode::Success;
}

CLIExitCode ModulesCommand::handleEnable(const CommandExecutionContext& context,
                                         const std::string& moduleName,
                                         std::map<std::string, ModuleStatus>& statuses,
                                         services::ModuleRegistry& registry) const
{
    auto statusIt = statuses.find(moduleName);
    if (statusIt == statuses.end()) {
        std::string message = "Module '" + moduleName + "' is not declared in the project manifest or lockfile";
        emitCommandFailed(context, CLIExitCode::FileNotFound, message);
        return CLIExitCode::FileNotFound;
    }

    ModuleStatus& status = statusIt->second;
    if (status.enabled) {
        std::string message = "Module '" + moduleName + "' is already enabled";
        emitCommandInfo(context, message);
        if (usesJson(context)) {
            emitModuleEvent(context, status, "modules_state");
        }
        return CLIExitCode::Success;
    }

    std::vector<std::string> missingDeps;
    for (const auto& dep : status.dependsOn) {
        auto depIt = statuses.find(dep);
        if (depIt == statuses.end() || !depIt->second.enabled) {
            missingDeps.emplace_back(dep);
        }
    }
    if (!missingDeps.empty()) {
        std::ostringstream oss;
        oss << "Cannot enable module '" << moduleName << "'; dependencies disabled: ";
        bool first = true;
        for (const auto& dep : missingDeps) {
            if (!first) {
                oss << ", ";
            }
            oss << dep;
            first = false;
        }
        emitCommandFailed(context, CLIExitCode::DependencyError, oss.str());
        return CLIExitCode::DependencyError;
    }

    services::ModuleLockEntry entry;
    if (auto existing = registry.find(moduleName)) {
        entry = *existing;
    } else {
        entry.name = moduleName;
        entry.version = status.version.empty() ? "unspecified" : status.version;
        entry.hash = status.hash;
    }
    entry.enabled = true;

    registry.upsert(entry);
    try {
        registry.save();
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }

    status.enabled = true;
    status.locked = true;
    status.version = entry.version;
    status.hash = entry.hash;

    std::string message = "Enabled module '" + moduleName + "'";
    emitCommandInfo(context, message);
    if (usesJson(context)) {
        emitModuleEvent(context, status, "modules_state");
    }
    return CLIExitCode::Success;
}

CLIExitCode ModulesCommand::handleDisable(const CommandExecutionContext& context,
                                          const std::string& moduleName,
                                          std::map<std::string, ModuleStatus>& statuses,
                                          services::ModuleRegistry& registry) const
{
    auto statusIt = statuses.find(moduleName);
    if (statusIt == statuses.end()) {
        std::string message = "Module '" + moduleName + "' is not declared in the project manifest or lockfile";
        emitCommandFailed(context, CLIExitCode::FileNotFound, message);
        return CLIExitCode::FileNotFound;
    }

    ModuleStatus& status = statusIt->second;
    if (status.isCore || (!status.optional && status.declared)) {
        std::string message = "Module '" + moduleName + "' is required and cannot be disabled";
        emitCommandFailed(context, CLIExitCode::DependencyError, message);
        return CLIExitCode::DependencyError;
    }

    std::vector<std::string> dependents;
    for (const auto& [name, candidate] : statuses) {
        if (name == moduleName || !candidate.enabled) {
            continue;
        }
        if (std::find(candidate.dependsOn.begin(),
                      candidate.dependsOn.end(),
                      moduleName) != candidate.dependsOn.end()) {
            dependents.emplace_back(name);
        }
    }
    if (!dependents.empty()) {
        std::ostringstream oss;
        oss << "Cannot disable module '" << moduleName << "'; enabled dependents: ";
        bool first = true;
        for (const auto& dep : dependents) {
            if (!first) {
                oss << ", ";
            }
            oss << dep;
            first = false;
        }
        emitCommandFailed(context, CLIExitCode::DependencyError, oss.str());
        return CLIExitCode::DependencyError;
    }

    if (!status.enabled) {
        std::string message = "Module '" + moduleName + "' is already disabled";
        emitCommandInfo(context, message);
        if (usesJson(context)) {
            emitModuleEvent(context, status, "modules_state");
        }
        return CLIExitCode::Success;
    }

    services::ModuleLockEntry entry;
    if (auto existing = registry.find(moduleName)) {
        entry = *existing;
    } else {
        entry.name = moduleName;
        entry.version = status.version.empty() ? "unspecified" : status.version;
        entry.hash = status.hash;
    }
    entry.enabled = false;

    registry.upsert(entry);
    try {
        registry.save();
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }

    status.enabled = false;
    status.locked = true;
    status.version = entry.version;
    status.hash = entry.hash;

    std::string message = "Disabled module '" + moduleName + "'";
    emitCommandInfo(context, message);
    if (usesJson(context)) {
        emitModuleEvent(context, status, "modules_state");
    }
    return CLIExitCode::Success;
}

void ModulesCommand::emitModuleEvent(const CommandExecutionContext& context,
                                     const ModuleStatus& status,
                                     const std::string& event) const
{
    if (!usesJson(context)) {
        return;
    }

    context.emitter->emit([&](auto& writer) {
        writer.Key("event"); writer.String(event.c_str());
        writer.Key("module"); writer.String(status.name.c_str());
        writer.Key("enabled"); writer.Bool(status.enabled);
        writer.Key("optional"); writer.Bool(status.optional);
        writer.Key("declared"); writer.Bool(status.declared);
        writer.Key("core"); writer.Bool(status.isCore);
        if (!status.dependsOn.empty()) {
            writer.Key("depends_on");
            writer.StartArray();
            for (const auto& dep : status.dependsOn) {
                writer.String(dep.c_str());
            }
            writer.EndArray();
        }
        if (!status.version.empty()) {
            writer.Key("version"); writer.String(status.version.c_str());
        }
        if (!status.hash.empty()) {
            writer.Key("hash"); writer.String(status.hash.c_str());
        }
    });
}

std::string ModulesCommand::formatStatusSummary(const ModuleStatus& status)
{
    std::ostringstream oss;
    oss << status.name << " [" << (status.enabled ? "enabled" : "disabled");
    if (status.isCore) {
        oss << ", core";
    } else if (!status.optional) {
        oss << ", required";
    } else {
        oss << ", optional";
    }
    oss << "]";
    if (!status.version.empty()) {
        oss << " v" << status.version;
    }
    if (!status.hash.empty()) {
        oss << " (" << status.hash << ')';
    }
    return oss.str();
}

} // namespace glint::cli

