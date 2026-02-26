// Machine Summary Block
// {"file":"cli/src/commands/ops_command.cpp","purpose":"Implements ops command for JSON operations execution.","depends_on":["glint/cli/commands/ops_command.h","glint/cli/logger.h","application_core.h","render_utils.h","path_security.h","render_settings.h"],"notes":["json_ops_v1_execution","headless_rendering","replaces_legacy_cli_parser"]}
// Human Summary
// Executes JSON operations files with optional rendering, replacing the legacy --ops flag implementation.

#include "glint/cli/commands/ops_command.h"
#include "glint/cli/logger.h"
#include "glint/cli/command_io.h"

#include "application_core.h"
#include "scene_manager.h"
#include "light.h"
#include "render_utils.h"
#include "path_security.h"
#include "render_settings.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace glint::cli {

namespace {

std::string loadTextFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool hasFlag(const std::vector<std::string>& args, const std::string& flag)
{
    return std::find(args.begin(), args.end(), flag) != args.end();
}

std::string getValue(const std::vector<std::string>& args,
                     const std::string& flag,
                     const std::string& defaultValue = "")
{
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == flag) {
            if (i + 1 >= args.size() || args[i + 1].rfind("--", 0) == 0) {
                return defaultValue;
            }
            return args[i + 1];
        }
    }
    return defaultValue;
}

int getIntValue(const std::vector<std::string>& args,
                const std::string& flag,
                int defaultValue)
{
    std::string val = getValue(args, flag);
    if (val.empty()) return defaultValue;
    try {
        return std::stoi(val);
    } catch (...) {
        return defaultValue;
    }
}

float getFloatValue(const std::vector<std::string>& args,
                    const std::string& flag,
                    float defaultValue)
{
    std::string val = getValue(args, flag);
    if (val.empty()) return defaultValue;
    try {
        return std::stof(val);
    } catch (...) {
        return defaultValue;
    }
}

bool hasSuffix(const std::string& text, const std::string& suffix)
{
    if (suffix.size() > text.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), text.rbegin());
}

std::string stripSuffix(const std::string& text, const std::string& suffix)
{
    if (!hasSuffix(text, suffix)) {
        return text;
    }
    return text.substr(0, text.size() - suffix.size());
}

bool collectBatchOpsFiles(const std::string& dirPath,
                         std::vector<std::filesystem::path>& outFiles,
                         std::string& errorMsg)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path dir(dirPath);
    if (!fs::exists(dir, ec) || ec) {
        errorMsg = "Batch ops directory not found: " + dirPath;
        return false;
    }
    if (!fs::is_directory(dir, ec) || ec) {
        errorMsg = "Batch ops path is not a directory: " + dirPath;
        return false;
    }

    outFiles.clear();
    for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec)) {
        if (ec) {
            errorMsg = "Failed to enumerate batch ops directory: " + ec.message();
            return false;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string filename = entry.path().filename().string();
        if (!hasSuffix(filename, ".ops.json")) {
            continue;
        }
        outFiles.push_back(entry.path());
    }

    std::sort(outFiles.begin(), outFiles.end(),
              [](const fs::path& a, const fs::path& b) {
                  return a.filename().string() < b.filename().string();
              });

    if (outFiles.empty()) {
        errorMsg = "No .ops.json files found in batch ops directory: " + dirPath;
        return false;
    }

    return true;
}

std::filesystem::path buildBatchOutputPath(const std::filesystem::path& renderDir,
                                           const std::filesystem::path& opsPath)
{
    std::string stem = opsPath.stem().string(); // "frame_000000.ops"
    stem = stripSuffix(stem, ".ops");
    return renderDir / (stem + ".png");
}

} // anonymous namespace

