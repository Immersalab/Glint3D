// Machine Summary Block
// {"file":"cli/src/commands/render_command.cpp","purpose":"Implements the Glint CLI render command with determinism logging.","depends_on":["glint/cli/commands/render_command.h","glint/cli/services/run_manifest_writer.h","glint/cli/command_io.h","application/cli_parser.h","render_offscreen.h","<chrono>","<filesystem>","<fstream>","<sstream>"],"notes":["determinism_logging","run_manifest_integration","offscreen_render_pipeline"]}
// Human Summary
// Orchestrates offscreen rendering, captures provenance metadata (platform, engine, determinism), and writes run manifests to `renders/<name>/run.json`.

#include "glint/cli/commands/render_command.h"
#include "glint/cli/services/run_manifest_writer.h"
#include "glint/cli/command_io.h"
#include "application/cli_parser.h"
#include "application/application_core.h"
#include <rapidjson/document.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <limits>
#include <iomanip>

#ifdef _WIN32
#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#include <windows.h>
#include <intrin.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#endif

namespace glint::cli {

namespace {

std::string getCpuInfo()
{
#ifdef _WIN32
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);
    char brand[64] = {0};
    *reinterpret_cast<int*>(brand) = cpuInfo[1];
    *reinterpret_cast<int*>(brand + 4) = cpuInfo[3];
    *reinterpret_cast<int*>(brand + 8) = cpuInfo[2];
    return std::string(brand);
#else
    return "Unknown CPU";
#endif
}

std::string getOsInfo()
{
#ifdef _WIN32
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown OS";
#endif
}

std::string getKernelInfo()
{
#ifdef _WIN32
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        std::ostringstream oss;
        oss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << "." << osvi.dwBuildNumber;
        return oss.str();
    }
    return "Unknown";
#else
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        return std::string(buffer.release);
    }
    return "Unknown";
#endif
}

std::string computeFileHash(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        return "";
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }

    // Simple hash: combine file size and first 1KB
    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(std::min<size_t>(1024, static_cast<size_t>(fileSize)));
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    std::hash<std::string> hasher;
    std::ostringstream oss;
    oss << std::hex << hasher(std::string(buffer.begin(), buffer.end())) << fileSize;
    return oss.str();
}

std::string generateRunId()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);

    std::ostringstream oss;
    oss << "run_" << ms << "_" << dis(gen);
    return oss.str();
}

bool parseVec3(const std::string& value, std::array<double, 3>& out)
{
    std::stringstream ss(value);
    std::string token;
    std::vector<double> nums;
    while (std::getline(ss, token, ',')) {
        try {
            nums.push_back(std::stod(token));
        } catch (...) {
            return false;
        }
    }
    if (nums.size() != 3) {
        return false;
    }
    out = {nums[0], nums[1], nums[2]};
    return true;
}

services::FrameRecord applyOverridesToFrame(const RenderCommand::RenderOptions& options,
                                            const services::FrameRecord& baseFrame)
{
    services::FrameRecord result = baseFrame;
    if (options.modelTranslate.has_value() && !result.transform.translation.has_value()) {
        result.transform.translation = options.modelTranslate;
    }
    if (options.modelRotateEuler.has_value() && !result.transform.rotationEuler.has_value()) {
        result.transform.rotationEuler = options.modelRotateEuler;
    }
    if (options.modelScale.has_value() && !result.transform.scale.has_value()) {
        result.transform.scale = options.modelScale;
    }
    if (options.cameraPos.has_value() && !result.camera.position.has_value()) {
        result.camera.position = options.cameraPos;
    }
    if (options.cameraTarget.has_value() && !result.camera.target.has_value()) {
        result.camera.target = options.cameraTarget;
    }
    if (options.cameraUp.has_value() && !result.camera.up.has_value()) {
        result.camera.up = options.cameraUp;
    }
    if (options.cameraFov.has_value() && !result.camera.fovDeg.has_value()) {
        result.camera.fovDeg = options.cameraFov;
    }
    return result;
}

