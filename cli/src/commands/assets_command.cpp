// Machine Summary Block
// {"file":"cli/src/commands/assets_command.cpp","purpose":"Implements the glint assets command for listing pack status and synchronizing assets.lock.","depends_on":["glint/cli/commands/assets_command.h","glint/cli/command_io.h","<algorithm>","<filesystem>","<sstream>","<vector>"],"notes":["asset_management_cli","lockfile_mutation","ndjson_event_emission"]}
// Human Summary
// Handles `glint assets` subcommands (list, status, sync), merging manifest data with assets.lock, flagging sync requirements, and emitting structured output.

#include "glint/cli/commands/assets_command.h"

#include <algorithm>
#include <sstream>
#include <vector>

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

std::vector<std::string> collectSyncTargets(const services::ProjectManifest& manifest,
                                            const std::optional<std::string>& packName)
{
    std::vector<std::string> targets;
    if (packName.has_value()) {
        targets.emplace_back(*packName);
        return targets;
    }

    targets.reserve(manifest.assets.size());
    for (const auto& asset : manifest.assets) {
        targets.emplace_back(asset.name);
    }
    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
    return targets;
}

} // namespace

AssetsCommand::AssetsCommand() = default;

CLIExitCode AssetsCommand::run(const CommandExecutionContext& context)
{
    CLIExitCode errorCode = CLIExitCode::UnknownFlag;
    std::string errorMessage;
    auto parsed = parseArguments(context, errorCode, errorMessage);
    if (!parsed.has_value()) {
        emitCommandFailed(context, errorCode, errorMessage);
        return errorCode;
    }

    std::filesystem::path manifestPath = parsed->manifestPath.empty()
        ? defaultManifestPath()
        : parsed->manifestPath;

    std::error_code fsError;
    if (!std::filesystem::exists(manifestPath, fsError)) {
        std::ostringstream oss;
        oss << "Unable to locate project manifest at " << manifestPath.string();
        emitCommandFailed(context, CLIExitCode::FileNotFound, oss.str());
        return CLIExitCode::FileNotFound;
    }

    services::ProjectManifest manifest;
    try {
        manifest = loadManifest(manifestPath, context);
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }

    services::AssetRegistry registry;
    try {
        registry = loadRegistry(manifest, context);
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }

    switch (parsed->mode) {
    case Mode::List: {
        auto statuses = buildAssetStatuses(manifest, registry);
        return handleList(context, *parsed, statuses);
    }
    case Mode::Status: {
        auto statuses = buildAssetStatuses(manifest, registry);
        return handleStatus(context, *parsed, statuses);
    }
    case Mode::Sync:
        return handleSync(context, *parsed, manifest, registry);
    }

    emitCommandFailed(context, CLIExitCode::RuntimeError, "Unhandled assets command mode");
    return CLIExitCode::RuntimeError;
}

std::optional<AssetsCommand::ParsedArgs> AssetsCommand::parseArguments(
    const CommandExecutionContext& context,
    CLIExitCode& errorCode,
    std::string& errorMessage) const
{
    ParsedArgs args;
    args.manifestPath = context.globals.projectPath.empty()
        ? defaultManifestPath()
        : std::filesystem::path(context.globals.projectPath);

    size_t index = 0;
    const auto& tokens = context.arguments;
    if (index < tokens.size()) {
        const std::string& candidate = tokens[index];
        if (candidate == "list") {
            args.mode = Mode::List;
            ++index;
        } else if (candidate == "status") {
            args.mode = Mode::Status;
            ++index;
        } else if (candidate == "sync") {
            args.mode = Mode::Sync;
            ++index;
        }
    }

    for (; index < tokens.size(); ++index) {
        const std::string& token = tokens[index];
        if (token == "--json") {
            continue;
        }
        if (token == "--pack") {
            if (index + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --pack (expected asset pack name)";
                return std::nullopt;
            }
            args.packName = tokens[++index];
            continue;
        }
        if (token == "--source") {
            if (index + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --source (expected URI or path)";
                return std::nullopt;
            }
            args.sourceOverride = tokens[++index];
            continue;
        }
        if (token == "--project") {
            if (index + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --project (expected path to glint.project.json)";
                return std::nullopt;
            }
            args.manifestPath = tokens[++index];
            continue;
        }

        errorCode = CLIExitCode::UnknownFlag;
        errorMessage = "Unknown argument for glint assets: " + token;
        return std::nullopt;
    }

    if (args.sourceOverride.has_value() && args.mode != Mode::Sync) {
        errorCode = CLIExitCode::UnknownFlag;
        errorMessage = "--source is only valid with 'glint assets sync'";
        return std::nullopt;
    }

    if (args.sourceOverride.has_value() && !args.packName.has_value()) {
        errorCode = CLIExitCode::UnknownFlag;
        errorMessage = "--source requires --pack to specify the target asset pack";
        return std::nullopt;
    }

    return args;
}