bool OpsCommand::parseOpsArguments(const std::vector<std::string>& args,
                                   OpsOptions& options,
                                   std::string& errorMsg)
{
    // Positional ops file (legacy/single mode) is optional when using batch mode flags.
    if (!args.empty() && args[0].rfind("--", 0) != 0) {
        options.opsFile = args[0];
    }

    // Parse flags
    options.enableDenoise = hasFlag(args, "--denoise");
    options.forceRaytrace = hasFlag(args, "--raytrace");
    options.strictSchema = hasFlag(args, "--strict-schema");
    options.shouldRender = hasFlag(args, "--render");
    options.selectionOverlay = hasFlag(args, "--selection-overlay");

    // Parse string values
    options.batchOpsDir = getValue(args, "--batch-dir");
    options.batchRenderDir = getValue(args, "--render-dir");
    options.outputFile = getValue(args, "--render");
    options.assetRoot = getValue(args, "--asset-root");
    options.schemaVersion = getValue(args, "--schema-version", "v1.3");
    options.toneMapping = getValue(args, "--tone", "linear");
    options.batchMode = !options.batchOpsDir.empty();

    // Parse integer values
    options.outputWidth = getIntValue(args, "--w", 1024);
    options.outputHeight = getIntValue(args, "--h", 1024);
    options.samples = getIntValue(args, "--samples", 1);
    options.reflectionSpp = getIntValue(args, "--refl-spp", 8);

    // Parse render settings
    std::string seedStr = getValue(args, "--seed", "0");
    try {
        options.seed = static_cast<uint32_t>(std::stoul(seedStr));
    } catch (...) {
        errorMsg = "Invalid seed value: " + seedStr + " (must be a non-negative integer)";
        return false;
    }

    options.exposure = getFloatValue(args, "--exposure", 0.0f);
    options.gamma = getFloatValue(args, "--gamma", 2.2f);

    // Validate required values
    if (hasFlag(args, "--asset-root") && options.assetRoot.empty()) {
        errorMsg = "Missing value for --asset-root (expected a directory path)";
        return false;
    }

    if (hasFlag(args, "--batch-dir") && options.batchOpsDir.empty()) {
        errorMsg = "Missing value for --batch-dir (expected a directory path)";
        return false;
    }

    if (hasFlag(args, "--render-dir") && options.batchRenderDir.empty()) {
        errorMsg = "Missing value for --render-dir (expected a directory path)";
        return false;
    }

    if (hasFlag(args, "--schema-version") && options.schemaVersion.empty()) {
        errorMsg = "Missing value for --schema-version (expected e.g. v1.3)";
        return false;
    }

    if (options.batchMode) {
        if (!options.opsFile.empty()) {
            errorMsg = "Batch mode does not take a positional ops file. Use: glint ops --batch-dir <dir> --render-dir <dir> [options]";
            return false;
        }
        if (options.batchRenderDir.empty()) {
            errorMsg = "Batch mode requires --render-dir <dir>";
            return false;
        }
        if (hasFlag(args, "--render") && !options.outputFile.empty()) {
            errorMsg = "Batch mode uses --render-dir, not --render <file>";
            return false;
        }
    } else if (options.opsFile.empty()) {
        errorMsg = "Missing operations file path. Usage: glint ops <file.json> [options]";
        return false;
    }

    // Validate schema version
    if (options.schemaVersion != "v1.3") {
        errorMsg = "Invalid schema version: " + options.schemaVersion + " (supported: v1.3)";
        return false;
    }

    // Validate tone mapping
    if (!RenderSettings::isValidToneMapping(options.toneMapping)) {
        errorMsg = "Invalid tone mapping: " + options.toneMapping + " (supported: linear, reinhard, aces, filmic)";
        return false;
    }

    // Validate numeric ranges
    if (options.samples < 1) {
        errorMsg = "Invalid samples value: must be >= 1";
        return false;
    }

    if (options.reflectionSpp < 1) {
        errorMsg = "Invalid reflection samples per pixel: must be >= 1";
        return false;
    }

    if (options.gamma <= 0.0f) {
        errorMsg = "Invalid gamma value: must be positive";
        return false;
    }

    return true;
}

