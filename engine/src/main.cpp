#include "application_core.h"
#include "render_utils.h"
#include "cli_parser.h"
#include "path_security.h"
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

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
// Global application pointer for JS bridge
static ApplicationCore* g_app = nullptr;
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    int app_apply_ops_json(const char* json)
    {
        if (!g_app || !json) return 0;
        std::string err; // collect error for debugging
        bool ok = g_app->applyJsonOpsV1(std::string(json), err);
        if (!ok) {
            emscripten_log(EM_LOG_CONSOLE, "applyJsonOpsV1 error: %s", err.c_str());
        }
        return ok ? 1 : 0;
    }

    EMSCRIPTEN_KEEPALIVE
    const char* app_share_link()
    {
        static std::string last;
        if (!g_app) return "";
        last = g_app->buildShareLink();
        return last.c_str();
    }

    EMSCRIPTEN_KEEPALIVE
    const char* app_scene_to_json()
    {
        static std::string last;
        if (!g_app) return "";
        last = g_app->sceneToJson();
        return last.c_str();
    }

    EMSCRIPTEN_KEEPALIVE
    int app_is_ready()
    {
        return g_app ? 1 : 0;
    }
}
#endif

namespace {
    static std::string loadTextFile(const std::string& path){ std::ifstream f(path, std::ios::binary); if(!f) return {}; std::ostringstream ss; ss<<f.rdbuf(); return ss.str(); }
}

static const char* GLINT_VERSION = "0.3.0";

int main(int argc, char** argv)
{
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
    
    // Configure schema validation
    if (parseResult.options.strictSchema) {
        Logger::debug("Enabling strict schema validation for " + parseResult.options.schemaVersion);
        app->setStrictSchema(true, parseResult.options.schemaVersion);
    }
    
    // Configure render settings again to apply shader-related values post-init
    app->setRenderSettings(parseResult.options.renderSettings);

#ifdef __EMSCRIPTEN__
    // Assign global for JS bridge
    g_app = app;
    if (parseResult.options.headlessMode) {
        Logger::error("Web build does not support headless mode");
        delete app;
        return static_cast<int>(CLIExitCode::RuntimeError);
    }
    // Web: parse ?state=... and replay ops (robust Base64URL + JSON envelope)
    {
        const char* param = emscripten_run_script_string(
            "(function(){var p=(new URLSearchParams(window.location.search)).get('state');return p?p:'';})()"
        );
        if (param && *param) {
            auto fromBase64Url = [](std::string s)->std::string {
                // Accept both standard and URL-safe base64. Strip whitespace.
                s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c){ return c=='\n'||c=='\r'||c=='\t'||c==' '; }), s.end());
                for (auto& c : s) { if (c=='-') c = '+'; else if (c=='_') c = '/'; }
                // Pad to multiple of 4
                while (s.size() % 4) s.push_back('=');
                auto val = [](char c)->int{
                    if (c>='A'&&c<='Z') return c-'A';
                    if (c>='a'&&c<='z') return c-'a'+26;
                    if (c>='0'&&c<='9') return c-'0'+52;
                    if (c=='+') return 62; if (c=='/') return 63; if (c=='=') return -2; return -1; };
                std::string out; out.reserve((s.size()*3)/4);
                int buf=0, bits=0;
                for (char ch : s){
                    int v = val(ch);
                    if (v == -2) break; // padding
                    if (v < 0) continue; // ignore non-base64
                    buf = (buf<<6) | v; bits += 6;
                    if (bits >= 8) { bits -= 8; out.push_back(char((buf>>bits)&0xFF)); }
                }
                return out;
            };

            std::string raw(param);
            std::string json;
            // Heuristic: if looks like JSON, accept directly, otherwise decode base64
            auto ltrim = [](const std::string& s){ size_t i=0; while(i<s.size() && (s[i]==' '||s[i]=='\n'||s[i]=='\r'||s[i]=='\t')) ++i; return s.substr(i); };
            std::string lt = ltrim(raw);
            if (!lt.empty() && (lt[0]=='{' || lt[0]=='[')) {
                json = raw;
            } else {
                json = fromBase64Url(raw);
            }

            if (!json.empty()) {
                std::string err;
                if (!app->applyJsonOpsV1(json, err)) {
                    emscripten_log(EM_LOG_CONSOLE, "State import failed: %s", err.c_str());
                }
            } else {
                emscripten_log(EM_LOG_CONSOLE, "State import: empty or undecodable 'state' param");
            }
        }
    }
    emscripten_set_main_loop_arg([](void* p){ static_cast<ApplicationCore*>(p)->frame(); }, app, 0, true);
#else
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
    } else {
        Logger::info("Launching UI mode");
        app->run();
        delete app;
        return 0;
    }
#endif
}