bool parseAnimationScript(const std::string& scriptPath,
                          std::vector<services::FrameRecord>& framesOut,
                          int& defaultStart,
                          int& defaultEnd,
                          std::string& errorMessage)
{
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        errorMessage = "Failed to open animation script: " + scriptPath;
        return false;
    }
    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (!doc.IsObject()) {
        errorMessage = "Animation script must be a JSON object";
        return false;
    }
    if (!doc.HasMember("frames") || !doc["frames"].IsArray()) {
        errorMessage = "Animation script missing `frames` array";
        return false;
    }
    const auto& frames = doc["frames"];
    defaultStart = std::numeric_limits<int>::max();
    defaultEnd = std::numeric_limits<int>::min();
    for (auto& f : frames.GetArray()) {
        if (!f.IsObject() || !f.HasMember("frame") || !f["frame"].IsInt()) {
            errorMessage = "Each frame must be an object with integer `frame`";
            return false;
        }
        services::FrameRecord rec;
        rec.frame = f["frame"].GetInt();
        defaultStart = std::min(defaultStart, rec.frame);
        defaultEnd = std::max(defaultEnd, rec.frame);
        if (f.HasMember("model_transform") && f["model_transform"].IsObject()) {
            const auto& mt = f["model_transform"];
            if (mt.HasMember("translation") && mt["translation"].IsArray() && mt["translation"].Size() == 3) {
                rec.transform.translation = {mt["translation"][0].GetDouble(), mt["translation"][1].GetDouble(), mt["translation"][2].GetDouble()};
            }
            if (mt.HasMember("rotation_euler") && mt["rotation_euler"].IsArray() && mt["rotation_euler"].Size() == 3) {
                rec.transform.rotationEuler = {mt["rotation_euler"][0].GetDouble(), mt["rotation_euler"][1].GetDouble(), mt["rotation_euler"][2].GetDouble()};
            }
            if (mt.HasMember("scale") && mt["scale"].IsArray() && mt["scale"].Size() == 3) {
                rec.transform.scale = {mt["scale"][0].GetDouble(), mt["scale"][1].GetDouble(), mt["scale"][2].GetDouble()};
            }
        }
        if (f.HasMember("camera") && f["camera"].IsObject()) {
            const auto& cam = f["camera"];
            if (cam.HasMember("position") && cam["position"].IsArray() && cam["position"].Size() == 3) {
                rec.camera.position = {cam["position"][0].GetDouble(), cam["position"][1].GetDouble(), cam["position"][2].GetDouble()};
            }
            if (cam.HasMember("target") && cam["target"].IsArray() && cam["target"].Size() == 3) {
                rec.camera.target = {cam["target"][0].GetDouble(), cam["target"][1].GetDouble(), cam["target"][2].GetDouble()};
            }
            if (cam.HasMember("up") && cam["up"].IsArray() && cam["up"].Size() == 3) {
                rec.camera.up = {cam["up"][0].GetDouble(), cam["up"][1].GetDouble(), cam["up"][2].GetDouble()};
            }
            if (cam.HasMember("fov_deg") && cam["fov_deg"].IsNumber()) {
                rec.camera.fovDeg = cam["fov_deg"].GetDouble();
            }
        }
        framesOut.push_back(rec);
    }
    if (framesOut.empty()) {
        errorMessage = "Animation script contains no frames";
        return false;
    }
    return true;
}

} // namespace

CLIExitCode RenderCommand::run(const CommandExecutionContext& context)
{
    // Respect caller working directory (so relative ops/assets resolve where user invoked glint)
    std::filesystem::path originalCwd = std::filesystem::current_path();
    const char* callerCwd = std::getenv("GLINT_CALLER_CWD");
    if (callerCwd && *callerCwd) {
        std::error_code ec;
        std::filesystem::current_path(callerCwd, ec);
        if (ec) {
            emitCommandWarning(context, std::string("Warning: could not use caller working directory: ") + ec.message());
        }
    }

    RenderOptions options;
    std::string errorMessage;

    CLIExitCode parseResult = parseArguments(context.arguments, options, errorMessage);
    if (parseResult != CLIExitCode::Success) {
        emitCommandFailed(context, parseResult, errorMessage, "argument_error");
        return parseResult;
    }

    return executeRender(context, options);
}

