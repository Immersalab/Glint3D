// Machine Summary Block
// {"file":"cli/src/services/run_manifest_writer.cpp","purpose":"Implements deterministic render run manifest writing for the CLI.","depends_on":["glint/cli/services/run_manifest_writer.h","glint/cli/services/run_manifest_validator.h","rapidjson/stringbuffer.h","rapidjson/writer.h","<chrono>","<filesystem>","<fstream>","<iomanip>","<sstream>"],"notes":["iso8601_timestamps","lf_utf8_output","schema_versioned_payload","validation_before_write"]}
// Human Summary
// Serialises run manifest metadata into `renders/<name>/run.json`, ensuring reproducibility fields and exit codes follow the CLI contract. Validates output against schema before writing.

#include "glint/cli/services/run_manifest_writer.h"
#include "glint/cli/services/run_manifest_validator.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace glint::cli::services {

namespace {

std::string defaultTimestampUtc()
{
    auto now = std::chrono::system_clock::now();
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now - seconds).count();

    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tmSnapshot{};
#ifdef _WIN32
    gmtime_s(&tmSnapshot, &time);
#else
    gmtime_r(&time, &tmSnapshot);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmSnapshot, "%Y-%m-%dT%H:%M:%S");
    oss << '.'
        << std::setfill('0') << std::setw(9) << nanos
        << "Z";
    return oss.str();
}

void ensureParentExists(const std::filesystem::path& filePath)
{
    auto parent = filePath.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent);
    }
}

template <typename Writer>
void writeCliSection(Writer& writer, const RunManifestOptions& options)
{
    writer.Key("cli");
    writer.StartObject();
    writer.Key("command");
    writer.String(options.cli.command.c_str());
    writer.Key("arguments");
    writer.StartArray();
    for (const auto& arg : options.cli.arguments) {
        writer.String(arg.c_str());
    }
    writer.EndArray();
    writer.Key("exit_code");
    writer.Int(static_cast<int>(options.exitCode));
    writer.Key("exit_code_name");
    writer.String(CLIParser::exitCodeToString(options.exitCode));
    writer.Key("json_mode");
    writer.Bool(options.cli.jsonMode);
    if (!options.cli.projectPath.empty()) {
        writer.Key("project");
        writer.String(options.cli.projectPath.c_str());
    }
    writer.EndObject();
}

template <typename Writer>
void writePlatformSection(Writer& writer, const PlatformMetadata& platform)
{
    writer.Key("platform");
    writer.StartObject();
    writer.Key("os");
    writer.String(platform.operatingSystem.c_str());
    writer.Key("cpu");
    writer.String(platform.cpu.c_str());
    writer.Key("gpu");
    writer.String(platform.gpu.c_str());
    writer.Key("driver_version");
    writer.String(platform.driverVersion.c_str());
    writer.Key("kernel");
    writer.String(platform.kernel.c_str());
    writer.EndObject();
}