services::ProjectManifest AssetsCommand::loadManifest(const std::filesystem::path& manifestPath,
                                                      const CommandExecutionContext&) const
{
    return services::ProjectManifestLoader::load(manifestPath);
}

services::AssetRegistry AssetsCommand::loadRegistry(const services::ProjectManifest& manifest,
                                                    const CommandExecutionContext&) const
{
    return services::AssetRegistry::load(manifest.workspaceRoot);
}

std::map<std::string, AssetsCommand::AssetStatus> AssetsCommand::buildAssetStatuses(
    const services::ProjectManifest& manifest,
    const services::AssetRegistry& registry) const
{
    std::map<std::string, AssetStatus> statuses;

    for (const auto& asset : manifest.assets) {
        auto it = statuses.find(asset.name);
        if (it == statuses.end()) {
            AssetStatus status;
            status.name = asset.name;
            status.manifestVersion = asset.version;
            status.manifestSource = asset.source;
            status.manifestHash = asset.hash;
            status.manifestOptional = asset.optional;
            status.declared = true;
            statuses.emplace(status.name, std::move(status));
        } else {
            AssetStatus& status = it->second;
            status.name = asset.name;
            status.manifestVersion = asset.version;
            status.manifestSource = asset.source;
            status.manifestHash = asset.hash;
            status.manifestOptional = asset.optional;
            status.declared = true;
        }
    }

    for (const auto& entry : registry.assets()) {
        auto& status = statuses[entry.name];
        if (status.name.empty()) {
            status.name = entry.name;
        }
        status.hasLockEntry = true;
        status.lockVersion = entry.version;
        status.lockStatus = entry.status;
        status.lockHash = entry.hash;
    }

    for (auto& [_, status] : statuses) {
        if (!status.hasLockEntry) {
            status.lockStatus = "missing";
        } else if (status.lockStatus.empty()) {
            status.lockStatus = "unknown";
        }

        bool versionMismatch = false;
        if (status.declared && status.hasLockEntry) {
            if (!status.manifestVersion.empty()
                && !status.lockVersion.empty()
                && status.manifestVersion != status.lockVersion) {
                versionMismatch = true;
            }
            if (!status.manifestHash.empty()
                && !status.lockHash.empty()
                && status.manifestHash != status.lockHash) {
                versionMismatch = true;
            }
        }

        status.needsSync = !status.declared
            || !status.hasLockEntry
            || versionMismatch
            || (status.hasLockEntry && status.lockStatus != "installed");
    }

    return statuses;
}

CLIExitCode AssetsCommand::handleList(const CommandExecutionContext& context,
                                      const ParsedArgs& args,
                                      const std::map<std::string, AssetStatus>& statuses) const
{
    std::vector<const AssetStatus*> selection;
    selection.reserve(statuses.size());

    if (args.packName.has_value()) {
        auto it = statuses.find(*args.packName);
        if (it == statuses.end()) {
            std::string message = "Asset pack '" + *args.packName + "' not found in manifest or assets.lock";
            emitCommandFailed(context, CLIExitCode::FileNotFound, message);
            return CLIExitCode::FileNotFound;
        }
        selection.emplace_back(&it->second);
    } else {
        for (const auto& [_, status] : statuses) {
            selection.emplace_back(&status);
        }
    }

    if (selection.empty()) {
        emitCommandInfo(context, "No asset packs defined in manifest or assets.lock.");
        return CLIExitCode::Success;
    }

    for (const auto* status : selection) {
        emitCommandInfo(context, formatStatusSummary(*status));
        emitAssetEvent(context, *status, "assets_state");
    }

    return CLIExitCode::Success;
}

