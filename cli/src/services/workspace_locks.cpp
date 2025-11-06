// Machine Summary Block
// {"file":"cli/src/services/workspace_locks.cpp","purpose":"Implements helpers for loading and saving workspace module and asset lockfiles.","depends_on":["glint/cli/services/workspace_locks.h","rapidjson/document.h","rapidjson/istreamwrapper.h","rapidjson/stringbuffer.h","rapidjson/writer.h","<algorithm>","<filesystem>","<fstream>","<stdexcept>"],"notes":["lockfile_roundtrip","deterministic_serialisation","json_validation"]}
// Human Summary
// Reads and writes `modules.lock` and `assets.lock`, providing deterministic ordering and simple mutation helpers for CLI commands.

#include "glint/cli/services/workspace_locks.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <type_traits>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace glint::cli::services {

namespace {

constexpr const char* kLockSchemaVersion = "1.0.0";
constexpr const char* kModulesDirName = "modules";
constexpr const char* kAssetsDirName = "assets";
constexpr const char* kModulesLockName = "modules.lock";
constexpr const char* kAssetsLockName = "assets.lock";

std::filesystem::path resolveLockPath(const std::filesystem::path& workspaceRoot,
                                      const std::filesystem::path& directory,
                                      const std::filesystem::path& filename)
{
    return workspaceRoot / directory / filename;
}

void ensureParentDirectory(const std::filesystem::path& path)
{
    auto parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent);
    }
}

template <typename T>
std::optional<size_t> findIndexByName(const std::vector<T>& entries, const std::string& name)
{
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

ModuleLockEntry parseModuleEntry(const rapidjson::Value& value)
{
    ModuleLockEntry entry;
    if (!value.IsObject()) {
        return entry;
    }

    if (value.HasMember("name") && value["name"].IsString()) {
        entry.name = value["name"].GetString();
    }
    if (value.HasMember("version") && value["version"].IsString()) {
        entry.version = value["version"].GetString();
    }
    if (value.HasMember("enabled") && value["enabled"].IsBool()) {
        entry.enabled = value["enabled"].GetBool();
    }
    if (value.HasMember("hash") && value["hash"].IsString()) {
        entry.hash = value["hash"].GetString();
    }
    return entry;
}

AssetLockEntry parseAssetEntry(const rapidjson::Value& value)
{
    AssetLockEntry entry;
    if (!value.IsObject()) {
        return entry;
    }

    if (value.HasMember("name") && value["name"].IsString()) {
        entry.name = value["name"].GetString();
    }
    if (value.HasMember("version") && value["version"].IsString()) {
        entry.version = value["version"].GetString();
    }
    if (value.HasMember("status") && value["status"].IsString()) {
        entry.status = value["status"].GetString();
    }
    if (value.HasMember("hash") && value["hash"].IsString()) {
        entry.hash = value["hash"].GetString();
    }
    return entry;
}

template <typename T>
void sortByName(std::vector<T>& entries)
{
    std::sort(entries.begin(), entries.end(), [](const T& lhs, const T& rhs) {
        return lhs.name < rhs.name;
    });
}

template <typename T>
void writeEntries(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                  const std::vector<T>& entries)
{
    writer.StartArray();
    for (const auto& entry : entries) {
        writer.StartObject();
        writer.Key("name");
        writer.String(entry.name.c_str());
        writer.Key("version");
        writer.String(entry.version.c_str());
        if constexpr (std::is_same_v<T, ModuleLockEntry>) {
            writer.Key("enabled");
            writer.Bool(entry.enabled);
        } else {
            writer.Key("status");
            writer.String(entry.status.c_str());
        }
        if (!entry.hash.empty()) {
            writer.Key("hash");
            writer.String(entry.hash.c_str());
        }
        writer.EndObject();
    }
    writer.EndArray();
}

} // namespace

ModuleRegistry::ModuleRegistry() = default;

ModuleRegistry ModuleRegistry::load(const std::filesystem::path& workspaceRoot)
{
    ModuleRegistry registry;
    registry.m_lockPath = resolveLockPath(workspaceRoot, kModulesDirName, kModulesLockName);

    if (!std::filesystem::exists(registry.m_lockPath)) {
        return registry;
    }

    std::ifstream stream(registry.m_lockPath);
    if (!stream) {
        throw std::runtime_error("Failed to open modules.lock: " + registry.m_lockPath.string());
    }

    rapidjson::IStreamWrapper wrapper(stream);
    rapidjson::Document document;
    document.ParseStream(wrapper);
    if (document.HasParseError()) {
        throw std::runtime_error("Failed to parse modules.lock: invalid JSON");
    }

    if (document.HasMember("modules") && document["modules"].IsArray()) {
        const auto& modules = document["modules"];
        registry.m_modules.reserve(modules.Size());
        for (const auto& entryValue : modules.GetArray()) {
            auto entry = parseModuleEntry(entryValue);
            if (!entry.name.empty()) {
                registry.m_modules.emplace_back(std::move(entry));
            }
        }
    }

    return registry;
}

