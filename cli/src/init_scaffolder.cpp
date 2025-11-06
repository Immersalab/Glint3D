// Machine Summary Block
// {"file":"cli/src/init_scaffolder.cpp","purpose":"Implements the glint init scaffolding planner and executor.","depends_on":["glint/cli/init_scaffolder.h","rapidjson/document.h","rapidjson/stringbuffer.h","rapidjson/writer.h"],"notes":["template_metadata_loading","deterministic_plan_generation","safe_filesystem_operations"]}
// Human Summary
// Generates deterministic plans for `glint init`, merges template metadata, builds manifest/config payloads, and executes the plan with optional dry-run.

#include "glint/cli/init_scaffolder.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace glint::cli {

namespace {

constexpr const char* kTemplatesRoot = "resources/templates";
constexpr const char* kTemplateIndexFile = "template.json";
constexpr const char* kTemplateManifestPatch = "project.patch.json";
constexpr const char* kTemplateConfigDefaults = "config.defaults.json";
constexpr const char* kDefaultSceneId = "SHOT000";

bool isDirectoryEmpty(const std::filesystem::path& path)
{
    return std::filesystem::is_empty(path);
}

rapidjson::Document parseTemplateFile(const std::filesystem::path& file)
{
    std::ifstream stream(file);
    if (!stream.is_open()) {
        throw std::runtime_error("Unable to open template file: " + file.string());
    }
    rapidjson::IStreamWrapper wrapper(stream);
    rapidjson::Document doc;
    doc.ParseStream(wrapper);
    if (doc.HasParseError()) {
        throw std::runtime_error("Template file parse error at " + file.string());
    }
    return doc;
}

std::string toJsonString(const rapidjson::Value& value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

std::filesystem::path normalizePath(const std::filesystem::path& path)
{
    std::filesystem::path result = path.empty()
        ? std::filesystem::current_path()
        : std::filesystem::absolute(path);
    return result.lexically_normal();
}

} // namespace

InitScaffolder::InitScaffolder() = default;

InitPlan InitScaffolder::plan(const InitRequest& request) const
{
    InitPlan plan;
    auto workspace = normalizePath(request.workspaceRoot);

    if (std::filesystem::exists(workspace) && !request.force && !isDirectoryEmpty(workspace)) {
        throw std::runtime_error("Workspace directory is not empty. Use --force to override: " + workspace.string());
    }

    TemplateMetadata metadata = loadTemplateMetadata(request.templateName);

    std::set<std::string> moduleSet(metadata.defaultModules.begin(), metadata.defaultModules.end());
    moduleSet.insert(request.modules.begin(), request.modules.end());
    std::vector<std::string> modules(moduleSet.begin(), moduleSet.end());
    std::sort(modules.begin(), modules.end());

    std::set<std::string> assetSet(metadata.defaultAssetPacks.begin(), metadata.defaultAssetPacks.end());
    if (request.withSamples) {
        assetSet.insert(metadata.defaultAssetPacks.begin(), metadata.defaultAssetPacks.end());
    }
    assetSet.insert(request.assetPacks.begin(), request.assetPacks.end());
    std::vector<std::string> assetPacks(assetSet.begin(), assetSet.end());
    std::sort(assetPacks.begin(), assetPacks.end());

    appendDirectorySkeleton(request, plan);
    appendTemplateFiles(metadata, request, plan);

    // Deterministic file payloads
    InitOperation manifestOp{
        InitOperationType::WriteFile,
        {},
        workspace / "glint.project.json",
        buildBaseManifest(request, metadata, modules, assetPacks)
    };
    plan.operations.push_back(std::move(manifestOp));

    if (!request.noConfig) {
        InitOperation configOp{
            InitOperationType::WriteFile,
            {},
            workspace / ".glint" / "config.json",
            buildWorkspaceConfig(metadata)
        };
        plan.operations.push_back(std::move(configOp));
    }

    InitOperation moduleLock{
        InitOperationType::WriteFile,
        {},
        workspace / "modules" / "modules.lock",
        buildModulesLock(modules)
    };
    plan.operations.push_back(std::move(moduleLock));

    InitOperation assetLock{
        InitOperationType::WriteFile,
        {},
        workspace / "assets" / "assets.lock",
        buildAssetsLock(assetPacks)
    };
    plan.operations.push_back(std::move(assetLock));

    plan.nextSteps = {
        "cd " + workspace.string(),
        "glint validate --project glint.project.json --strict",
        "glint render --project glint.project.json --scene " + std::string(kDefaultSceneId),
        "glint assets sync"
    };

    return plan;
}

InitResult InitScaffolder::execute(const InitPlan& plan, bool dryRun) const
{
    InitResult result;
    result.plan = plan;
    result.executed = !dryRun;

    if (dryRun) {
        return result;
    }

    for (const auto& op : plan.operations) {
        switch (op.type) {
        case InitOperationType::CreateDirectory:
            std::filesystem::create_directories(op.destinationPath);
            break;
        case InitOperationType::CopyTemplateFile:
            std::filesystem::create_directories(op.destinationPath.parent_path());
            std::filesystem::copy_file(op.sourcePath, op.destinationPath,
                                       std::filesystem::copy_options::overwrite_existing);
            break;
        case InitOperationType::WriteFile: {
            std::filesystem::create_directories(op.destinationPath.parent_path());
            std::ofstream output(op.destinationPath, std::ios::binary | std::ios::trunc);
            output << op.contents;
            output.close();
            break;
        }
        default:
            throw std::runtime_error("Unknown init operation encountered.");
        }
    }

    return result;
}

InitScaffolder::TemplateMetadata InitScaffolder::loadTemplateMetadata(const std::string& name) const
{
    TemplateMetadata metadata;
    metadata.name = name;
    metadata.templateRoot = std::filesystem::absolute(std::filesystem::path(kTemplatesRoot) / name);

    if (!std::filesystem::exists(metadata.templateRoot)) {
        throw std::runtime_error("Template \"" + name + "\" not found at " + metadata.templateRoot.string());
    }

    auto indexPath = metadata.templateRoot / kTemplateIndexFile;
    rapidjson::Document indexDoc = parseTemplateFile(indexPath);
    if (!indexDoc.IsObject()) {
        throw std::runtime_error("Template index must be a JSON object: " + indexPath.string());
    }

    metadata.description = indexDoc.HasMember("description") && indexDoc["description"].IsString()
        ? indexDoc["description"].GetString()
        : "Glint workspace template";

    if (indexDoc.HasMember("modules") && indexDoc["modules"].IsArray()) {
        for (const auto& value : indexDoc["modules"].GetArray()) {
            if (value.IsString()) {
                metadata.defaultModules.emplace_back(value.GetString());
            }
        }
    }
    if (indexDoc.HasMember("asset_packs") && indexDoc["asset_packs"].IsArray()) {
        for (const auto& value : indexDoc["asset_packs"].GetArray()) {
            if (value.IsString()) {
                metadata.defaultAssetPacks.emplace_back(value.GetString());
            }
        }
    }

    return metadata;
}

std::string InitScaffolder::buildBaseManifest(const InitRequest& request,
                                              const TemplateMetadata& metadata,
                                              const std::vector<std::string>& modules,
                                              const std::vector<std::string>& assetPacks) const
{
    rapidjson::Document manifest;
    manifest.SetObject();
    auto& allocator = manifest.GetAllocator();

    manifest.AddMember("schema_version", rapidjson::Value("1.0.0", allocator), allocator);

    rapidjson::Value project(rapidjson::kObjectType);
    project.AddMember("name", rapidjson::Value("Glint Workspace", allocator), allocator);
    project.AddMember("slug", rapidjson::Value("glint_workspace", allocator), allocator);
    project.AddMember("version", rapidjson::Value("0.1.0", allocator), allocator);
    project.AddMember("description", rapidjson::Value(metadata.description.c_str(), allocator), allocator);
    project.AddMember("default_template", rapidjson::Value(metadata.name.c_str(), allocator), allocator);
    manifest.AddMember("project", project, allocator);

    rapidjson::Value workspace(rapidjson::kObjectType);
    workspace.AddMember("root", rapidjson::Value(".", allocator), allocator);
    workspace.AddMember("assets_dir", rapidjson::Value("assets", allocator), allocator);
    workspace.AddMember("renders_dir", rapidjson::Value("renders", allocator), allocator);
    workspace.AddMember("modules_dir", rapidjson::Value("modules", allocator), allocator);
    workspace.AddMember("config_dir", rapidjson::Value(".glint", allocator), allocator);
    manifest.AddMember("workspace", workspace, allocator);

    rapidjson::Value engine(rapidjson::kObjectType);
    engine.AddMember("core_version", rapidjson::Value("3.0.0", allocator), allocator);
    rapidjson::Value moduleArray(rapidjson::kArrayType);
    for (const auto& module : modules) {
        moduleArray.PushBack(rapidjson::Value(module.c_str(), allocator), allocator);
    }
    engine.AddMember("modules", moduleArray, allocator);
    engine.AddMember("requires_gpu", false, allocator);
    manifest.AddMember("engine", engine, allocator);

    rapidjson::Value scenes(rapidjson::kArrayType);
    rapidjson::Value scene(rapidjson::kObjectType);
    scene.AddMember("id", rapidjson::Value(kDefaultSceneId, allocator), allocator);
    auto scenePath = "shots/" + std::string(kDefaultSceneId) + ".json";
    auto defaultOutput = "renders/" + std::string(kDefaultSceneId);
    scene.AddMember("path", rapidjson::Value(scenePath.c_str(), allocator), allocator);
    scene.AddMember("default_output", rapidjson::Value(defaultOutput.c_str(), allocator), allocator);
    scenes.PushBack(scene, allocator);
    manifest.AddMember("scenes", scenes, allocator);

    rapidjson::Value assets(rapidjson::kArrayType);
    for (const auto& pack : assetPacks) {
        rapidjson::Value entry(rapidjson::kObjectType);
        entry.AddMember("pack", rapidjson::Value(pack.c_str(), allocator), allocator);
        entry.AddMember("version", rapidjson::Value("1.0.0", allocator), allocator);
        std::string source = "https://example.com/" + pack + ".zip";
        entry.AddMember("source", rapidjson::Value(source.c_str(), allocator), allocator);
        entry.AddMember("optional", false, allocator);
        assets.PushBack(entry, allocator);
    }
    manifest.AddMember("assets", assets, allocator);

    rapidjson::Value modulesOverrides(rapidjson::kArrayType);
    manifest.AddMember("modules", modulesOverrides, allocator);

    rapidjson::Value configuration(rapidjson::kObjectType);
    {
        rapidjson::Value defaultsObj(rapidjson::kObjectType);
        configuration.AddMember("defaults", defaultsObj, allocator);
    }
    {
        rapidjson::Value overridesObj(rapidjson::kObjectType);
        configuration.AddMember("overrides", overridesObj, allocator);
    }
    manifest.AddMember("configuration", configuration, allocator);

    rapidjson::Value determinism(rapidjson::kObjectType);
    determinism.AddMember("rng_seed", 123456789, allocator);
    rapidjson::Value lockfiles(rapidjson::kObjectType);
    lockfiles.AddMember("modules", rapidjson::Value("modules.lock", allocator), allocator);
    lockfiles.AddMember("assets", rapidjson::Value("assets.lock", allocator), allocator);
    determinism.AddMember("lockfiles", lockfiles, allocator);
    rapidjson::Value provenance(rapidjson::kObjectType);
    provenance.AddMember("capture", true, allocator);
    rapidjson::Value artifacts(rapidjson::kArrayType);
    artifacts.PushBack(rapidjson::Value("renders/<name>/run.json", allocator), allocator);
    provenance.AddMember("artifacts", artifacts, allocator);
    determinism.AddMember("provenance", provenance, allocator);
    manifest.AddMember("determinism", determinism, allocator);

    auto patchPath = metadata.templateRoot / kTemplateManifestPatch;
    if (std::filesystem::exists(patchPath)) {
        rapidjson::Document patch = parseTemplateFile(patchPath);
        for (auto itr = patch.MemberBegin(); itr != patch.MemberEnd(); ++itr) {
            manifest[itr->name.GetString()] = rapidjson::Value(itr->value, allocator);
        }
    }

    return toJsonString(manifest);
}

std::string InitScaffolder::buildWorkspaceConfig(const TemplateMetadata& metadata) const
{
    auto configPath = metadata.templateRoot / kTemplateConfigDefaults;
    if (!std::filesystem::exists(configPath)) {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Value render(rapidjson::kObjectType);
        render.AddMember("device", "auto", doc.GetAllocator());
        render.AddMember("samples", 32, doc.GetAllocator());
        doc.AddMember("render", render, doc.GetAllocator());
        return toJsonString(doc);
    }
    rapidjson::Document config = parseTemplateFile(configPath);
    return toJsonString(config);
}

std::string InitScaffolder::buildModulesLock(const std::vector<std::string>& modules) const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    doc.AddMember("schema_version", "1.0.0", allocator);
    rapidjson::Value moduleArray(rapidjson::kArrayType);
    for (const auto& module : modules) {
        rapidjson::Value entry(rapidjson::kObjectType);
        entry.AddMember("name", rapidjson::Value(module.c_str(), allocator), allocator);
        entry.AddMember("version", "1.0.0", allocator);
        entry.AddMember("enabled", true, allocator);
        moduleArray.PushBack(entry, allocator);
    }
    doc.AddMember("modules", moduleArray, allocator);
    return toJsonString(doc);
}