CLIExitCode OpsCommand::executeOps(const OpsOptions& options,
                                   const CommandExecutionContext& context)
{
    if (options.batchMode) {
        return executeBatchOps(options, context);
    }

    // Set up logging level from global options
    glint::cli::Logger::setLevel(context.globals.logLevel);

    Logger::info("Glint 3D Engine v0.3.0");
    Logger::info("Executing JSON operations from: " + options.opsFile);

    // Initialize path security if asset root is provided
    if (!options.assetRoot.empty()) {
        if (!PathSecurity::setAssetRoot(options.assetRoot)) {
            Logger::error("Failed to set asset root: " + options.assetRoot);
            return CLIExitCode::RuntimeError;
        }
        Logger::info("Asset root set to: " + PathSecurity::getAssetRoot());
    }

    // Initialize application in headless mode
    auto* app = new ApplicationCore();

    // Configure render settings
    RenderSettings renderSettings;
    renderSettings.seed = options.seed;
    renderSettings.toneMapping = RenderSettings::parseToneMapping(options.toneMapping);
    renderSettings.exposure = options.exposure;
    renderSettings.gamma = options.gamma;
    renderSettings.samples = options.samples;

    app->setRenderSettings(renderSettings);

    if (!app->init("Glint 3D", options.outputWidth, options.outputHeight, true)) {
        Logger::error("Failed to initialize application");
        delete app;
        return CLIExitCode::RuntimeError;
    }

    // Configure additional settings
    if (options.enableDenoise) {
        Logger::debug("Enabling denoiser");
        app->setDenoiseEnabled(true);
    }

    if (options.forceRaytrace) {
        Logger::debug("Enabling raytracing mode");
        app->setRaytraceMode(true);
    }

    app->setReflectionSpp(options.reflectionSpp);
    app->setOffscreenSelectionOverlayEnabled(options.selectionOverlay);

    if (options.strictSchema) {
        Logger::debug("Enabling strict schema validation for " + options.schemaVersion);
        app->setStrictSchema(true, options.schemaVersion);
    }

    // Load and apply operations
    Logger::info("Loading operations from: " + options.opsFile);
    std::string ops = loadTextFile(options.opsFile);
    if (ops.empty()) {
        Logger::error("Failed to read operations file: " + options.opsFile);
        delete app;
        return CLIExitCode::FileNotFound;
    }

    std::string err;
    if (!app->applyJsonOpsV1(ops, err)) {
        Logger::error("Operations failed: " + err);
        delete app;
        // Check if it's a schema validation error
        if (options.strictSchema && err.find("Schema validation failed") != std::string::npos) {
            return CLIExitCode::SchemaValidationError;
        }
        return CLIExitCode::RuntimeError;
    }
    Logger::info("Operations applied successfully");

    // Render if requested
    if (options.shouldRender || !options.outputFile.empty()) {
        std::string outputPath = options.outputFile;
        if (outputPath.empty()) {
            // Generate default output path
            outputPath = RenderUtils::processOutputPath("");
        } else {
            outputPath = RenderUtils::processOutputPath(outputPath);
        }

        Logger::info("Rendering to: " + outputPath +
                    " (" + std::to_string(options.outputWidth) +
                    "x" + std::to_string(options.outputHeight) + ")");

        // Log render settings
        Logger::info("Render settings: seed=" + std::to_string(renderSettings.seed) +
                    ", tone=" + RenderSettings::toneMappingToString(renderSettings.toneMapping) +
                    ", exposure=" + std::to_string(renderSettings.exposure) +
                    ", gamma=" + std::to_string(renderSettings.gamma) +
                    ", samples=" + std::to_string(renderSettings.samples));

        if (!app->renderToPNG(outputPath, options.outputWidth, options.outputHeight)) {
            Logger::error("Render failed");
            delete app;
            return CLIExitCode::RuntimeError;
        }
        Logger::info("Render completed successfully");
    }

    delete app;
    return CLIExitCode::Success;
}

