// Machine Summary Block
// {"file":"engine/core/application/main.cpp","purpose":"Entry point dispatching CLI verbs before launching the interactive application.","depends_on":["application_core.h","render_utils.h","cli_parser.h","path_security.h","glint/cli/command_dispatcher.h"],"notes":["cli_dispatcher","legacy_ui_fallback","headless_support"]}
// Human Summary
// Dispatches supported CLI verbs via CommandDispatcher, falling back to the legacy application bootstrap for interactive mode.

#include "application_core.h"
#include "render_utils.h"
#include "cli_parser.h"
#include "path_security.h"
#include "glint/cli/command_dispatcher.h"
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <filesystem>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
    static std::string loadTextFile(const std::string& path){ std::ifstream f(path, std::ios::binary); if(!f) return {}; std::ostringstream ss; ss<<f.rdbuf(); return ss.str(); }
}

static const char* GLINT_VERSION = "0.3.0";

int main(int argc, char** argv)
{
    glint::cli::CommandDispatcher dispatcher;
    if (auto dispatched = dispatcher.tryRun(argc, argv)) {
        return *dispatched;
    }

    // Ensure Windows console uses UTF-8 so Unicode ASCII art renders correctly
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // Parse command line arguments
    auto parseResult = CLIParser::parse(argc, argv);
    
    // Handle parse errors
    if (parseResult.exitCode != CLIExitCode::Success) {
        Logger::error(parseResult.errorMessage);
        return static_cast<int>(parseResult.exitCode);
    }
    
    // Set up logging level
    Logger::setLevel(parseResult.options.logLevel);
    
    // Handle help and version
    if (parseResult.options.showHelp) {
        CLIParser::printHelp();
        return 0;
    }
    
    if (parseResult.options.showVersion) {
        CLIParser::printVersion();
        return 0;
    }
    
    Logger::info("Glint 3D Engine v" + std::string(GLINT_VERSION));
    
    // Initialize path security if asset root is provided
    if (!parseResult.options.assetRoot.empty()) {
        if (!PathSecurity::setAssetRoot(parseResult.options.assetRoot)) {
            Logger::error("Failed to set asset root: " + parseResult.options.assetRoot);
            return static_cast<int>(CLIExitCode::RuntimeError);
        }
        Logger::info("Asset root set to: " + PathSecurity::getAssetRoot());
    }
    
    // Initialize application
    auto* app = new ApplicationCore();
    int windowWidth = parseResult.options.headlessMode ? parseResult.options.outputWidth : 800;
    int windowHeight = parseResult.options.headlessMode ? parseResult.options.outputHeight : 600;
    
    // Configure render settings early so window hints (e.g., samples) can be applied
    app->setRenderSettings(parseResult.options.renderSettings);

    if (!app->init("Glint 3D", windowWidth, windowHeight, parseResult.options.headlessMode)) {
        Logger::error("Failed to initialize application");
        delete app;
        return static_cast<int>(CLIExitCode::RuntimeError);
    }
    
    // Configure application settings
    if (parseResult.options.enableDenoise) {
        Logger::debug("Enabling denoiser");
        app->setDenoiseEnabled(true);
    }
    
    if (parseResult.options.forceRaytrace) {
        Logger::debug("Enabling raytracing mode");
        app->setRaytraceMode(true);
    }
    
    // Configure reflection samples per pixel
    if (parseResult.options.reflectionSpp != 8) { // Only log if not default
        Logger::debug("Setting reflection samples per pixel to " + std::to_string(parseResult.options.reflectionSpp));
    }
    app->setReflectionSpp(parseResult.options.reflectionSpp);
    
    // Configure schema validation
    if (parseResult.options.strictSchema) {
        Logger::debug("Enabling strict schema validation for " + parseResult.options.schemaVersion);
        app->setStrictSchema(true, parseResult.options.schemaVersion);
    }
    
    // Configure render settings again to apply shader-related values post-init
    app->setRenderSettings(parseResult.options.renderSettings);

    if (parseResult.options.headlessMode) {
        Logger::info("Running in headless mode");
        
        // Apply ops if provided
        if (!parseResult.options.opsFile.empty()) {
            Logger::info("Loading operations from: " + parseResult.options.opsFile);
            std::string ops = loadTextFile(parseResult.options.opsFile);
            if (ops.empty()) {
                Logger::error("Failed to read operations file: " + parseResult.options.opsFile);
                delete app;
                return static_cast<int>(CLIExitCode::FileNotFound);
            }
            
            std::string err;
            if (!app->applyJsonOpsV1(ops, err)) {
                Logger::error("Operations failed: " + err);
                delete app;
                // Check if it's a schema validation error
                if (parseResult.options.strictSchema && err.find("Schema validation failed") != std::string::npos) {
                    return static_cast<int>(CLIExitCode::SchemaValidationError);
                }
                return static_cast<int>(CLIExitCode::RuntimeError);
            }
            Logger::info("Operations applied successfully");
        }

        // Render if requested
        if (!parseResult.options.outputFile.empty() || !parseResult.options.opsFile.empty()) {
            std::string outputPath = parseResult.options.outputFile;
            if (outputPath.empty()) {
                // Generate default output path
                outputPath = RenderUtils::processOutputPath("");
            } else {
                outputPath = RenderUtils::processOutputPath(outputPath);
            }
            
            Logger::info("Rendering to: " + outputPath + 
                        " (" + std::to_string(parseResult.options.outputWidth) + 
                        "x" + std::to_string(parseResult.options.outputHeight) + ")");
            
            // Log render settings
            const auto& rs = parseResult.options.renderSettings;
            Logger::info("Render settings: seed=" + std::to_string(rs.seed) + 
                        ", tone=" + RenderSettings::toneMappingToString(rs.toneMapping) + 
                        ", exposure=" + std::to_string(rs.exposure) + 
                        ", gamma=" + std::to_string(rs.gamma) + 
                        ", samples=" + std::to_string(rs.samples));
            
            if (!app->renderToPNG(outputPath, parseResult.options.outputWidth, parseResult.options.outputHeight)) {
                Logger::error("Render failed");
                delete app;
                return static_cast<int>(CLIExitCode::RuntimeError);
            }
            Logger::info("Render completed successfully");
        }
        
        delete app;
        return 0;
    }

    Logger::info("Launching UI mode");
    app->run();
    delete app;
    return 0;
}
#include "application_core.h"
#include "render_utils.h"
#include "cli_parser.h"
#include "path_security.h"
#include "glint/cli/command_dispatcher.h"