CLIExitCode AssetsCommand::handleStatus(const CommandExecutionContext& context,
                                        const ParsedArgs& args,
                                        const std::map<std::string, AssetStatus>& statuses) const
{
    std::vector<const AssetStatus*> selection;
    selection.reserve(statuses.size());

    if (args.packName.has_value()) {
        auto it = statuses.find(*args.packName);
        if (it == statuses.end()) {
            std::string message = "Asset pack '" + *args.packName + "' not found in manifest or assets.lock";
            emitCommandFailed(context, CLIExitCode::FileNotFound, message);
            return CLIExitCode::FileNotFound;
        }
        selection.emplace_back(&it->second);
    } else {
        for (const auto& [_, status] : statuses) {
            selection.emplace_back(&status);
        }
    }

    if (selection.empty()) {
        emitCommandInfo(context, "No asset packs defined in manifest or assets.lock.");
        return CLIExitCode::Success;
    }

    bool hasIssues = false;
    for (const auto* status : selection) {
        emitCommandInfo(context, formatStatusSummary(*status));
        emitAssetEvent(context, *status, "assets_state");
        if (status->needsSync) {
            hasIssues = true;
            std::vector<std::string> reasons;
            if (!status->declared) {
                reasons.emplace_back("not declared in manifest");
            }
            if (!status->hasLockEntry) {
                reasons.emplace_back("missing assets.lock entry");
            } else {
                if (status->lockStatus != "installed") {
                    reasons.emplace_back("lock status is '" + status->lockStatus + "'");
                }
                if (!status->manifestVersion.empty()
                    && !status->lockVersion.empty()
                    && status->manifestVersion != status->lockVersion) {
                    std::ostringstream reason;
                    reason << "version mismatch (manifest " << status->manifestVersion
                           << " vs lock " << status->lockVersion << ')';
                    reasons.emplace_back(reason.str());
                }
                if (!status->manifestHash.empty()
                    && !status->lockHash.empty()
                    && status->manifestHash != status->lockHash) {
                    reasons.emplace_back("hash mismatch");
                }
            }

            std::ostringstream oss;
            oss << "Asset pack '" << status->name << "' requires sync";
            if (!reasons.empty()) {
                oss << ": ";
                for (size_t i = 0; i < reasons.size(); ++i) {
                    if (i > 0) {
                        oss << "; ";
                    }
                    oss << reasons[i];
                }
            }
            emitCommandWarning(context, oss.str());
        }
    }

    if (!hasIssues) {
        emitCommandInfo(context, "All asset packs are synchronized.");
    }

    return CLIExitCode::Success;
}

CLIExitCode AssetsCommand::handleSync(const CommandExecutionContext& context,
                                      const ParsedArgs& args,
                                      const services::ProjectManifest& manifest,
                                      services::AssetRegistry& registry) const
{
    std::vector<std::string> targets = collectSyncTargets(manifest, args.packName);
    if (targets.empty()) {
        emitCommandInfo(context, "No asset packs declared in manifest.");
        return CLIExitCode::Success;
    }

    size_t updated = 0;
    for (const auto& name : targets) {
        auto it = std::find_if(manifest.assets.begin(),
                               manifest.assets.end(),
                               [&](const auto& asset) { return asset.name == name; });
        if (it == manifest.assets.end()) {
            std::string message = "Asset pack '" + name + "' is not declared in glint.project.json";
            emitCommandFailed(context, CLIExitCode::DependencyError, message);
            return CLIExitCode::DependencyError;
        }

        services::AssetLockEntry entry;
        entry.name = it->name;
        entry.version = it->version;
        entry.status = "installed";
        entry.hash = it->hash;

        registry.upsert(entry);
        ++updated;

        if (args.sourceOverride.has_value()) {
            std::ostringstream oss;
            oss << "Using override source for '" << it->name << "': " << *args.sourceOverride;
            emitCommandInfo(context, oss.str());
        }
    }

    try {
        registry.save();
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }

    auto statuses = buildAssetStatuses(manifest, registry);
    for (const auto& name : targets) {
        auto it = statuses.find(name);
        if (it != statuses.end()) {
            emitAssetEvent(context, it->second, "assets_synced", args.sourceOverride);
        }
    }

    std::ostringstream summary;
    summary << "Synchronized " << updated << " asset pack";
    if (updated != 1) {
        summary << 's';
    }
    summary << '.';
    emitCommandInfo(context, summary.str());
    return CLIExitCode::Success;
}