CLIExitCode RenderCommand::parseArguments(const std::vector<std::string>& args,
                                         RenderOptions& options,
                                         std::string& errorMessage) const
{
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "--json") {
            // Already handled by dispatcher
            continue;
        }

        if (arg == "--output" || arg == "-o") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for " + arg;
                return CLIExitCode::RuntimeError;
            }
            options.outputPath = args[++i];
        }
        else if (arg == "--ops") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --ops";
                return CLIExitCode::RuntimeError;
            }
            options.opsPath = args[++i];
        }
        else if (arg == "--input" || arg == "-i") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for " + arg;
                return CLIExitCode::RuntimeError;
            }
            options.inputPath = args[++i];
        }
        else if (arg == "--width" || arg == "-w") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for " + arg;
                return CLIExitCode::RuntimeError;
            }
            try {
                options.width = std::stoi(args[++i]);
                if (options.width <= 0 || options.width > 16384) {
                    errorMessage = "Width must be between 1 and 16384";
                    return CLIExitCode::RuntimeError;
                }
            } catch (...) {
                errorMessage = "Invalid width value: " + args[i];
                return CLIExitCode::RuntimeError;
            }
        }
        else if (arg == "--height" || arg == "-h") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for " + arg;
                return CLIExitCode::RuntimeError;
            }
            try {
                options.height = std::stoi(args[++i]);
                if (options.height <= 0 || options.height > 16384) {
                    errorMessage = "Height must be between 1 and 16384";
                    return CLIExitCode::RuntimeError;
                }
            } catch (...) {
                errorMessage = "Invalid height value: " + args[i];
                return CLIExitCode::RuntimeError;
            }
        }
        else if (arg == "--denoise") {
            options.denoise = true;
        }
        else if (arg == "--raytrace") {
            options.raytrace = true;
        }
        else if (arg == "--name") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --name";
                return CLIExitCode::RuntimeError;
            }
            options.renderName = args[++i];
        }
        else if (arg == "--no-manifest") {
            options.writeManifest = false;
        }
        else if (arg == "--animation-script") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --animation-script";
                return CLIExitCode::RuntimeError;
            }
            options.animationScriptPath = args[++i];
            options.sequenceMode = true;
        }
        else if (arg == "--png-sequence-out") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --png-sequence-out";
                return CLIExitCode::RuntimeError;
            }
            options.pngSequenceOut = args[++i];
            options.sequenceMode = true;
        }
        else if (arg == "--frame-start") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --frame-start";
                return CLIExitCode::RuntimeError;
            }
            options.frameStart = std::stoi(args[++i]);
            options.sequenceMode = true;
        }
        else if (arg == "--frame-end") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --frame-end";
                return CLIExitCode::RuntimeError;
            }
            options.frameEnd = std::stoi(args[++i]);
            options.sequenceMode = true;
        }
        else if (arg == "--frame-step") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --frame-step";
                return CLIExitCode::RuntimeError;
            }
            options.frameStep = std::max(1, std::stoi(args[++i]));
            options.sequenceMode = true;
        }
        else if (arg == "--camera-pos") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --camera-pos";
                return CLIExitCode::RuntimeError;
            }
            std::array<double, 3> vec{};
            if (!parseVec3(args[++i], vec)) {
                errorMessage = "Invalid vector for --camera-pos (expected x,y,z)";
                return CLIExitCode::RuntimeError;
            }
            options.cameraPos = vec;
        }
        else if (arg == "--camera-target") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --camera-target";
                return CLIExitCode::RuntimeError;
            }
            std::array<double, 3> vec{};
            if (!parseVec3(args[++i], vec)) {
                errorMessage = "Invalid vector for --camera-target (expected x,y,z)";
                return CLIExitCode::RuntimeError;
            }
            options.cameraTarget = vec;
        }
        else if (arg == "--camera-up") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --camera-up";
                return CLIExitCode::RuntimeError;
            }
            std::array<double, 3> vec{};
            if (!parseVec3(args[++i], vec)) {
                errorMessage = "Invalid vector for --camera-up (expected x,y,z)";
                return CLIExitCode::RuntimeError;
            }
            options.cameraUp = vec;
        }
        else if (arg == "--camera-fov") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --camera-fov";
                return CLIExitCode::RuntimeError;
            }
            try {
                options.cameraFov = std::stod(args[++i]);
            } catch (...) {
                errorMessage = "Invalid value for --camera-fov";
                return CLIExitCode::RuntimeError;
            }
        }
        else if (arg == "--model-translate") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --model-translate";
                return CLIExitCode::RuntimeError;
            }
            std::array<double, 3> vec{};
            if (!parseVec3(args[++i], vec)) {
                errorMessage = "Invalid vector for --model-translate (expected x,y,z)";
                return CLIExitCode::RuntimeError;
            }
            options.modelTranslate = vec;
        }
        else if (arg == "--model-rotate-euler") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --model-rotate-euler";
                return CLIExitCode::RuntimeError;
            }
            std::array<double, 3> vec{};
            if (!parseVec3(args[++i], vec)) {
                errorMessage = "Invalid vector for --model-rotate-euler (expected x,y,z in degrees)";
                return CLIExitCode::RuntimeError;
            }
            options.modelRotateEuler = vec;
        }
        else if (arg == "--model-scale") {
            if (i + 1 >= args.size()) {
                errorMessage = "Missing value for --model-scale";
                return CLIExitCode::RuntimeError;
            }
            std::array<double, 3> vec{};
            if (!parseVec3(args[++i], vec)) {
                errorMessage = "Invalid vector for --model-scale (expected x,y,z)";
                return CLIExitCode::RuntimeError;
            }
            options.modelScale = vec;
        }
        else if (!arg.empty() && arg[0] == '-') {
            errorMessage = "Unknown flag: " + arg;
            return CLIExitCode::UnknownFlag;
        }
        else {
            // Positional argument (treat as output path if not set)
            if (options.outputPath.empty()) {
                options.outputPath = arg;
            } else {
                errorMessage = "Unexpected positional argument: " + arg;
                return CLIExitCode::RuntimeError;
            }
        }
    }

    // Derive defaults when only --ops is provided
    if (options.outputPath.empty() && !options.opsPath.empty()) {
        std::filesystem::path opsPath(options.opsPath);
        auto stem = opsPath.stem().string();
        if (stem.empty()) {
            stem = "render";
        }
        if (options.renderName == "default") {
            options.renderName = stem;
        }
        options.outputPath = stem + ".png";
    }

    if (options.sequenceMode) {
        if (options.animationScriptPath.empty()) {
            errorMessage = "Animation mode requires --animation-script";
            return CLIExitCode::RuntimeError;
        }
        if (options.pngSequenceOut.empty()) {
            options.pngSequenceOut = (std::filesystem::path("renders") / options.renderName / "frames").string();
        }
        if (options.frameEnd < options.frameStart) {
            options.frameEnd = options.frameStart;
        }
    } else {
        // Validation
        if (options.outputPath.empty()) {
            errorMessage = "Missing required --output path";
            return CLIExitCode::UnknownFlag;
        }
    }

    if (options.inputPath.empty() && options.opsPath.empty()) {
        errorMessage = "Must specify either --input or --ops";
        return CLIExitCode::UnknownFlag;
    }

    return CLIExitCode::Success;
}