void ModuleRegistry::save() const
{
    if (m_lockPath.empty()) {
        throw std::runtime_error("ModuleRegistry::save called without a lock path");
    }

    ensureParentDirectory(m_lockPath);

    std::vector<ModuleLockEntry> sorted = m_modules;
    sortByName(sorted);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("schema_version");
    writer.String(kLockSchemaVersion);
    writer.Key("modules");
    writeEntries(writer, sorted);
    writer.EndObject();

    std::ofstream stream(m_lockPath, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("Failed to open modules.lock for writing: " + m_lockPath.string());
    }
    stream << buffer.GetString() << '\n';
    if (!stream) {
        throw std::runtime_error("Failed to write modules.lock: " + m_lockPath.string());
    }
}

void ModuleRegistry::upsert(const ModuleLockEntry& entry)
{
    if (entry.name.empty()) {
        return;
    }
    if (auto index = findIndexByName(m_modules, entry.name)) {
        m_modules[*index] = entry;
    } else {
        m_modules.emplace_back(entry);
    }
}

void ModuleRegistry::setEnabled(const std::string& moduleName, bool enabled)
{
    if (auto index = findIndexByName(m_modules, moduleName)) {
        m_modules[*index].enabled = enabled;
    }
}

void ModuleRegistry::remove(const std::string& moduleName)
{
    if (auto index = findIndexByName(m_modules, moduleName)) {
        m_modules.erase(m_modules.begin() + static_cast<std::ptrdiff_t>(*index));
    }
}

std::optional<ModuleLockEntry> ModuleRegistry::find(const std::string& moduleName) const
{
    if (auto index = findIndexByName(m_modules, moduleName)) {
        return m_modules[*index];
    }
    return std::nullopt;
}

AssetRegistry::AssetRegistry() = default;

AssetRegistry AssetRegistry::load(const std::filesystem::path& workspaceRoot)
{
    AssetRegistry registry;
    registry.m_lockPath = resolveLockPath(workspaceRoot, kAssetsDirName, kAssetsLockName);

    if (!std::filesystem::exists(registry.m_lockPath)) {
        return registry;
    }

    std::ifstream stream(registry.m_lockPath);
    if (!stream) {
        throw std::runtime_error("Failed to open assets.lock: " + registry.m_lockPath.string());
    }

    rapidjson::IStreamWrapper wrapper(stream);
    rapidjson::Document document;
    document.ParseStream(wrapper);
    if (document.HasParseError()) {
        throw std::runtime_error("Failed to parse assets.lock: invalid JSON");
    }

    if (document.HasMember("packs") && document["packs"].IsArray()) {
        const auto& packs = document["packs"];
        registry.m_assets.reserve(packs.Size());
        for (const auto& entryValue : packs.GetArray()) {
            auto entry = parseAssetEntry(entryValue);
            if (!entry.name.empty()) {
                registry.m_assets.emplace_back(std::move(entry));
            }
        }
    }

    return registry;
}

void AssetRegistry::save() const
{
    if (m_lockPath.empty()) {
        throw std::runtime_error("AssetRegistry::save called without a lock path");
    }

    ensureParentDirectory(m_lockPath);

    std::vector<AssetLockEntry> sorted = m_assets;
    sortByName(sorted);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("schema_version");
    writer.String(kLockSchemaVersion);
    writer.Key("packs");
    writeEntries(writer, sorted);
    writer.EndObject();

    std::ofstream stream(m_lockPath, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("Failed to open assets.lock for writing: " + m_lockPath.string());
    }
    stream << buffer.GetString() << '\n';
    if (!stream) {
        throw std::runtime_error("Failed to write assets.lock: " + m_lockPath.string());
    }
}

void AssetRegistry::upsert(const AssetLockEntry& entry)
{
    if (entry.name.empty()) {
        return;
    }
    if (auto index = findIndexByName(m_assets, entry.name)) {
        m_assets[*index] = entry;
    } else {
        m_assets.emplace_back(entry);
    }
}

void AssetRegistry::setStatus(const std::string& name, const std::string& status)
{
    if (auto index = findIndexByName(m_assets, name)) {
        m_assets[*index].status = status;
    }
}

std::optional<AssetLockEntry> AssetRegistry::find(const std::string& name) const
{
    if (auto index = findIndexByName(m_assets, name)) {
        return m_assets[*index];
    }
    return std::nullopt;
}

} // namespace glint::cli::services
