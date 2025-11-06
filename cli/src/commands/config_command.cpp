
// Machine Summary Block
// {"file":"cli/src/commands/config_command.cpp","purpose":"Implements the glint config command for querying and mutating configuration layers.","depends_on":["glint/cli/commands/config_command.h","glint/cli/command_io.h","io/user_paths.h","rapidjson/document.h","rapidjson/stringbuffer.h","rapidjson/writer.h","<fstream>","<sstream>"],"notes":["configuration_resolution","workspace_and_global_scopes","dot_key_mutation"]}
// Human Summary
// Handles `glint config` operations: showing snapshots, reading specific keys with provenance, and writing updates to workspace, project, or global config scopes.

#include "glint/cli/commands/config_command.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "io/user_paths.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace glint::cli {

namespace {

constexpr const char* kWorkspaceConfigFilename = "config.json";
constexpr const char* kDefaultsKey = "defaults";

std::filesystem::path defaultManifestPath()
{
    return std::filesystem::current_path() / "glint.project.json";
}

std::vector<std::string> splitDotKey(const std::string& key)
{
    std::vector<std::string> parts;
    std::string current;
    for (char c : key) {
        if (c == '.') {
            if (!current.empty()) {
                parts.emplace_back(std::move(current));
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        parts.emplace_back(std::move(current));
    }
    return parts;
}

rapidjson::Value* findValue(rapidjson::Value& root,
                            const std::vector<std::string>& parts)
{
    rapidjson::Value* current = &root;
    for (const auto& part : parts) {
        if (!current->IsObject()) {
            return nullptr;
        }
        auto member = current->FindMember(part.c_str());
        if (member == current->MemberEnd()) {
            return nullptr;
        }
        current = &member->value;
    }
    return current;
}

const rapidjson::Value* findValueConst(const rapidjson::Value& root,
                                       const std::vector<std::string>& parts)
{
    const rapidjson::Value* current = &root;
    for (const auto& part : parts) {
        if (!current->IsObject()) {
            return nullptr;
        }
        auto member = current->FindMember(part.c_str());
        if (member == current->MemberEnd()) {
            return nullptr;
        }
        current = &member->value;
    }
    return current;
}

void ensureObject(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator)
{
    if (!value.IsObject()) {
        value.SetObject();
    }
}

void assignDotKey(rapidjson::Value& root,
                  const std::vector<std::string>& parts,
                  size_t index,
                  const rapidjson::Value& newValue,
                  rapidjson::Document::AllocatorType& allocator)
{
    const std::string& part = parts[index];

    if (index == parts.size() - 1) {
        auto member = root.FindMember(part.c_str());
        rapidjson::Value copy(newValue, allocator);
        if (member == root.MemberEnd()) {
            rapidjson::Value name(part.c_str(), allocator);
            root.AddMember(name, copy, allocator);
        } else {
            member->value = copy;
        }
        return;
    }

    auto member = root.FindMember(part.c_str());
    if (member == root.MemberEnd()) {
        rapidjson::Value name(part.c_str(), allocator);
        rapidjson::Value child(rapidjson::kObjectType);
        root.AddMember(name, child, allocator);
        member = root.FindMember(part.c_str());
    }

    ensureObject(member->value, allocator);
    assignDotKey(member->value, parts, index + 1, newValue, allocator);
}

bool removeDotKey(rapidjson::Value& root,
                  const std::vector<std::string>& parts,
                  size_t index,
                  rapidjson::Document::AllocatorType& allocator)
{
    if (!root.IsObject()) {
        return false;
    }

    const std::string& part = parts[index];
    auto member = root.FindMember(part.c_str());
    if (member == root.MemberEnd()) {
        return false;
    }

    if (index == parts.size() - 1) {
        root.RemoveMember(member);
        return true;
    }

    bool removed = removeDotKey(member->value, parts, index + 1, allocator);
    if (removed && member->value.IsObject() && member->value.ObjectEmpty()) {
        root.RemoveMember(member);
    }
    return removed;
}

rapidjson::Value parseLiteral(const std::string& literal,
                              rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Document temp;
    temp.Parse(literal.c_str());
    if (!temp.HasParseError()) {
        return rapidjson::Value(temp, allocator);
    }
    rapidjson::Value value;
    value.SetString(literal.c_str(), allocator);
    return value;
}

std::string serializeJson(const rapidjson::Value& value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

void writeDocumentToFile(const rapidjson::Document& document,
                         const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("Failed to open config file for writing: " + path.string());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    stream << buffer.GetString() << '\n';
    if (!stream) {
        throw std::runtime_error("Failed to write config file: " + path.string());
    }
}

std::string makeProvenanceSummary(const ConfigSnapshot& snapshot,
                                  const std::string& key)
{
    auto itr = snapshot.provenance.find(key);
    if (itr == snapshot.provenance.end()) {
        return "No provenance entries";
    }
    std::ostringstream oss;
    oss << "Provenance for " << key << ":";
    for (const auto& record : itr->second) {
        oss << " [layer=" << static_cast<int>(record.layer)
            << ", source=" << record.source << "]";
    }
    return oss.str();
}

} // namespace

ConfigCommand::ConfigCommand() = default;

CLIExitCode ConfigCommand::run(const CommandExecutionContext& context)
{
    CLIExitCode errorCode = CLIExitCode::UnknownFlag;
    std::string errorMessage;
    auto args = parseArguments(context, errorCode, errorMessage);
    if (!args.has_value()) {
        emitCommandFailed(context, errorCode, errorMessage);
        return errorCode;
    }

    try {
        services::ProjectManifest manifest = services::ProjectManifestLoader::load(args->manifestPath);
        switch (args->operation) {
        case Operation::Show:
            return executeShow(context, *args, manifest);
        case Operation::Get:
            return executeGet(context, *args, manifest);
        case Operation::Set:
            return executeSet(context, *args, manifest);
        case Operation::Unset:
            return executeUnset(context, *args, manifest);
        }
    } catch (const std::exception& ex) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, ex.what());
        return CLIExitCode::RuntimeError;
    }

    emitCommandFailed(context, CLIExitCode::RuntimeError, "Unknown config operation");
    return CLIExitCode::RuntimeError;
}

std::optional<ConfigCommand::ParsedArgs> ConfigCommand::parseArguments(
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
        if (token == "--scope") {
            if (i + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --scope (expected workspace|project|global)";
                return std::nullopt;
            }
            args.scope = tokens[++i];
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
        if (token == "--explain") {
            args.explain = true;
            continue;
        }
        if (token == "--get") {
            if (args.operation != Operation::Show) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Only one of --get, --set, or --unset may be provided";
                return std::nullopt;
            }
            if (i + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --get (expected config key)";
                return std::nullopt;
            }
            args.operation = Operation::Get;
            args.key = tokens[++i];
            continue;
        }
        if (token == "--set") {
            if (args.operation != Operation::Show) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Only one of --get, --set, or --unset may be provided";
                return std::nullopt;
            }
            if (i + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --set (expected key=value)";
                return std::nullopt;
            }
            args.operation = Operation::Set;
            std::string assignment = tokens[++i];
            auto pos = assignment.find('=');
            if (pos == std::string::npos) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Expected key=value format for --set";
                return std::nullopt;
            }
            args.key = assignment.substr(0, pos);
            args.value = assignment.substr(pos + 1);
            continue;
        }
        if (token == "--unset") {
            if (args.operation != Operation::Show) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Only one of --get, --set, or --unset may be provided";
                return std::nullopt;
            }
            if (i + 1 >= tokens.size()) {
                errorCode = CLIExitCode::UnknownFlag;
                errorMessage = "Missing value for --unset (expected config key)";
                return std::nullopt;
            }
            args.operation = Operation::Unset;
            args.key = tokens[++i];
            continue;
        }

        errorCode = CLIExitCode::UnknownFlag;
        errorMessage = "Unknown argument for glint config: " + token;
        return std::nullopt;
    }

    if ((args.operation == Operation::Get || args.operation == Operation::Unset || args.operation == Operation::Set) && !args.key.has_value()) {
        errorCode = CLIExitCode::UnknownFlag;
        errorMessage = "Config operation requires a key";
        return std::nullopt;
    }
    if (args.operation == Operation::Set && !args.value.has_value()) {
        errorCode = CLIExitCode::UnknownFlag;
        errorMessage = "--set requires key=value";
        return std::nullopt;
    }
    return args;
}