CLIExitCode OpsCommand::executeBatchOps(const OpsOptions& options,
                                        const CommandExecutionContext& context)
{
    namespace fs = std::filesystem;

    glint::cli::Logger::setLevel(context.globals.logLevel);
    Logger::info("Glint 3D Engine v0.3.0");
    Logger::info("Executing batch JSON operations from directory: " + options.batchOpsDir);

    if (!options.assetRoot.empty()) {
        if (!PathSecurity::setAssetRoot(options.assetRoot)) {
            Logger::error("Failed to set asset root: " + options.assetRoot);
            return CLIExitCode::RuntimeError;
        }
        Logger::info("Asset root set to: " + PathSecurity::getAssetRoot());
    }

    std::vector<fs::path> opsFiles;
    std::string batchError;
    if (!collectBatchOpsFiles(options.batchOpsDir, opsFiles, batchError)) {
        Logger::error(batchError);
        return CLIExitCode::FileNotFound;
    }

    std::error_code fsError;
    fs::create_directories(options.batchRenderDir, fsError);
    if (fsError) {
        Logger::error("Failed to create render output directory: " + options.batchRenderDir +
                      " (" + fsError.message() + ")");
        return CLIExitCode::RuntimeError;
    }

    auto* app = new ApplicationCore();

    RenderSettings renderSettings;
    renderSettings.seed = options.seed;
    renderSettings.toneMapping = RenderSettings::parseToneMapping(options.toneMapping);
    renderSettings.exposure = options.exposure;
    renderSettings.gamma = options.gamma;
    renderSettings.samples = options.samples;
    app->setRenderSettings(renderSettings);

    if (!app->init("Glint 3D", options.outputWidth, options.outputHeight, true)) {
        Logger::error("Failed to initialize application");
        delete app;
        return CLIExitCode::RuntimeError;
    }

    if (options.enableDenoise) {
        Logger::debug("Enabling denoiser");
        app->setDenoiseEnabled(true);
    }
    if (options.forceRaytrace) {
        Logger::debug("Enabling raytracing mode");
        app->setRaytraceMode(true);
    }
    app->setReflectionSpp(options.reflectionSpp);
    app->setOffscreenSelectionOverlayEnabled(options.selectionOverlay);
    if (options.strictSchema) {
        Logger::debug("Enabling strict schema validation for " + options.schemaVersion);
        app->setStrictSchema(true, options.schemaVersion);
    }

    Logger::info("Batch render count: " + std::to_string(opsFiles.size()));

    for (size_t i = 0; i < opsFiles.size(); ++i) {
        const fs::path& opsPath = opsFiles[i];
        const fs::path outputPath = buildBatchOutputPath(fs::path(options.batchRenderDir), opsPath);

        // Each staged preview ops file expects a fresh scene (and may add lights via preset ops).
        app->getSceneManager().clear();
        app->getLights().clearLights();

        Logger::info("[" + std::to_string(i + 1) + "/" + std::to_string(opsFiles.size()) +
                     "] Loading operations from: " + opsPath.string());
        std::string ops = loadTextFile(opsPath.string());
        if (ops.empty()) {
            Logger::error("Failed to read operations file: " + opsPath.string());
            delete app;
            return CLIExitCode::FileNotFound;
        }

        std::string err;
        if (!app->applyJsonOpsV1(ops, err)) {
            Logger::error("Operations failed: " + err);
            delete app;
            if (options.strictSchema && err.find("Schema validation failed") != std::string::npos) {
                return CLIExitCode::SchemaValidationError;
            }
            return CLIExitCode::RuntimeError;
        }

        std::error_code createDirEc;
        fs::create_directories(outputPath.parent_path(), createDirEc);
        if (createDirEc) {
            Logger::error("Failed to create frame output directory: " + outputPath.parent_path().string());
            delete app;
            return CLIExitCode::RuntimeError;
        }

        Logger::info("Rendering to: " + outputPath.string() +
                    " (" + std::to_string(options.outputWidth) +
                    "x" + std::to_string(options.outputHeight) + ")");
        if (!app->renderToPNG(outputPath.string(), options.outputWidth, options.outputHeight)) {
            Logger::error("Render failed");
            delete app;
            return CLIExitCode::RuntimeError;
        }
    }

    Logger::info("Batch render completed successfully");
    delete app;
    return CLIExitCode::Success;
}

CLIExitCode OpsCommand::run(const CommandExecutionContext& context)
{
    OpsOptions options;
    std::string errorMsg;

    if (!parseOpsArguments(context.arguments, options, errorMsg)) {
        Logger::error(errorMsg);
        emitCommandFailed(context, CLIExitCode::UnknownFlag, errorMsg, "parse_error");
        return CLIExitCode::UnknownFlag;
    }

    return executeOps(options, context);
}

} // namespace glint::cli
