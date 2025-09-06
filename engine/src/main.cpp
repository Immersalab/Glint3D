#include "application_core.h"
#include "render_utils.h"
#include "cli_parser.h"
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
    
    // Initialize application
    auto* app = new ApplicationCore();
    int windowWidth = parseResult.options.headlessMode ? parseResult.options.outputWidth : 800;
    int windowHeight = parseResult.options.headlessMode ? parseResult.options.outputHeight : 600;
    
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

#ifdef __EMSCRIPTEN__
    // Assign global for JS bridge
    g_app = app;
    if (parseResult.options.headlessMode) {
        Logger::error("Web build does not support headless mode");
        delete app;
        return static_cast<int>(CLIExitCode::RuntimeError);
    }
    // Web: parse ?state=... and replay ops
    {
        const char* s64 = emscripten_run_script_string("(function(){var p=(new URLSearchParams(window.location.search)).get('state');return p?p:'';})()");
        if (s64 && *s64) {
            // Base64 decode (URL-safe variants not used here)
            auto b64dec = [](const std::string& in){
                auto val = [](char c)->int{
                    if (c>='A'&&c<='Z') return c-'A';
                    if (c>='a'&&c<='z') return c-'a'+26;
                    if (c>='0'&&c<='9') return c-'0'+52;
                    if (c=='+') return 62; if (c=='/') return 63; return -1; };
                std::string out; out.reserve((in.size()*3)/4);
                int buf=0, bits=0;
                for (char ch : in){ if (ch=='=') break; int v=val(ch); if (v<0) continue; buf=(buf<<6)|v; bits+=6; if (bits>=8){ bits-=8; out.push_back(char((buf>>bits)&0xFF)); } }
                return out; };
            std::string state = b64dec(s64);
            if (!state.empty()) {
                // Extract ops array and apply each element
                auto pos = state.find("\"ops\"");
                if (pos != std::string::npos) {
                    auto colon = state.find(':', pos);
                    auto lb = state.find('[', colon);
                    if (lb != std::string::npos) {
                        size_t i = lb + 1; int depth = 0; bool inString=false; char esc=0;
                        while (i < state.size()) {
                            // Skip whitespace and commas between elements
                            while (i < state.size() && (state[i]==' '||state[i]=='\n'||state[i]=='\r'||state[i]=='\t'||state[i]==',')) ++i;
                            if (i >= state.size() || state[i] == ']') break;
                            size_t start = i;
                            // Determine element type
                            if (state[i] == '{' || state[i] == '[') {
                                char open = state[i]; char close = (open=='{'?'}':']');
                                depth = 1; ++i;
                                while (i < state.size() && depth > 0) {
                                    char c = state[i++];
                                    if (inString) {
                                        if (esc) { esc = 0; }
                                        else if (c == '\\') esc = 1;
                                        else if (c == '"') inString = false;
                                    } else {
                                        if (c == '"') inString = true;
                                        else if (c == open) depth++;
                                        else if (c == close) depth--;
                                    }
                                }
                                size_t end = i; // one past
                                std::string snippet = state.substr(start, end - start);
                                std::string err;
                                app->applyJsonOpsV1(snippet, err);
                            } else {
                                // Unexpected token; break
                                break;
                            }
                        }
                    }
                }
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