std::string InitScaffolder::buildAssetsLock(const std::vector<std::string>& assetPacks) const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    doc.AddMember("schema_version", "1.0.0", allocator);
    rapidjson::Value packArray(rapidjson::kArrayType);
    for (const auto& pack : assetPacks) {
        rapidjson::Value entry(rapidjson::kObjectType);
        entry.AddMember("name", rapidjson::Value(pack.c_str(), allocator), allocator);
        entry.AddMember("version", "1.0.0", allocator);
        entry.AddMember("status", "pending", allocator);
        packArray.PushBack(entry, allocator);
    }
    doc.AddMember("packs", packArray, allocator);
    return toJsonString(doc);
}

void InitScaffolder::appendDirectorySkeleton(const InitRequest& request, InitPlan& plan) const
{
    auto root = normalizePath(request.workspaceRoot);

    const std::vector<std::filesystem::path> directories = {
        root,
        root / ".glint",
        root / "modules",
        root / "assets",
        root / "assets" / "packs",
        root / "renders",
        root / "shots",
        root / "templates"
    };

    for (const auto& dir : directories) {
        plan.operations.push_back(
            InitOperation{InitOperationType::CreateDirectory, {}, dir, {}});
    }
}

void InitScaffolder::appendTemplateFiles(const TemplateMetadata& metadata,
                                         const InitRequest& request,
                                         InitPlan& plan) const
{
    auto root = normalizePath(request.workspaceRoot);

    auto templateShots = metadata.templateRoot / "shots";
    if (std::filesystem::exists(templateShots)) {
        for (auto& entry : std::filesystem::recursive_directory_iterator(templateShots)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            auto relative = std::filesystem::relative(entry.path(), metadata.templateRoot);
            InitOperation op{
                InitOperationType::CopyTemplateFile,
                entry.path(),
                root / relative,
                {}
            };
            plan.operations.push_back(std::move(op));
        }
    }
}

} // namespace glint::cli