void AssetsCommand::emitAssetEvent(const CommandExecutionContext& context,
                                   const AssetStatus& status,
                                   const std::string& event,
                                   const std::optional<std::string>& sourceOverride) const
{
    if (!usesJson(context)) {
        return;
    }

    context.emitter->emit([&](auto& writer) {
        writer.Key("event"); writer.String(event.c_str());
        writer.Key("command"); writer.String(context.verb.c_str());
        writer.Key("asset_pack"); writer.String(status.name.c_str());
        writer.Key("declared"); writer.Bool(status.declared);
        writer.Key("optional"); writer.Bool(status.manifestOptional);
        writer.Key("needs_sync"); writer.Bool(status.needsSync);
        if (!status.manifestVersion.empty()) {
            writer.Key("manifest_version"); writer.String(status.manifestVersion.c_str());
        }
        if (!status.lockVersion.empty()) {
            writer.Key("lock_version"); writer.String(status.lockVersion.c_str());
        }
        if (!status.lockStatus.empty()) {
            writer.Key("lock_status"); writer.String(status.lockStatus.c_str());
        }
        if (!status.manifestSource.empty()) {
            writer.Key("source"); writer.String(status.manifestSource.c_str());
        }
        if (sourceOverride.has_value()) {
            writer.Key("source_override"); writer.String(sourceOverride->c_str());
        }
        if (!status.manifestHash.empty()) {
            writer.Key("manifest_hash"); writer.String(status.manifestHash.c_str());
        }
        if (!status.lockHash.empty() && status.lockHash != status.manifestHash) {
            writer.Key("lock_hash"); writer.String(status.lockHash.c_str());
        }
    });
}

std::string AssetsCommand::formatStatusSummary(const AssetStatus& status)
{
    std::vector<std::string> tags;
    if (status.declared) {
        tags.emplace_back(status.manifestOptional ? "optional" : "required");
    } else {
        tags.emplace_back("undeclared");
    }

    if (status.hasLockEntry) {
        tags.emplace_back(status.lockStatus.empty() ? "unknown" : status.lockStatus);
        if (!status.manifestVersion.empty()
            && !status.lockVersion.empty()
            && status.manifestVersion != status.lockVersion) {
            tags.emplace_back("version-mismatch");
        }
        if (!status.manifestHash.empty()
            && !status.lockHash.empty()
            && status.manifestHash != status.lockHash) {
            tags.emplace_back("hash-mismatch");
        }
    } else {
        tags.emplace_back("missing-lock");
    }

    if (status.needsSync) {
        tags.emplace_back("needs-sync");
    }

    std::ostringstream oss;
    oss << status.name << " [";
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << tags[i];
    }
    oss << "]";
    if (!status.manifestVersion.empty()) {
        oss << " manifest v" << status.manifestVersion;
    }
    if (status.hasLockEntry
        && !status.lockVersion.empty()
        && status.lockVersion != status.manifestVersion) {
        oss << " (lock v" << status.lockVersion << ')';
    }
    if (!status.manifestSource.empty()) {
        oss << " <- " << status.manifestSource;
    }
    return oss.str();
}

} // namespace glint::cli