CLIExitCode ConfigCommand::executeShow(const CommandExecutionContext& context,
                                       const ParsedArgs& args,
                                       const services::ProjectManifest& manifest) const
{
    ConfigResolver resolver;
    ConfigResolveRequest request;
    request.workspaceRoot = manifest.workspaceRoot;
    request.manifestPath = manifest.manifestPath;
    request.sceneId = args.sceneId.value_or("");
    ConfigSnapshot snapshot = resolver.resolve(request);

    if (context.globals.jsonOutput && context.emitter) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("config_snapshot");
            writer.Key("scope"); writer.String(args.scope.c_str());
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(buffer);
            snapshot.document.Accept(jsonWriter);
            writer.Key("config_json"); writer.String(buffer.GetString());
        });
    } else {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(buffer);
        snapshot.document.Accept(jsonWriter);
        std::cout << buffer.GetString() << std::endl;
    }

    emitCommandInfo(context, "Configuration snapshot emitted");
    return CLIExitCode::Success;
}

CLIExitCode ConfigCommand::executeGet(const CommandExecutionContext& context,
                                      const ParsedArgs& args,
                                      const services::ProjectManifest& manifest) const
{
    ConfigResolver resolver;
    ConfigResolveRequest request;
    request.workspaceRoot = manifest.workspaceRoot;
    request.manifestPath = manifest.manifestPath;
    request.sceneId = args.sceneId.value_or("");
    ConfigSnapshot snapshot = resolver.resolve(request);

    auto keyParts = splitDotKey(*args.key);
    const rapidjson::Value* value = findValueConst(snapshot.document, keyParts);
    if (!value) {
        emitCommandFailed(context, CLIExitCode::RuntimeError, "Config key not found: " + *args.key, "not_found");
        return CLIExitCode::RuntimeError;
    }

    std::string jsonValue = serializeJson(*value);
    if (context.globals.jsonOutput && context.emitter) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("config_lookup");
            writer.Key("key"); writer.String(args.key->c_str());
            writer.Key("value_json"); writer.String(jsonValue.c_str());
        });
    } else {
        std::cout << args.key.value() << " = " << jsonValue << std::endl;
    }

    if (args.explain) {
        emitCommandInfo(context, makeProvenanceSummary(snapshot, args.key.value()));
    }

    return CLIExitCode::Success;
}

