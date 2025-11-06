// Machine Summary Block
// {"file":"cli/src/services/project_manifest.cpp","purpose":"Loads and validates glint.project.json manifests into strongly typed structures.","depends_on":["glint/cli/services/project_manifest.h","rapidjson/document.h","rapidjson/istreamwrapper.h","<filesystem>","<fstream>","<stdexcept>"],"notes":["workspace_path_resolution","manifest_schema_validation","determinism_metadata_extraction"]}
// Human Summary
// Parses `glint.project.json`, resolves workspace-relative paths, and populates manifest structures used by CLI commands.

#include "glint/cli/services/project_manifest.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

namespace glint::cli::services {

namespace {

std::filesystem::path normalizePath(const std::filesystem::path& path)
{
    return path.lexically_normal();
}

std::string requireString(const rapidjson::Value& value, const char* key)
{
    if (!value.IsObject() || !value.HasMember(key) || !value[key].IsString()) {
        throw std::runtime_error(std::string("Manifest missing string field: ") + key);
    }
    return value[key].GetString();
}

bool optionalBool(const rapidjson::Value& value, const char* key, bool fallback = false)
{
    if (!value.IsObject() || !value.HasMember(key)) {
        return fallback;
    }
    if (!value[key].IsBool()) {
        throw std::runtime_error(std::string("Manifest field '") + key + "' must be a boolean");
    }
    return value[key].GetBool();
}

std::filesystem::path resolveWorkspacePath(const std::filesystem::path& workspaceRoot,
                                           const rapidjson::Value& parent,
                                           const char* key)
{
    std::string relative = requireString(parent, key);
    return normalizePath(workspaceRoot / relative);
}

SceneDescriptor parseScene(const std::filesystem::path& workspaceRoot, const rapidjson::Value& value)
{
    SceneDescriptor scene;
    if (!value.IsObject()) {
        throw std::runtime_error("Manifest scene entries must be objects");
    }

    scene.id = requireString(value, "id");
    scene.path = resolveWorkspacePath(workspaceRoot, value, "path");

    if (value.HasMember("thumbnail") && value["thumbnail"].IsString()) {
        scene.thumbnail = normalizePath(workspaceRoot / value["thumbnail"].GetString());
    }
    if (value.HasMember("default_output") && value["default_output"].IsString()) {
        scene.defaultOutput = normalizePath(workspaceRoot / value["default_output"].GetString());
    }

    return scene;
}

ModuleDescriptor parseModuleDescriptor(const rapidjson::Value& value)
{
    ModuleDescriptor module;
    if (!value.IsObject()) {
        throw std::runtime_error("Manifest module entries must be objects");
    }

    module.name = requireString(value, "name");
    module.enabled = optionalBool(value, "enabled", true);
    module.optional = optionalBool(value, "optional", false);

    if (value.HasMember("depends_on")) {
        if (!value["depends_on"].IsArray()) {
            throw std::runtime_error("module.depends_on must be an array of strings");
        }
        for (const auto& dep : value["depends_on"].GetArray()) {
            if (!dep.IsString()) {
                throw std::runtime_error("module.depends_on entries must be strings");
            }
            module.dependsOn.emplace_back(dep.GetString());
        }
    }

    return module;
}

AssetDescriptor parseAssetDescriptor(const rapidjson::Value& value)
{
    AssetDescriptor asset;
    if (!value.IsObject()) {
        throw std::runtime_error("Manifest asset entries must be objects");
    }

    asset.name = requireString(value, "pack");
    asset.version = requireString(value, "version");
    asset.source = requireString(value, "source");
    asset.optional = optionalBool(value, "optional", false);
    if (value.HasMember("hash") && value["hash"].IsString()) {
        asset.hash = value["hash"].GetString();
    }

    return asset;
}

DeterminismDescriptor parseDeterminism(const std::filesystem::path& workspaceRoot,
                                       const rapidjson::Value& value)
{
    DeterminismDescriptor determinism;
    if (!value.IsObject()) {
        return determinism;
    }

    if (value.HasMember("rng_seed")) {
        if (!value["rng_seed"].IsInt64()) {
            throw std::runtime_error("determinism.rng_seed must be an integer");
        }
        determinism.rngSeed = value["rng_seed"].GetInt64();
    }

    if (value.HasMember("lockfiles") && value["lockfiles"].IsObject()) {
        const auto& lockfiles = value["lockfiles"];
        if (lockfiles.HasMember("modules") && lockfiles["modules"].IsString()) {
            determinism.modulesLock = normalizePath(workspaceRoot / lockfiles["modules"].GetString());
        }
        if (lockfiles.HasMember("assets") && lockfiles["assets"].IsString()) {
            determinism.assetsLock = normalizePath(workspaceRoot / lockfiles["assets"].GetString());
        }
    }

    if (value.HasMember("provenance") && value["provenance"].IsObject()) {
        determinism.captureProvenance = optionalBool(value["provenance"], "capture", true);
    }

    return determinism;
}

std::unordered_map<std::string, std::unordered_map<std::string, std::string>> parseConfigurationOverrides(
    const rapidjson::Value& configuration)
{
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> overrides;

    if (!configuration.IsObject() || !configuration.HasMember("overrides")) {
        return overrides;
    }
    const auto& overridesValue = configuration["overrides"];
    if (!overridesValue.IsObject()) {
        throw std::runtime_error("configuration.overrides must be an object");
    }

    for (auto it = overridesValue.MemberBegin(); it != overridesValue.MemberEnd(); ++it) {
        const auto& target = it->name.GetString();
        const auto& overrideObject = it->value;
        if (!overrideObject.IsObject()) {
            throw std::runtime_error("configuration override entries must be objects");
        }

        std::unordered_map<std::string, std::string> kv;
        for (auto kvIt = overrideObject.MemberBegin(); kvIt != overrideObject.MemberEnd(); ++kvIt) {
            if (!kvIt->value.IsString()) {
                throw std::runtime_error("configuration override values must be strings");
            }
            kv.emplace(kvIt->name.GetString(), kvIt->value.GetString());
        }
        overrides.emplace(target, std::move(kv));
    }

    return overrides;
}

} // namespace

ProjectManifest ProjectManifestLoader::load(const std::filesystem::path& manifestPath)
{
    std::filesystem::path absolutePath = normalizePath(std::filesystem::absolute(manifestPath));
    std::ifstream stream(absolutePath);
    if (!stream) {
        throw std::runtime_error("Failed to open project manifest: " + absolutePath.string());
    }

    rapidjson::IStreamWrapper wrapper(stream);
    rapidjson::Document document;
    document.ParseStream(wrapper);
    if (document.HasParseError() || !document.IsObject()) {
        throw std::runtime_error("Project manifest is not valid JSON");
    }

    ProjectManifest manifest;
    manifest.manifestPath = absolutePath;
    manifest.schemaVersion = requireString(document, "schema_version");

    if (!document.HasMember("project") || !document["project"].IsObject()) {
        throw std::runtime_error("Project manifest missing 'project' section");
    }
    const auto& project = document["project"];
    manifest.projectName = requireString(project, "name");
    manifest.projectSlug = requireString(project, "slug");
    manifest.projectVersion = requireString(project, "version");
    if (project.HasMember("description") && project["description"].IsString()) {
        manifest.description = project["description"].GetString();
    }
    if (project.HasMember("default_template") && project["default_template"].IsString()) {
        manifest.defaultTemplate = project["default_template"].GetString();
    }

    if (!document.HasMember("workspace") || !document["workspace"].IsObject()) {
        throw std::runtime_error("Project manifest missing 'workspace' section");
    }
    const auto& workspace = document["workspace"];

    std::filesystem::path manifestDir = absolutePath.parent_path();
    std::filesystem::path workspaceRoot = normalizePath(manifestDir / requireString(workspace, "root"));
    manifest.workspaceRoot = workspaceRoot;
    manifest.assetsDirectory = resolveWorkspacePath(workspaceRoot, workspace, "assets_dir");
    manifest.rendersDirectory = resolveWorkspacePath(workspaceRoot, workspace, "renders_dir");
    manifest.modulesDirectory = resolveWorkspacePath(workspaceRoot, workspace, "modules_dir");
    manifest.configDirectory = resolveWorkspacePath(workspaceRoot, workspace, "config_dir");

    if (!document.HasMember("engine") || !document["engine"].IsObject()) {
        throw std::runtime_error("Project manifest missing 'engine' section");
    }
    const auto& engine = document["engine"];
    if (!engine.HasMember("modules") || !engine["modules"].IsArray()) {
        throw std::runtime_error("engine.modules must be an array");
    }
    for (const auto& moduleName : engine["modules"].GetArray()) {
        if (!moduleName.IsString()) {
            throw std::runtime_error("engine.modules entries must be strings");
        }
        manifest.engineModules.emplace_back(moduleName.GetString());
    }
    manifest.requiresGpu = optionalBool(engine, "requires_gpu", false);

    if (!document.HasMember("scenes") || !document["scenes"].IsArray()) {
        throw std::runtime_error("Project manifest requires a non-empty scenes array");
    }
    const auto& scenes = document["scenes"];
    if (scenes.Empty()) {
        throw std::runtime_error("Project manifest scenes array must not be empty");
    }
    manifest.scenes.reserve(scenes.Size());
    for (const auto& sceneValue : scenes.GetArray()) {
        manifest.scenes.emplace_back(parseScene(workspaceRoot, sceneValue));
    }

    if (document.HasMember("modules") && document["modules"].IsArray()) {
        const auto& modules = document["modules"];
        manifest.modules.reserve(modules.Size());
        for (const auto& moduleValue : modules.GetArray()) {
            manifest.modules.emplace_back(parseModuleDescriptor(moduleValue));
        }
    }

    if (document.HasMember("assets") && document["assets"].IsArray()) {
        const auto& assets = document["assets"];
        manifest.assets.reserve(assets.Size());
        for (const auto& assetValue : assets.GetArray()) {
            manifest.assets.emplace_back(parseAssetDescriptor(assetValue));
        }
    }

    if (document.HasMember("determinism")) {
        manifest.determinism = parseDeterminism(workspaceRoot, document["determinism"]);
    }

    if (document.HasMember("configuration") && document["configuration"].IsObject()) {
        manifest.configurationOverrides = parseConfigurationOverrides(document["configuration"]);
    }

    return manifest;
}

} // namespace glint::cli::services