CLIExitCode RenderCommand::executeRender(const CommandExecutionContext& context,
                                         const RenderOptions& options) const
{
    using namespace services;

    std::string warningMessage;
    bool renderSuccess = false;
    std::vector<FrameRecord> manifestFrames;
    AnimationMetadata animationMeta;
    std::vector<int> determinismFrames;

    try {
        // Verify input files exist
        if (!options.inputPath.empty() && !std::filesystem::exists(options.inputPath)) {
            emitCommandFailed(context, CLIExitCode::FileNotFound,
                            "Input file not found: " + options.inputPath,
                            "file_not_found");
            return CLIExitCode::FileNotFound;
        }

        if (!options.opsPath.empty() && !std::filesystem::exists(options.opsPath)) {
            emitCommandFailed(context, CLIExitCode::FileNotFound,
                            "Ops file not found: " + options.opsPath,
                            "file_not_found");
            return CLIExitCode::FileNotFound;
        }

        // Initialize headless application
        ApplicationCore app;
        if (!app.init("Glint Headless Render", options.width, options.height, true)) {
            emitCommandFailed(context, CLIExitCode::RuntimeError,
                            "Failed to initialize rendering engine",
                            "init_failed");
            return CLIExitCode::RuntimeError;
        }

        // Configure render settings
        app.setRaytraceMode(options.raytrace);
        app.setDenoiseEnabled(options.denoise);

        // Load scene via JSON ops if provided
        if (!options.opsPath.empty()) {
            std::ifstream opsFile(options.opsPath);
            if (!opsFile.is_open()) {
                emitCommandFailed(context, CLIExitCode::FileNotFound,
                                "Failed to open ops file: " + options.opsPath,
                                "file_not_found");
                return CLIExitCode::FileNotFound;
            }

            std::string opsJson((std::istreambuf_iterator<char>(opsFile)),
                               std::istreambuf_iterator<char>());
            std::string error;
            if (!app.applyJsonOpsV1(opsJson, error)) {
                emitCommandFailed(context, CLIExitCode::RuntimeError,
                                "Failed to apply JSON ops: " + error,
                                "ops_failed");
                return CLIExitCode::RuntimeError;
            }
        } else if (!options.inputPath.empty()) {
            // Load scene directly (if it's a scene JSON file)
            std::ifstream sceneFile(options.inputPath);
            if (!sceneFile.is_open()) {
                emitCommandFailed(context, CLIExitCode::FileNotFound,
                                "Failed to open scene file: " + options.inputPath,
                                "file_not_found");
                return CLIExitCode::FileNotFound;
            }

            std::string sceneJson((std::istreambuf_iterator<char>(sceneFile)),
                                 std::istreambuf_iterator<char>());
            std::string error;
            if (!app.applyJsonOpsV1(sceneJson, error)) {
                emitCommandFailed(context, CLIExitCode::RuntimeError,
                                "Failed to load scene: " + error,
                                "scene_load_failed");
                return CLIExitCode::RuntimeError;
            }
        }

        std::filesystem::path renderDir = std::filesystem::path("renders") / options.renderName;
        std::filesystem::create_directories(renderDir);

        if (options.sequenceMode) {
            // Parse animation script
            if (!std::filesystem::exists(options.animationScriptPath)) {
                emitCommandFailed(context, CLIExitCode::FileNotFound,
                                "Animation script not found: " + options.animationScriptPath,
                                "file_not_found");
                return CLIExitCode::FileNotFound;
            }
            std::vector<FrameRecord> scriptFrames;
            int defaultStart = 0;
            int defaultEnd = 0;
            std::string parseError;
            if (!parseAnimationScript(options.animationScriptPath, scriptFrames, defaultStart, defaultEnd, parseError)) {
                emitCommandFailed(context, CLIExitCode::RuntimeError, parseError, "animation_parse_error");
                return CLIExitCode::RuntimeError;
            }
            int frameStart = (options.frameStart == 0 && options.frameEnd == 0) ? defaultStart : options.frameStart;
            int frameEnd = (options.frameStart == 0 && options.frameEnd == 0) ? defaultEnd : options.frameEnd;
            if (frameEnd < frameStart) {
                frameEnd = frameStart;
            }
            std::filesystem::path frameDir = options.pngSequenceOut.empty()
                                                 ? renderDir / "frames"
                                                 : std::filesystem::path(options.pngSequenceOut);
            std::filesystem::create_directories(frameDir);

            for (const auto& frame : scriptFrames) {
                if (frame.frame < frameStart || frame.frame > frameEnd) {
                    continue;
                }
                if (((frame.frame - frameStart) % options.frameStep) != 0) {
                    continue;
                }
                FrameRecord resolved = applyOverridesToFrame(options, frame);
                std::ostringstream fname;
                fname << "frame_" << std::setfill('0') << std::setw(4) << frame.frame << ".png";
                std::filesystem::path outPath = frameDir / fname.str();
                auto perFrameStart = std::chrono::high_resolution_clock::now();
                renderSuccess = app.renderToPNG(outPath.string(), options.width, options.height);
                auto perFrameEnd = std::chrono::high_resolution_clock::now();
                resolved.durationMs = std::chrono::duration<double, std::milli>(perFrameEnd - perFrameStart).count();
                // Store output relative to renderDir when possible
                std::error_code relEc;
                auto relPath = std::filesystem::relative(outPath, renderDir, relEc);
                resolved.output = relEc ? outPath.generic_string() : relPath.generic_string();
                manifestFrames.push_back(resolved);
                determinismFrames.push_back(resolved.frame);
                if (!renderSuccess) {
                    emitCommandFailed(context, CLIExitCode::RuntimeError,
                                    "Rendering failed for frame " + std::to_string(frame.frame),
                                    "render_failed");
                    return CLIExitCode::RuntimeError;
                }
            }

            if (manifestFrames.empty()) {
                emitCommandFailed(context, CLIExitCode::RuntimeError,
                                "No frames rendered: check frame range/step and script contents",
                                "animation_no_frames");
                return CLIExitCode::RuntimeError;
            }

            animationMeta.enabled = true;
            animationMeta.frameStart = frameStart;
            animationMeta.frameEnd = frameEnd;
            animationMeta.frameStep = options.frameStep;
            animationMeta.type = "keyframe";
            animationMeta.scriptPath = options.animationScriptPath;
            animationMeta.frames = manifestFrames;
            renderSuccess = true;
        } else {
            // Single-frame render
            std::filesystem::path fullOutputPath = renderDir / std::filesystem::path(options.outputPath).filename();
            auto perFrameStart = std::chrono::high_resolution_clock::now();
            renderSuccess = app.renderToPNG(fullOutputPath.string(), options.width, options.height);
            auto perFrameEnd = std::chrono::high_resolution_clock::now();
            double durationMs = std::chrono::duration<double, std::milli>(perFrameEnd - perFrameStart).count();

            if (!renderSuccess) {
                emitCommandFailed(context, CLIExitCode::RuntimeError,
                                "Rendering failed - check console for details",
                                "render_failed");
                return CLIExitCode::RuntimeError;
            }

            FrameRecord frame;
            frame.frame = 0;
            frame.durationMs = durationMs;
            frame.output = std::filesystem::path(options.outputPath).filename().string();
            manifestFrames.push_back(frame);
            determinismFrames.push_back(0);
        }

        app.shutdown();

    } catch (const std::exception& e) {
        emitCommandFailed(context, CLIExitCode::RuntimeError,
                        std::string("Render failed: ") + e.what(),
                        "render_error");
        return CLIExitCode::RuntimeError;
    }

    if (!renderSuccess) {
        emitCommandFailed(context, CLIExitCode::RuntimeError,
                        "Render failed",
                        "render_failed");
        return CLIExitCode::RuntimeError;
    }

    // Write run manifest if requested
    if (options.writeManifest) {
        try {
            std::filesystem::path renderDir = std::filesystem::path("renders") / options.renderName;
            std::filesystem::create_directories(renderDir);

            std::filesystem::path manifestPath = renderDir / "run.json";

            RunManifestWriter writer(manifestPath);
            RunManifestOptions manifestOpts;
            manifestOpts.runId = generateRunId();
            manifestOpts.outputDirectory = renderDir;

            // CLI metadata
            manifestOpts.cli.command = "render";
            manifestOpts.cli.arguments = context.arguments;
            manifestOpts.cli.jsonMode = (context.emitter != nullptr);
            if (!context.globals.projectPath.empty()) {
                manifestOpts.cli.projectPath = context.globals.projectPath;
            }

            // Platform metadata
            manifestOpts.platform = capturePlatformMetadata();

            // Engine metadata
            manifestOpts.engine = captureEngineMetadata();

            // Determinism metadata
            manifestOpts.determinism = captureDeterminismMetadata(options, determinismFrames);

            // Frames and optional animation metadata
            manifestOpts.frames = manifestFrames;
            manifestOpts.animation = animationMeta;

            // Warnings
            if (!warningMessage.empty()) {
                manifestOpts.warnings.push_back(warningMessage);
            }

            manifestOpts.exitCode = CLIExitCode::Success;

            writer.write(manifestOpts);

            emitCommandInfo(context, "Run manifest written to: " + manifestPath.string());

        } catch (const std::exception& e) {
            std::string msg = "Warning: Failed to write run manifest: ";
            msg += e.what();
            emitCommandWarning(context, msg);
        }
    }

    emitCommandInfo(context, "Render completed successfully");
    return CLIExitCode::Success;
}

