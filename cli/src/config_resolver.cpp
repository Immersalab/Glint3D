// Machine Summary Block
// {"file":"cli/src/config_resolver.cpp","purpose":"Implements layered configuration resolution for the Glint CLI platform.","depends_on":["glint/cli/config_resolver.h","io/user_paths.h","rapidjson/error/en.h","rapidjson/istreamwrapper.h"],"notes":["loads_json_layers","supports_environment_variables","tracks_provenance"]}
// Human Summary
// Reads configuration layers (built-in, global, workspace, manifest, environment, overrides) and merges them into a deterministic snapshot with provenance metadata.

#include "glint/cli/config_resolver.h"

#include "io/user_paths.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include <cctype>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace glint::cli {

namespace {

std::string serializeJson(const rapidjson::Value& value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

rapidjson::Document parseFile(const std::filesystem::path& path)
{
    rapidjson::Document doc;
    if (!std::filesystem::exists(path)) {
        doc.SetObject();
        return doc;
    }

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        doc.SetObject();
        return doc;
    }

    rapidjson::IStreamWrapper wrapper(ifs);
    doc.ParseStream(wrapper);
    if (doc.HasParseError()) {
        ifs.close();
        throw std::runtime_error(
            "Failed to parse JSON file \"" + path.string() + "\": " +
            std::string(rapidjson::GetParseError_En(doc.GetParseError())) +
            " (offset " + std::to_string(doc.GetErrorOffset()) + ")");
    }
    ifs.close();

    return doc;
}

rapidjson::Document parseJsonString(const std::string& json)
{
    rapidjson::Document doc;
    if (json.empty()) {
        doc.SetObject();
        return doc;
    }
    doc.Parse(json.c_str());
    if (doc.HasParseError()) {
        throw std::runtime_error(
            "Failed to parse JSON value \"" + json + "\": " +
            std::string(rapidjson::GetParseError_En(doc.GetParseError())) +
            " (offset " + std::to_string(doc.GetErrorOffset()) + ")");
    }
    return doc;
}

std::vector<std::string> collectEnvironmentVariables()
{
    std::vector<std::string> result;
#if defined(_WIN32)
    LPCH envBlock = GetEnvironmentStringsA();
    if (!envBlock) {
        return result;
    }
    LPCH current = envBlock;
    while (*current != '\0') {
        std::string entry(current);
        result.emplace_back(entry);
        current += std::strlen(current) + 1;
    }
    FreeEnvironmentStringsA(envBlock);
    return result;
#else
    extern char** environ;
    char** env = environ;
    if (!env) {
        return result;
    }
    for (char** ptr = env; *ptr != nullptr; ++ptr) {
        result.emplace_back(*ptr);
    }
    return result;
#endif
}

std::string normalizeEnvKey(const std::string& key)
{
    std::string normalized;
    normalized.reserve(key.size());
    for (size_t i = 0; i < key.size(); ++i) {
        char c = key[i];
        if (c == '_') {
            if (i + 1 < key.size() && key[i + 1] == '_') {
                normalized.push_back('.');
                ++i;
                continue;
            }
            normalized.push_back('.');
        } else {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    if (!normalized.empty() && normalized.front() == '.') {
        normalized.erase(normalized.begin());
    }
    return normalized;
}

void ensureObject(rapidjson::Document& doc)
{
    if (!doc.IsObject()) {
        doc.SetObject();
    }
}

void ensureObject(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator)
{
    if (!value.IsObject()) {
        value.SetObject();
    }
}

void assignDotKey(rapidjson::Document& doc,
                  const std::string& key,
                  const rapidjson::Value& value,
                  rapidjson::Document::AllocatorType& allocator)
{
    ensureObject(doc);
    rapidjson::Value* cursor = &doc;
    std::stringstream ss(key);
    std::string segment;
    std::vector<std::string> segments;
    while (std::getline(ss, segment, '.')) {
        segments.push_back(segment);
    }
    for (size_t i = 0; i < segments.size(); ++i) {
        const std::string& part = segments[i];
        auto member = cursor->FindMember(part.c_str());
        if (i == segments.size() - 1) {
            rapidjson::Value copy(value, allocator);
            if (member == cursor->MemberEnd()) {
                rapidjson::Value name(part.c_str(), allocator);
                cursor->AddMember(name, copy, allocator);
            } else {
                member->value = copy;
            }
            return;
        }
        if (member == cursor->MemberEnd()) {
            rapidjson::Value nestedObj(rapidjson::kObjectType);
            rapidjson::Value name(part.c_str(), allocator);
            cursor->AddMember(name, nestedObj, allocator);
            member = cursor->FindMember(part.c_str());
        }
        if (!member->value.IsObject()) {
            member->value.SetObject();
        }
        cursor = &member->value;
        ensureObject(*cursor, allocator);
    }
}

} // namespace

ConfigResolver::ConfigResolver() = default;

ConfigSnapshot ConfigResolver::resolve(const ConfigResolveRequest& request) const
{
    ConfigSnapshot snapshot;
    snapshot.document.SetObject();
    auto& allocator = snapshot.document.GetAllocator();

    snapshot.provenance.clear();

    const struct LayerSpec {
        ConfigLayer layer;
        std::string label;
        std::function<rapidjson::Document()> loader;
    } layers[] = {
        {ConfigLayer::BuiltInDefaults, "built-in defaults", [this]() {
             return buildBuiltInDefaults();
         }},
        {ConfigLayer::GlobalConfig, "global config", [this]() {
             return loadGlobalConfig();
         }},
        {ConfigLayer::Environment, "environment variables", [this, &request]() {
             if (!request.includeEnvironment) {
                 rapidjson::Document empty;
                 empty.SetObject();
                 return empty;
             }
             return loadEnvironmentVariables();
        }},
        {ConfigLayer::WorkspaceConfig, "workspace config", [this, &request]() {
             return loadWorkspaceConfig(request.workspaceRoot);
        }},
        {ConfigLayer::ManifestDefaults, "manifest defaults", [this, &request]() {
             return loadManifestConfig(request.manifestPath, request.sceneId, false);
        }},
        {ConfigLayer::ManifestOverrides, "manifest overrides", [this, &request]() {
             return loadManifestConfig(request.manifestPath, request.sceneId, true);
        }},
        {ConfigLayer::CommandContext, "command context", [this, &request]() {
             return buildMapDocument(request.commandContext);
        }},
        {ConfigLayer::CliFlags, "cli overrides", [this, &request]() {
             return buildMapDocument(request.cliOverrides);
        }},
    };

    for (const auto& spec : layers) {
        rapidjson::Document layerDoc = spec.loader();
        if (!layerDoc.IsObject() || layerDoc.ObjectEmpty()) {
            continue;
        }
            mergeDocuments(snapshot.document, layerDoc, snapshot.provenance, spec.layer, spec.label, allocator);
    }

    return snapshot;
}

rapidjson::Document ConfigResolver::buildBuiltInDefaults() const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    rapidjson::Value render(rapidjson::kObjectType);
    render.AddMember("device", "auto", allocator);
    render.AddMember("samples", 64, allocator);
    render.AddMember("denoise", false, allocator);

    rapidjson::Value assets(rapidjson::kObjectType);
    assets.AddMember("cache_dir", "cache/assets", allocator);

    doc.AddMember("render", render, allocator);
    doc.AddMember("assets", assets, allocator);

    return doc;
}

rapidjson::Document ConfigResolver::loadGlobalConfig() const
{
    auto path = glint::getConfigPath("config.json");
    if (path.empty()) {
        rapidjson::Document doc;
        doc.SetObject();
        return doc;
    }
    return parseFile(path);
}

rapidjson::Document ConfigResolver::loadWorkspaceConfig(const std::filesystem::path& workspaceRoot) const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto configPath = workspaceRoot / ".glint" / "config.json";
    if (!std::filesystem::exists(configPath)) {
        return doc;
    }
    return parseFile(configPath);
}