CLIExitCode ConfigCommand::executeSet(const CommandExecutionContext& context,
                                      const ParsedArgs& args,
                                      const services::ProjectManifest& manifest) const
{
    auto keyParts = splitDotKey(*args.key);
    rapidjson::Document document;
    document.SetObject();

    std::filesystem::path targetPath;
    if (args.scope == "workspace") {
        targetPath = manifest.configDirectory / kWorkspaceConfigFilename;
        if (std::filesystem::exists(targetPath)) {
            std::ifstream stream(targetPath);
            if (stream) {
                rapidjson::IStreamWrapper wrapper(stream);
                document.ParseStream(wrapper);
                stream.close();
            }
        }
        if (!document.IsObject()) {
            document.SetObject();
        }
    } else if (args.scope == "global") {
        targetPath = glint::getConfigPath(kWorkspaceConfigFilename);
        if (std::filesystem::exists(targetPath)) {
            std::ifstream stream(targetPath);
            if (stream) {
                rapidjson::IStreamWrapper wrapper(stream);
                document.ParseStream(wrapper);
                stream.close();
            }
        }
        if (!document.IsObject()) {
            document.SetObject();
        }
    } else if (args.scope == "project") {
        std::ifstream stream(manifest.manifestPath);
        if (!stream) {
            emitCommandFailed(context, CLIExitCode::RuntimeError, "Failed to open manifest for editing", "io_error");
            return CLIExitCode::RuntimeError;
        }
        rapidjson::IStreamWrapper wrapper(stream);
        document.ParseStream(wrapper);
        stream.close();
        if (!document.IsObject()) {
            emitCommandFailed(context, CLIExitCode::RuntimeError, "Manifest is not a valid JSON object", "parse_error");
            return CLIExitCode::RuntimeError;
        }
        auto& allocator = document.GetAllocator();
        auto configurationMember = document.FindMember("configuration");
        if (configurationMember == document.MemberEnd()) {
            rapidjson::Value name("configuration", allocator);
            rapidjson::Value value(rapidjson::kObjectType);
            document.AddMember(name, value, allocator);
            configurationMember = document.FindMember("configuration");
        }
        rapidjson::Value& configuration = configurationMember->value;
        ensureObject(configuration, allocator);

        auto defaultsMember = configuration.FindMember(kDefaultsKey);
        if (defaultsMember == configuration.MemberEnd()) {
            rapidjson::Value name(kDefaultsKey, allocator);
            rapidjson::Value defaultsObj(rapidjson::kObjectType);
            configuration.AddMember(name, defaultsObj, allocator);
            defaultsMember = configuration.FindMember(kDefaultsKey);
        }
        rapidjson::Value& defaults = defaultsMember->value;
        ensureObject(defaults, allocator);

        rapidjson::Value newValue = parseLiteral(args.value.value(), allocator);
        assignDotKey(defaults, keyParts, 0, newValue, allocator);

        writeDocumentToFile(document, manifest.manifestPath);

        emitCommandInfo(context, "Updated project configuration defaults");
        if (context.globals.jsonOutput && context.emitter) {
            context.emitter->emit([&](auto& writer) {
                writer.Key("event"); writer.String("config_set");
                writer.Key("scope"); writer.String("project");
                writer.Key("key"); writer.String(args.key->c_str());
                writer.Key("value"); writer.String(args.value->c_str());
            });
        }

        return CLIExitCode::Success;
    } else {
        emitCommandFailed(context, CLIExitCode::UnknownFlag, "Unknown scope for config command: " + args.scope, "invalid_scope");
        return CLIExitCode::UnknownFlag;
    }

    rapidjson::Value newValue = parseLiteral(args.value.value(), document.GetAllocator());
    assignDotKey(document, keyParts, 0, newValue, document.GetAllocator());

    writeDocumentToFile(document, targetPath);

    emitCommandInfo(context, "Configuration value set at " + targetPath.string());
    if (context.globals.jsonOutput && context.emitter) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("config_set");
            writer.Key("scope"); writer.String(args.scope.c_str());
            writer.Key("key"); writer.String(args.key->c_str());
            writer.Key("value"); writer.String(args.value->c_str());
        });
    }

    return CLIExitCode::Success;
}

