#include "application_core.h"
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdio>

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
    struct Args {
        std::vector<std::string> a;
        Args(int argc, char** argv){ for(int i=0;i<argc;++i) a.emplace_back(argv[i]); }
        bool has(const std::string& k) const { return std::find(a.begin(), a.end(), k) != a.end(); }
        std::string value(const std::string& k, const std::string& def="") const {
            for (size_t i=0;i+1<a.size();++i) if (a[i]==k) return a[i+1]; return def;
        }
        int intOr(const std::string& k, int def) const {
            std::string v = value(k, ""); if (v.empty()) return def; try { return std::stoi(v); } catch(...) { return def; }
        }
    };
    static std::string loadTextFile(const std::string& path){ std::ifstream f(path, std::ios::binary); if(!f) return {}; std::ostringstream ss; ss<<f.rdbuf(); return ss.str(); }
}

static const char* GLINT_VERSION = "0.3.0";

static void print_help()
{
    printf("glint %s\n", GLINT_VERSION);
    printf("Usage:\n");
    printf("  glint                          # Launch UI\n");
    printf("  glint --ops <file>             # Apply JSON ops headlessly\n");
    printf("  glint --ops <file> --render <out.png> [--w W --h H] [--denoise] [--raytrace]\n");
    printf("\nOptions:\n");
    printf("  --help            Show this help\n");
    printf("  --version         Print version\n");
    printf("  --ops <file>      JSON ops file to apply (v1)\n");
    printf("  --render <png>    Output PNG path for headless render\n");
    printf("  --w <int>         Output image width (default 1024)\n");
    printf("  --h <int>         Output image height (default 1024)\n");
    printf("  --denoise         Enable denoiser if available\n");
    printf("  --raytrace        Force raytracing mode for rendering\n");
}

int main(int argc, char** argv)
{
    Args args(argc, argv);
    if (args.has("--help")) { print_help(); return 0; }
    if (args.has("--version")) { printf("%s\n", GLINT_VERSION); return 0; }
    bool wantHeadless = args.has("--ops") || args.has("--render");
    int W = args.intOr("--w", 1024);
    int H = args.intOr("--h", 1024);
    bool denoiseFlag = args.has("--denoise");
    bool raytraceFlag = args.has("--raytrace");

    auto* app = new ApplicationCore();
    if (!app->init("Glint 3D", wantHeadless ? W : 800, wantHeadless ? H : 600, wantHeadless))
        return -1;
    if (denoiseFlag) app->setDenoiseEnabled(true);
    if (raytraceFlag) app->setRaytraceMode(true);

#ifdef __EMSCRIPTEN__
    // Assign global for JS bridge
    g_app = app;
    if (wantHeadless) return -1; // web build doesn't support headless here
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
    if (wantHeadless) {
        // TODO: Wire JSON Ops and headless render to ApplicationCore/RenderSystem implementations.
        // - UIBridge::applyJsonOps should apply ops to the core scene.
        // - RenderSystem::renderToPNG should render offscreen and write the image.
        // Apply ops if provided
        if (args.has("--ops")) {
            std::string ops = loadTextFile(args.value("--ops"));
            if (ops.empty()) { fprintf(stderr, "Failed to read ops file\n"); return -2; }
            std::string err;
            if (!app->applyJsonOpsV1(ops, err)) { fprintf(stderr, "Ops error: %s\n", err.c_str()); return -3; }
        }

        // Render
        if (args.has("--render")) {
            std::string out = args.value("--render");
            if (!app->renderToPNG(out, W, H)) { fprintf(stderr, "Render failed\n"); return -4; }
        }
        delete app;
        return 0;
    } else {
        app->run();
        delete app;
        return 0;
    }
#endif
}