rapidjson::Document ConfigResolver::loadManifestConfig(const std::optional<std::filesystem::path>& manifestPath,
                                                       const std::string& sceneId,
                                                       bool overrides) const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    if (!manifestPath.has_value()) {
        return doc;
    }

    rapidjson::Document manifest = parseFile(*manifestPath);
    if (!manifest.IsObject()) {
        return doc;
    }

    const char* configurationKey = "configuration";
    const char* overridesKey = "overrides";
    const char* defaultsKey = "defaults";

    if (!manifest.HasMember(configurationKey) || !manifest[configurationKey].IsObject()) {
        return doc;
    }

    const rapidjson::Value& configuration = manifest[configurationKey];
    if (!overrides) {
        if (configuration.HasMember(defaultsKey) && configuration[defaultsKey].IsObject()) {
            doc.CopyFrom(configuration[defaultsKey], allocator);
        }
    } else {
        if (!sceneId.empty() && configuration.HasMember(overridesKey) &&
            configuration[overridesKey].IsObject()) {
            const rapidjson::Value& overridesObj = configuration[overridesKey];
            auto itr = overridesObj.FindMember(sceneId.c_str());
            if (itr != overridesObj.MemberEnd() && itr->value.IsObject()) {
                doc.CopyFrom(itr->value, allocator);
            }
        }
    }

    return doc;
}