CLIExitCode ConfigCommand::executeUnset(const CommandExecutionContext& context,
                                        const ParsedArgs& args,
                                        const services::ProjectManifest& manifest) const
{
    auto keyParts = splitDotKey(*args.key);
    rapidjson::Document document;
    document.SetObject();

    std::filesystem::path targetPath;
    if (args.scope == "workspace") {
        targetPath = manifest.configDirectory / kWorkspaceConfigFilename;
        if (std::filesystem::exists(targetPath)) {
            std::ifstream stream(targetPath);
            if (stream) {
                rapidjson::IStreamWrapper wrapper(stream);
                document.ParseStream(wrapper);
                stream.close();
            }
        }
        if (!document.IsObject()) {
            document.SetObject();
        }
    } else if (args.scope == "global") {
        targetPath = glint::getConfigPath(kWorkspaceConfigFilename);
        if (std::filesystem::exists(targetPath)) {
            std::ifstream stream(targetPath);
            if (stream) {
                rapidjson::IStreamWrapper wrapper(stream);
                document.ParseStream(wrapper);
                stream.close();
            }
        }
        if (!document.IsObject()) {
            document.SetObject();
        }
    } else if (args.scope == "project") {
        std::ifstream stream(manifest.manifestPath);
        if (!stream) {
            emitCommandFailed(context, CLIExitCode::RuntimeError, "Failed to open manifest for editing", "io_error");
            return CLIExitCode::RuntimeError;
        }
        rapidjson::IStreamWrapper wrapper(stream);
        document.ParseStream(wrapper);
        stream.close();
        if (!document.IsObject()) {
            emitCommandFailed(context, CLIExitCode::RuntimeError, "Manifest is not a valid JSON object", "parse_error");
            return CLIExitCode::RuntimeError;
        }
        auto& allocator = document.GetAllocator();
        auto configurationMember = document.FindMember("configuration");
        if (configurationMember == document.MemberEnd() || !configurationMember->value.IsObject()) {
            emitCommandInfo(context, "No configuration defaults to unset");
            return CLIExitCode::Success;
        }
        rapidjson::Value& configuration = configurationMember->value;
        auto defaultsMember = configuration.FindMember(kDefaultsKey);
        if (defaultsMember == configuration.MemberEnd() || !defaultsMember->value.IsObject()) {
            emitCommandInfo(context, "No configuration defaults to unset");
            return CLIExitCode::Success;
        }
        rapidjson::Value& defaults = defaultsMember->value;

        bool removed = removeDotKey(defaults, keyParts, 0, allocator);
        if (!removed) {
            emitCommandInfo(context, "Key not present in configuration defaults");
            return CLIExitCode::Success;
        }

        writeDocumentToFile(document, manifest.manifestPath);

        if (context.globals.jsonOutput && context.emitter) {
            context.emitter->emit([&](auto& writer) {
                writer.Key("event"); writer.String("config_unset");
                writer.Key("scope"); writer.String("project");
                writer.Key("key"); writer.String(args.key->c_str());
            });
        } else {
            emitCommandInfo(context, "Unset project configuration key " + args.key.value());
        }
        return CLIExitCode::Success;
    } else {
        emitCommandFailed(context, CLIExitCode::UnknownFlag, "Unknown scope for config command: " + args.scope, "invalid_scope");
        return CLIExitCode::UnknownFlag;
    }

    bool removed = removeDotKey(document, keyParts, 0, document.GetAllocator());
    if (!removed) {
        emitCommandInfo(context, "Key not present in configuration file");
        return CLIExitCode::Success;
    }

    writeDocumentToFile(document, targetPath);

    if (context.globals.jsonOutput && context.emitter) {
        context.emitter->emit([&](auto& writer) {
            writer.Key("event"); writer.String("config_unset");
            writer.Key("scope"); writer.String(args.scope.c_str());
            writer.Key("key"); writer.String(args.key->c_str());
        });
    } else {
        emitCommandInfo(context, "Unset configuration key " + args.key.value());
    }

    return CLIExitCode::Success;
}

} // namespace glint::cli