template <typename Writer>
void writeEngineSection(Writer& writer, const EngineMetadata& engine)
{
    writer.Key("engine");
    writer.StartObject();
    writer.Key("version");
    writer.String(engine.version.c_str());

    writer.Key("modules");
    writer.StartArray();
    for (const auto& module : engine.modules) {
        writer.StartObject();
        writer.Key("name");
        writer.String(module.name.c_str());
        writer.Key("version");
        writer.String(module.version.c_str());
        if (!module.hash.empty()) {
            writer.Key("hash");
            writer.String(module.hash.c_str());
        }
        writer.Key("enabled");
        writer.Bool(module.enabled);
        writer.EndObject();
    }
    writer.EndArray();

    writer.Key("assets");
    writer.StartArray();
    for (const auto& asset : engine.assets) {
        writer.StartObject();
        writer.Key("name");
        writer.String(asset.name.c_str());
        writer.Key("version");
        writer.String(asset.version.c_str());
        if (!asset.hash.empty()) {
            writer.Key("hash");
            writer.String(asset.hash.c_str());
        }
        if (!asset.status.empty()) {
            writer.Key("status");
            writer.String(asset.status.c_str());
        }
        writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
}

template <typename Writer>
void writeDeterminismSection(Writer& writer, const DeterminismMetadata& determinism)
{
    writer.Key("determinism");
    writer.StartObject();
    writer.Key("rng_seed");
    writer.Int64(determinism.rngSeed);

    writer.Key("frame_batch");
    writer.StartArray();
    for (int frame : determinism.frames) {
        writer.Int(frame);
    }
    writer.EndArray();

    if (!determinism.configDigest.empty()) {
        writer.Key("config_digest");
        writer.String(determinism.configDigest.c_str());
    }
    if (!determinism.sceneDigest.empty()) {
        writer.Key("scene_digest");
        writer.String(determinism.sceneDigest.c_str());
    }
    if (!determinism.templateName.empty()) {
        writer.Key("template");
        writer.String(determinism.templateName.c_str());
    }
    if (!determinism.gitRevision.empty()) {
        writer.Key("git_revision");
        writer.String(determinism.gitRevision.c_str());
    }
    if (!determinism.shaderHashes.empty()) {
        writer.Key("shader_hashes");
        writer.StartArray();
        for (const auto& hash : determinism.shaderHashes) {
            writer.String(hash.c_str());
        }
        writer.EndArray();
    }
    writer.EndObject();
}

template <typename Writer>
void writeOutputsSection(Writer& writer,
                         const std::filesystem::path& outputDirectory,
                         const std::vector<FrameRecord>& frames,
                         const std::vector<std::string>& warnings)
{
    writer.Key("outputs");
    writer.StartObject();
    writer.Key("render_path");
    writer.String(outputDirectory.generic_string().c_str());

    writer.Key("frames");
    writer.StartArray();
    for (const auto& frame : frames) {
        writer.StartObject();
        writer.Key("frame");
        writer.Int(frame.frame);
        writer.Key("duration_ms");
        writer.Double(frame.durationMs);
        if (!frame.output.empty()) {
            writer.Key("output");
            writer.String(frame.output.c_str());
        }
        writer.EndObject();
    }
    writer.EndArray();

    writer.Key("warnings");
    writer.StartArray();
    for (const auto& warning : warnings) {
        writer.String(warning.c_str());
    }
    writer.EndArray();
    writer.EndObject();
}

} // namespace

RunManifestWriter::RunManifestWriter(std::filesystem::path manifestPath)
    : m_manifestPath(std::move(manifestPath))
{
}

void RunManifestWriter::write(const RunManifestOptions& options) const
{
    if (options.runId.empty()) {
        throw std::runtime_error("RunManifestWriter::write requires a non-empty runId");
    }

    ensureParentExists(m_manifestPath);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("schema_version");
    writer.String(options.schemaVersion.c_str());
    writer.Key("run_id");
    writer.String(options.runId.c_str());
    writer.Key("timestamp_utc");
    const std::string timestamp = options.timestampUtc.empty()
        ? defaultTimestampUtc()
        : options.timestampUtc;
    writer.String(timestamp.c_str());

    writeCliSection(writer, options);
    writePlatformSection(writer, options.platform);
    writeEngineSection(writer, options.engine);
    writeDeterminismSection(writer, options.determinism);
    writeOutputsSection(writer, options.outputDirectory, options.frames, options.warnings);

    writer.EndObject();

    // Validate the generated manifest before writing
    std::string jsonContent = buffer.GetString();
    ValidationResult validation = RunManifestValidator::validateContent(jsonContent, true);

    if (!validation.isValid()) {
        std::ostringstream oss;
        oss << "Generated manifest failed validation:\n" << validation.getSummary();
        throw std::runtime_error(oss.str());
    }

    // Write to disk
    std::ofstream stream(m_manifestPath, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to open run manifest for writing: " + m_manifestPath.string());
    }
    stream << jsonContent << '\n';
    if (!stream) {
        throw std::runtime_error("Failed to write run manifest to: " + m_manifestPath.string());
    }

    // Write checksum file alongside manifest
    if (validation.checksum.has_value()) {
        std::filesystem::path checksumPath = m_manifestPath;
        checksumPath.replace_extension(".json.sha256");

        std::ofstream checksumStream(checksumPath);
        if (checksumStream) {
            checksumStream << validation.checksum.value() << " *" << m_manifestPath.filename().string() << '\n';
        }
    }
}

} // namespace glint::cli::services