services::PlatformMetadata RenderCommand::capturePlatformMetadata() const
{
    services::PlatformMetadata meta;
    meta.operatingSystem = getOsInfo();
    meta.cpu = getCpuInfo();
    meta.gpu = "Unknown GPU"; // TODO: Query from OpenGL/Vulkan
    meta.driverVersion = "Unknown";
    meta.kernel = getKernelInfo();
    return meta;
}

services::EngineMetadata RenderCommand::captureEngineMetadata() const
{
    services::EngineMetadata meta;
    meta.version = "0.3.0"; // TODO: Get from engine version header

    // TODO: Query actual module registry
    // For now, return placeholder data

    return meta;
}

services::DeterminismMetadata RenderCommand::captureDeterminismMetadata(const RenderOptions& options,
                                                                        const std::vector<int>& frames) const
{
    services::DeterminismMetadata meta;

    // Use deterministic seed (could be made configurable)
    meta.rngSeed = 42;

    if (!frames.empty()) {
        meta.frames = frames;
    } else {
        meta.frames.push_back(0);
    }

    // Compute digests of input files
    if (!options.inputPath.empty()) {
        meta.sceneDigest = computeFileHash(options.inputPath);
    }

    if (!options.opsPath.empty()) {
        meta.configDigest = computeFileHash(options.opsPath);
    }

    // TODO: Capture shader hashes from compiled shaders directory
    // TODO: Capture git revision from repository

    return meta;
}

} // namespace glint::cli