rapidjson::Document ConfigResolver::loadEnvironmentVariables() const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    const auto variables = collectEnvironmentVariables();
    for (const auto& entry : variables) {
        auto delimiter = entry.find('=');
        if (delimiter == std::string::npos) {
            continue;
        }
        std::string key = entry.substr(0, delimiter);
        if (key.rfind("GLINT_", 0) != 0) {
            continue;
        }
        std::string value = entry.substr(delimiter + 1);
        std::string normalized = normalizeEnvKey(key.substr(strlen("GLINT_")));
        if (normalized.empty()) {
            continue;
        }
        rapidjson::Value val;
        val.SetString(value.c_str(), allocator);
        assignDotKey(doc, normalized, val, allocator);
    }

    return doc;
}

rapidjson::Document ConfigResolver::buildMapDocument(const std::unordered_map<std::string, std::string>& map) const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    for (const auto& [key, json] : map) {
        rapidjson::Document parsed = parseJsonString(json);
        assignDotKey(doc, key, parsed, allocator);
    }
    return doc;
}

void ConfigResolver::mergeDocuments(rapidjson::Document& target,
                                    const rapidjson::Document& source,
                                    std::unordered_map<std::string, std::vector<ConfigProvenanceRecord>>& provenance,
                                    ConfigLayer layer,
                                    const std::string& sourceLabel,
                                    Allocator& allocator)
{
    if (!source.IsObject()) {
        return;
    }
    for (auto itr = source.MemberBegin(); itr != source.MemberEnd(); ++itr) {
        auto member = target.FindMember(itr->name);
        std::string keyPath = itr->name.GetString();
        if (member == target.MemberEnd()) {
            rapidjson::Value name(itr->name, allocator);
            rapidjson::Value value(itr->value, allocator);
            target.AddMember(name, value, allocator);
            member = target.FindMember(itr->name);
            appendProvenance(keyPath, member->value, layer, sourceLabel, provenance);
            continue;
        }
        mergeValues(member->value, itr->value, keyPath, provenance, layer, sourceLabel, allocator);
    }
}

void ConfigResolver::mergeValues(rapidjson::Value& target,
                                 const rapidjson::Value& source,
                                 const std::string& keyPath,
                                 std::unordered_map<std::string, std::vector<ConfigProvenanceRecord>>& provenance,
                                 ConfigLayer layer,
                                 const std::string& sourceLabel,
                                 Allocator& allocator)
{
    if (target.GetType() != source.GetType() || !target.IsObject() || !source.IsObject()) {
        rapidjson::Value newValue(source, allocator);
        target = newValue;
        appendProvenance(keyPath, target, layer, sourceLabel, provenance);
        return;
    }

    for (auto itr = source.MemberBegin(); itr != source.MemberEnd(); ++itr) {
        std::string childKeyPath = keyPath.empty() ? itr->name.GetString()
                                                   : keyPath + "." + itr->name.GetString();
        auto member = target.FindMember(itr->name);
        if (member == target.MemberEnd()) {
            rapidjson::Value name(itr->name, allocator);
            rapidjson::Value value(itr->value, allocator);
            target.AddMember(name, value, allocator);
            auto inserted = target.FindMember(itr->name);
            appendProvenance(childKeyPath, inserted->value, layer, sourceLabel, provenance);
        } else {
            mergeValues(member->value, itr->value, childKeyPath, provenance, layer, sourceLabel, allocator);
        }
    }
}

void ConfigResolver::appendProvenance(const std::string& keyPath,
                                      const rapidjson::Value& value,
                                      ConfigLayer layer,
                                      const std::string& sourceLabel,
                                      std::unordered_map<std::string, std::vector<ConfigProvenanceRecord>>& provenance)
{
    ConfigProvenanceRecord record;
    record.layer = layer;
    record.source = sourceLabel;
    record.json = serializeJson(value);
    provenance[keyPath].push_back(std::move(record));
}

} // namespace glint::cli
