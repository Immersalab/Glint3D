// Machine Summary Block
// {"file":"tools/qem_simplifier/src/ui/qem_studio_shell.cpp","purpose":"Provides a minimal external shell UI to test backend interfaces, stub qem_core calls, and the Glint CLI render bridge.","exports":["main"],"depends_on":["glint_qem_tool/qem_core_simplifier_backend.h","glint_qem_tool/glint_cli_render_backend.h"],"notes":["repl_style_shell","always_execute_preview","vertical_slice_test_harness"]}
// Human Summary
// Minimal console UI shell for exercising the QEM backend interfaces before a real GUI exists.

#include "glint_qem_tool/glint_cli_render_backend.h"
#include "glint_qem_tool/mesh_preview_staging.h"
#include "glint_qem_tool/obj_mesh_io.h"
#include "glint_qem_tool/qem_core_simplifier_backend.h"

#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>

namespace {

std::vector<std::string> SplitWords(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> out;
    std::string token;
    while (iss >> token) {
        out.push_back(token);
    }
    return out;
}

std::string Trim(const std::string& s) {
    const std::size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

glint_qem::IndexedTriangleMesh MakeSampleMesh();

struct ShellConfig {
    std::string glint_executable = "glint";
    std::string mesh_path;
    std::string output_dir = "tools/qem_simplifier/output";
    std::string preview_preset_ops = "tools/qem_simplifier/output/preview_preset.ops.json";
    std::string preview_display_mode = "solid";
};

bool IsValidPreviewDisplayMode(const std::string& value) {
    return value == "default" || value == "solid" || value == "wireframe" || value == "points" ||
           value == "wireframe_overlay";
}

glint_qem_tool::PreviewDisplayMode ParsePreviewDisplayMode(const std::string& value) {
    if (value == "wireframe") return glint_qem_tool::PreviewDisplayMode::kWireframe;
    if (value == "points") return glint_qem_tool::PreviewDisplayMode::kPoints;
    if (value == "solid") return glint_qem_tool::PreviewDisplayMode::kSolid;
    return glint_qem_tool::PreviewDisplayMode::kBackendDefault;
}

bool UsesSelectionWireframeOverlay(const std::string& value) {
    return value == "wireframe_overlay";
}

bool LoadShellConfigFile(const std::string& path, ShellConfig* cfg, std::string* error) {
    if (cfg == nullptr) {
        if (error) *error = "config load failed: null config pointer";
        return false;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        if (error) *error = "config load failed: could not open file";
        return false;
    }

    ShellConfig loaded = *cfg;
    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        const std::string t = Trim(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }
        const std::size_t eq = t.find('=');
        if (eq == std::string::npos) {
            if (error) *error = "config load failed: invalid line " + std::to_string(line_no);
            return false;
        }
        const std::string key = Trim(t.substr(0, eq));
        const std::string value = Trim(t.substr(eq + 1));
        if (key == "glint_executable") {
            loaded.glint_executable = value;
        } else if (key == "mesh_path") {
            loaded.mesh_path = value;
        } else if (key == "output_dir") {
            loaded.output_dir = value;
        } else if (key == "preview_preset_ops") {
            loaded.preview_preset_ops = value;
        } else if (key == "preview_display_mode") {
            if (!IsValidPreviewDisplayMode(value)) {
                if (error) *error = "config load failed: invalid preview_display_mode '" + value + "'";
                return false;
            }
            loaded.preview_display_mode = value;
        } else {
            if (error) *error = "config load failed: unknown key '" + key + "'";
            return false;
        }
    }

    if (loaded.output_dir.empty()) {
        if (error) *error = "config load failed: output_dir cannot be empty";
        return false;
    }

    *cfg = loaded;
    return true;
}

bool SaveShellConfigFile(const std::string& path, const ShellConfig& cfg, std::string* error) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path p(path);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path(), ec);
        if (ec) {
            if (error) *error = "config save failed: could not create parent directory";
            return false;
        }
    }

    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        if (error) *error = "config save failed: could not open file";
        return false;
    }

    out << "# glint_qem_studio_shell config v1\n";
    out << "glint_executable=" << cfg.glint_executable << "\n";
    out << "mesh_path=" << cfg.mesh_path << "\n";
    out << "output_dir=" << cfg.output_dir << "\n";
    out << "preview_preset_ops=" << cfg.preview_preset_ops << "\n";
    out << "preview_display_mode=" << cfg.preview_display_mode << "\n";
    if (!out.good()) {
        if (error) *error = "config save failed: write error";
        return false;
    }
    return true;
}

std::string BuildPreviewImagePath(const std::string& output_dir) {
    return (std::filesystem::path(output_dir) / "qem_preview.png").string();
}

std::string BuildSimplifiedObjPath(const std::string& output_dir) {
    return (std::filesystem::path(output_dir) / "staging" / "qem_simplified.obj").string();
}

std::string BuildCompareOpsPath(const std::string& output_dir) {
    return (std::filesystem::path(output_dir) / "staging" / "compare.ops.json").string();
}

void ClearConsoleScreen() {
#if defined(_WIN32)
    std::system("cls");
#else
    std::system("clear");
#endif
}

double ComputeTargetProgressPercent(const glint_qem::SimplifyProgressEvent& event) {
    if (!event.has_target_triangle_count) {
        return -1.0;
    }
    const std::uint32_t input_tris = event.input_triangle_count;
    const std::uint32_t target_tris = event.target_triangle_count;
    const std::uint32_t current_tris = event.current_triangle_count;
    if (input_tris <= target_tris) {
        return 100.0;
    }
    const double total = static_cast<double>(input_tris - target_tris);
    const double done = static_cast<double>(input_tris - std::max(current_tris, target_tris));
    const double pct = (total > 0.0) ? (100.0 * done / total) : 100.0;
    return std::max(0.0, std::min(100.0, pct));
}

class ConsoleProgressLogger {
public:
    explicit ConsoleProgressLogger(std::uint32_t interval = 100u)
        : interval_(std::max<std::uint32_t>(1u, interval)) {}

    glint_qem::SimplifyProgressSink MakeSink() {
        glint_qem::SimplifyProgressSink sink{};
        sink.callback = &ConsoleProgressLogger::OnProgressThunk;
        sink.user_data = this;
        sink.accepted_collapse_interval = interval_;
        sink.emit_initial = true;
        sink.emit_final = true;
        return sink;
    }

private:
    static void OnProgressThunk(const glint_qem::SimplifyProgressEvent* event, void* user_data) {
        if (event == nullptr || user_data == nullptr) {
            return;
        }
        static_cast<ConsoleProgressLogger*>(user_data)->OnProgress(*event);
    }

    void OnProgress(const glint_qem::SimplifyProgressEvent& event) {
        const std::uint32_t input_tris = std::max<std::uint32_t>(1u, event.input_triangle_count);
        const double tri_ratio = static_cast<double>(event.current_triangle_count) / static_cast<double>(input_tris);

        double pct = ComputeTargetProgressPercent(event);
        if (pct < 0.0) {
            pct = std::max(0.0, std::min(100.0, (1.0 - tri_ratio) * 100.0));
        }

        constexpr int kBarWidth = 30;
        const int filled = std::max(0, std::min(kBarWidth, static_cast<int>((pct / 100.0) * kBarWidth + 0.5)));

        std::ostringstream oss;
        oss << "[qem] [";
        for (int i = 0; i < kBarWidth; ++i) {
            oss << (i < filled ? '#' : '-');
        }
        oss << "] "
            << std::setw(3) << static_cast<int>(pct + 0.5) << "% "
            << "tris "
            << event.current_triangle_count << "/" << event.input_triangle_count
            << " (" << std::fixed << std::setprecision(3) << tri_ratio << ") "
            << "t=" << std::fixed << std::setprecision(2) << event.elapsed_seconds << "s";

        const std::string line = oss.str();
        const std::size_t pad = (last_line_len_ > line.size()) ? (last_line_len_ - line.size()) : 0u;

        std::cout << '\r' << line;
        if (pad > 0u) {
            std::cout << std::string(pad, ' ');
        }
        if (event.is_final) {
            std::cout << "\n";
            last_line_len_ = 0u;
        } else {
            std::cout << std::flush;
            last_line_len_ = line.size();
        }
    }

    std::uint32_t interval_ = 100u;
    std::size_t last_line_len_ = 0u;
};

bool LoadSimplifyInputMesh(const ShellConfig& config,
                           glint_qem::IndexedTriangleMesh& mesh,
                           std::string* source_label,
                           std::string* error) {
    mesh = {};

    if (config.mesh_path.empty()) {
        mesh = MakeSampleMesh();
        if (source_label) *source_label = "<sample mesh>";
        return true;
    }

    const std::filesystem::path mesh_path(config.mesh_path);
    std::string ext = mesh_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (ext != ".obj") {
        if (error) *error = "only OBJ is supported for qem input right now";
        return false;
    }

    std::string load_error;
    if (!glint_qem_tool::ReadObjMesh(config.mesh_path, mesh, &load_error)) {
        if (error) *error = load_error;
        return false;
    }

    if (source_label) *source_label = config.mesh_path;
    return true;
}

bool ParseSimplifyPercentArg(const std::vector<std::string>& args, float* out_ratio, std::string* error) {
    if (out_ratio == nullptr) {
        if (error) *error = "internal error: null out_ratio";
        return false;
    }

    *out_ratio = 0.5f; // default: keep 50%
    if (args.size() <= 1) {
        return true;
    }
    if (args.size() != 2) {
        if (error) *error = "usage: simplify [--percent]";
        return false;
    }

    const std::string& token = args[1];
    if (token.size() < 3 || token.rfind("--", 0) != 0) {
        if (error) *error = "usage: simplify [--percent] (example: simplify --50)";
        return false;
    }

    const std::string number_text = token.substr(2);
    for (char c : number_text) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            if (error) *error = "invalid simplify percent: " + token + " (use integer 1..100, e.g. --50)";
            return false;
        }
    }

    int percent = 0;
    try {
        percent = std::stoi(number_text);
    } catch (...) {
        if (error) *error = "invalid simplify percent: " + token;
        return false;
    }

    if (percent < 1 || percent > 100) {
        if (error) *error = "simplify percent out of range: use 1..100";
        return false;
    }

    *out_ratio = static_cast<float>(percent) / 100.0f;
    return true;
}

void PrintSimplifySummary(const glint_qem::SimplifyResult& r,
                         const std::string& mesh_source) {
    std::cout << "simplify status: " << glint_qem::ToString(r.status) << "\n";
    std::cout << "message: " << r.message << "\n";
    std::cout << "triangles: " << r.stats.input_triangle_count << " -> " << r.stats.output_triangle_count << "\n";
    std::cout << "vertices: " << r.stats.input_vertex_count << " -> " << r.stats.output_vertex_count << "\n";
    std::cout << "collapse_trace events: " << r.collapse_trace.size() << "\n";
    std::cout << "mesh source: " << mesh_source << "\n";
}

std::string QuoteCmdArg(const std::string& arg) {
    if (arg.find_first_of(" \t\"") == std::string::npos) {
        return arg;
    }
    std::string quoted = "\"";
    for (char c : arg) {
        if (c == '"') {
            quoted += "\\\"";
        } else {
            quoted += c;
        }
    }
    quoted += "\"";
    return quoted;
}

std::string BuildGlintUiOpsCommand(const std::string& glint_executable,
                                   const std::string& ops_path) {
    return QuoteCmdArg(glint_executable) + " ui --ops " + QuoteCmdArg(ops_path);
}

struct StagePreviewArtifactsResult {
    bool success = false;
    bool used_sample_mesh = false;
    bool used_preset = false;
    std::string mesh_for_ops;
    std::string staged_obj_path;
    std::string staged_ops_path;
    std::string preset_path_used;
    std::string error;
};

struct StageCompareArtifactsResult {
    bool success = false;
    bool staged_original_sample_obj = false;
    bool used_preset = false;
    std::string left_mesh_path;
    std::string right_mesh_path;
    std::string staged_original_obj_path;
    std::string compare_ops_path;
    std::string preset_path_used;
    double offset_x = 0.0;
    std::string error;
};

struct MeshBounds {
    bool valid = false;
    float min_x = 0.0f, min_y = 0.0f, min_z = 0.0f;
    float max_x = 0.0f, max_y = 0.0f, max_z = 0.0f;
};

MeshBounds ComputeBounds(const glint_qem::IndexedTriangleMesh& mesh) {
    MeshBounds b{};
    if (mesh.positions.empty()) return b;
    b.valid = true;
    b.min_x = b.max_x = mesh.positions[0].x;
    b.min_y = b.max_y = mesh.positions[0].y;
    b.min_z = b.max_z = mesh.positions[0].z;
    for (const auto& p : mesh.positions) {
        b.min_x = std::min(b.min_x, p.x); b.max_x = std::max(b.max_x, p.x);
        b.min_y = std::min(b.min_y, p.y); b.max_y = std::max(b.max_y, p.y);
        b.min_z = std::min(b.min_z, p.z); b.max_z = std::max(b.max_z, p.z);
    }
    return b;
}

double ComputeCompareOffsetX(const glint_qem::IndexedTriangleMesh& left,
                             const glint_qem::IndexedTriangleMesh& right) {
    const MeshBounds a = ComputeBounds(left);
    const MeshBounds b = ComputeBounds(right);
    const double ax = a.valid ? static_cast<double>(a.max_x - a.min_x) : 0.0;
    const double ay = a.valid ? static_cast<double>(a.max_y - a.min_y) : 0.0;
    const double az = a.valid ? static_cast<double>(a.max_z - a.min_z) : 0.0;
    const double bx = b.valid ? static_cast<double>(b.max_x - b.min_x) : 0.0;
    const double by = b.valid ? static_cast<double>(b.max_y - b.min_y) : 0.0;
    const double bz = b.valid ? static_cast<double>(b.max_z - b.min_z) : 0.0;
    const double max_dim = std::max(0.001, std::max({ax, ay, az, bx, by, bz}));
    const double min_separation = 0.5 * (ax + bx) + 0.15 * max_dim;
    const double offset = 0.5 * min_separation;
    return std::max(0.5 * max_dim, offset);
}

StagePreviewArtifactsResult StagePreviewArtifacts(const ShellConfig& config) {
    namespace fs = std::filesystem;
    StagePreviewArtifactsResult out;

    const fs::path stage_dir = fs::path(config.output_dir) / "staging";
    std::error_code ec;
    fs::create_directories(stage_dir, ec);
    if (ec) {
        out.error = "failed to create staging directory: " + ec.message();
        return out;
    }

    if (config.mesh_path.empty()) {
        out.staged_obj_path = (stage_dir / "sample_preview_mesh.obj").string();
        std::string stage_error;
        if (!glint_qem_tool::WriteObjMesh(MakeSampleMesh(), out.staged_obj_path, &stage_error)) {
            out.error = "mesh staging failed: " + stage_error;
            return out;
        }
        out.mesh_for_ops = out.staged_obj_path;
        out.used_sample_mesh = true;
    } else {
        out.mesh_for_ops = config.mesh_path;
        out.used_sample_mesh = false;
    }

    out.staged_ops_path = (stage_dir / "preview_mesh.ops.json").string();

    const bool has_preset_path = !config.preview_preset_ops.empty();
    const bool preset_exists = has_preset_path && fs::exists(fs::path(config.preview_preset_ops));
    glint_qem_tool::PreviewSceneStagingOptions opts{};
    opts.display_mode = ParsePreviewDisplayMode(config.preview_display_mode);
    opts.emit_select_loaded_object = UsesSelectionWireframeOverlay(config.preview_display_mode);
    if (preset_exists) {
        opts.emit_camera = false; // preset should provide camera/lights
        std::string ops_error;
        if (!glint_qem_tool::WritePreviewOpsForMeshWithPreset(out.mesh_for_ops,
                                                              config.preview_preset_ops,
                                                              out.staged_ops_path,
                                                              opts,
                                                              &ops_error)) {
            out.error = "ops staging failed: " + ops_error;
            return out;
        }
        out.used_preset = true;
        out.preset_path_used = config.preview_preset_ops;
    } else {
        std::string ops_error;
        if (!glint_qem_tool::WritePreviewOpsForMesh(out.mesh_for_ops, out.staged_ops_path, opts, &ops_error)) {
            out.error = "ops staging failed: " + ops_error;
            return out;
        }
        out.used_preset = false;
        if (has_preset_path) {
            out.preset_path_used = config.preview_preset_ops;
        }
    }

    out.success = true;
    return out;
}

StageCompareArtifactsResult StageCompareArtifacts(const ShellConfig& config,
                                                  const std::string& simplified_obj_path) {
    namespace fs = std::filesystem;
    StageCompareArtifactsResult out;

    if (simplified_obj_path.empty()) {
        out.error = "no simplified obj available; run simplify first";
        return out;
    }
    if (!fs::exists(simplified_obj_path)) {
        out.error = "simplified obj not found: " + simplified_obj_path;
        return out;
    }

    const fs::path stage_dir = fs::path(config.output_dir) / "staging";
    std::error_code ec;
    fs::create_directories(stage_dir, ec);
    if (ec) {
        out.error = "failed to create staging directory: " + ec.message();
        return out;
    }

    glint_qem::IndexedTriangleMesh left_mesh;
    if (config.mesh_path.empty()) {
        left_mesh = MakeSampleMesh();
        out.staged_original_obj_path = (stage_dir / "sample_compare_original.obj").string();
        std::string write_error;
        if (!glint_qem_tool::WriteObjMesh(left_mesh, out.staged_original_obj_path, &write_error)) {
            out.error = "compare staging failed: " + write_error;
            return out;
        }
        out.left_mesh_path = out.staged_original_obj_path;
        out.staged_original_sample_obj = true;
    } else {
        std::string load_error;
        if (!glint_qem_tool::ReadObjMesh(config.mesh_path, left_mesh, &load_error)) {
            out.error = "compare staging failed: could not load source mesh: " + load_error;
            return out;
        }
        out.left_mesh_path = config.mesh_path;
    }

    glint_qem::IndexedTriangleMesh right_mesh;
    {
        std::string load_error;
        if (!glint_qem_tool::ReadObjMesh(simplified_obj_path, right_mesh, &load_error)) {
            out.error = "compare staging failed: could not load simplified mesh: " + load_error;
            return out;
        }
    }
    out.right_mesh_path = simplified_obj_path;
    out.offset_x = ComputeCompareOffsetX(left_mesh, right_mesh);
    out.compare_ops_path = BuildCompareOpsPath(config.output_dir);

    glint_qem_tool::PreviewSceneStagingOptions opts{};
    opts.display_mode = ParsePreviewDisplayMode(config.preview_display_mode);
    opts.emit_select_loaded_object = UsesSelectionWireframeOverlay(config.preview_display_mode);
    opts.camera_pos[0] = 10.0; opts.camera_pos[1] = 6.0; opts.camera_pos[2] = 10.0;
    opts.camera_target[0] = 0.0; opts.camera_target[1] = 0.0; opts.camera_target[2] = 0.0;

    const bool has_preset_path = !config.preview_preset_ops.empty();
    const bool preset_exists = has_preset_path && fs::exists(fs::path(config.preview_preset_ops));
    if (preset_exists) {
        opts.emit_camera = false;
        std::string ops_error;
        if (!glint_qem_tool::WriteCompareOpsForMeshesWithPreset(out.left_mesh_path,
                                                                out.right_mesh_path,
                                                                config.preview_preset_ops,
                                                                out.compare_ops_path,
                                                                out.offset_x,
                                                                opts,
                                                                &ops_error)) {
            out.error = "compare ops staging failed: " + ops_error;
            return out;
        }
        out.used_preset = true;
        out.preset_path_used = config.preview_preset_ops;
    } else {
        std::string ops_error;
        if (!glint_qem_tool::WriteCompareOpsForMeshes(out.left_mesh_path,
                                                      out.right_mesh_path,
                                                      out.compare_ops_path,
                                                      out.offset_x,
                                                      opts,
                                                      &ops_error)) {
            out.error = "compare ops staging failed: " + ops_error;
            return out;
        }
        if (has_preset_path) {
            out.preset_path_used = config.preview_preset_ops;
        }
    }

    out.success = true;
    return out;
}

glint_qem::IndexedTriangleMesh MakeSampleMesh() {
    glint_qem::IndexedTriangleMesh mesh;
    mesh.positions = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
    };
    mesh.indices = {
        0, 1, 2,
        0, 1, 3,
        0, 2, 3,
        1, 2, 3
    };
    return mesh;
}

void PrintHelp() {
    std::cout
        << "\n"
        << "============================================================\n"
        << "  Welcome to...\n"
        << "    _____ _      _____ _   _ _______ ____  _____\n"
        << "   / ____| |    |_   _| \\ | |__   __|___ \\|  __ \\\n"
        << "  | |  __| |      | | |  \\| |  | |    __) | |  | |\n"
        << "  | | |_ | |      | | | . ` |  | |   |__ <| |  | |\n"
        << "  | |__| | |____ _| |_| |\\  |  | |   ___) | |__| |\n"
        << "   \\_____|______|_____|_| \\_|  |_|  |____/|_____/\n"
        << "\n"
        << "                     QEM Studio\n"
        << "============================================================\n"
        << "Workflow\n"
        << "  1) simplify [--50]        Simplify configured OBJ and save staged output\n"
        << "  2) renderqem              Render saved simplified mesh to image\n"
        << "  3) openqem                Open saved simplified mesh in Glint UI\n"
        << "  4) savecmp/opencmp        Build/open side-by-side original vs simplified compare view\n"
        << "\n"
        << "Core Commands\n"
        << "  help                      Show this menu\n"
        << "  ?                         Alias for help\n"
        << "  clear | cls               Clear the shell screen\n"
        << "  status | st               Show current shell state\n"
        << "  quit | exit               Exit shell\n"
        << "\n"
        << "Configuration\n"
        << "  config | cfg show         Show config path + values\n"
        << "  config | cfg load [path]  Load config file (key=value)\n"
        << "  config | cfg save [path]  Save config file (key=value)\n"
        << "  set glint <path>          Set renderer executable path\n"
        << "  set outdir <path>         Output folder (image + staging files)\n"
        << "  set mesh <path>           Source mesh path (OBJ for QEM input)\n"
        << "  set preset <path>         Preview preset JsonOps (camera/lights)\n"
        << "  set display <mode>        default|solid|wireframe|points|wireframe_overlay\n"
        << "\n"
        << "QEM + Preview\n"
        << "  simplify [--N]            Run QEM and save <outdir>/staging/qem_simplified.obj (keep N%% tris)\n"
        << "  render                    Stage current mesh + ops, then render\n"
        << "  renderqem                 Stage saved simplified mesh + ops, then render\n"
        << "  open                      Stage current mesh + ops, then launch Glint UI\n"
        << "  openqem                   Stage saved simplified mesh + ops, then launch Glint UI\n"
        << "  savecmp                   Save compare JsonOps (original + simplified side by side)\n"
        << "  rendercmp                 Render compare view (original + simplified)\n"
        << "  opencmp                   Open compare view in Glint UI\n"
        << "============================================================\n";
}

} // namespace

int main(int argc, char** argv) {
    glint_qem_tool::QemCoreSimplifierBackend simplify_backend;
    glint_qem_tool::GlintCliRenderBackend render_backend;

    ShellConfig config;
    std::string config_path = "tools/qem_simplifier/qem_shell.config";
    bool startup_clear_help = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--config") {
            if (i + 1 >= argc) {
                std::cout << "missing value after --config\n";
                return 2;
            }
            config_path = argv[++i];
            continue;
        }
        if (arg == "--startup-clear-help") {
            startup_clear_help = true;
            continue;
        }
        std::cout << "unknown argument: " << arg << "\n";
        return 2;
    }

    {
        std::string config_error;
        if (std::filesystem::exists(config_path) && !LoadShellConfigFile(config_path, &config, &config_error)) {
            std::cout << "warning: " << config_error << " (" << config_path << ")\n";
        }
    }

    std::string staged_obj_path;
    std::string staged_ops_path;
    std::string last_simplified_obj_path;
    std::string last_compare_ops_path;

    std::cout << "glint_qem_studio_shell (scaffold)\n";
    std::cout << "External UI shell for backend-interface testing. Type 'help'.\n";
    if (startup_clear_help) {
        ClearConsoleScreen();
        PrintHelp();
    }

    for (;;) {
        std::cout << "> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        const std::vector<std::string> args = SplitWords(line);
        if (args.empty()) {
            continue;
        }

        const std::string& cmd = args[0];
        if (cmd == "quit" || cmd == "exit") {
            break;
        }
        if (cmd == "help" || cmd == "?") {
            PrintHelp();
            continue;
        }
        if (cmd == "clear" || cmd == "cls") {
            ClearConsoleScreen();
            continue;
        }
        if (cmd == "status" || cmd == "st") {
            const auto s_info = simplify_backend.GetInfo();
            const auto r_info = render_backend.GetInfo();
            const bool preset_exists =
                (!config.preview_preset_ops.empty() && std::filesystem::exists(config.preview_preset_ops));

            std::cout
                << "\n"
                << "============================================================\n"
                << "  QEM STUDIO STATUS\n"
                << "============================================================\n"
                << "Session\n"
                << "  config_path      : " << config_path << "\n"
                << "\n"
                << "Renderer\n"
                << "  glint_executable : " << config.glint_executable << "\n"
                << "  render_backend   : " << r_info.display_name << " (" << r_info.backend_id << ")\n"
                << "\n"
                << "QEM\n"
                << "  simplify_backend : " << s_info.display_name << " (" << s_info.backend_id << ")\n"
                << "  source_mesh      : " << (config.mesh_path.empty() ? "<sample mesh>" : config.mesh_path) << "\n"
                << "  simplified_obj   : " << (last_simplified_obj_path.empty() ? "<none>" : last_simplified_obj_path) << "\n"
                << "\n"
                << "Preview\n"
                << "  output_dir       : " << config.output_dir << "\n"
                << "  preview_image    : " << BuildPreviewImagePath(config.output_dir) << "\n"
                << "  preset_ops       : " << (config.preview_preset_ops.empty() ? "<unset>" : config.preview_preset_ops) << "\n"
                << "  preset_exists    : " << (preset_exists ? "yes" : "no") << "\n"
                << "  display_mode     : " << config.preview_display_mode << "\n"
                << "  staged_obj       : " << (staged_obj_path.empty() ? "<none>" : staged_obj_path) << "\n"
                << "  staged_ops       : " << (staged_ops_path.empty() ? "<none>" : staged_ops_path) << "\n"
                << "  compare_ops      : " << (last_compare_ops_path.empty() ? "<none>" : last_compare_ops_path) << "\n"
                << "============================================================\n";
            continue;
        }

        if (cmd == "config" || cmd == "cfg") {
            if (args.size() < 2) {
                std::cout << "usage: config show|load [path]|save [path]\n";
                continue;
            }
            const std::string sub = args[1];
            if (sub == "show") {
                std::cout << "config_path=" << config_path << "\n";
                std::cout << "glint_executable=" << config.glint_executable << "\n";
                std::cout << "mesh_path=" << config.mesh_path << "\n";
                std::cout << "output_dir=" << config.output_dir << "\n";
                std::cout << "preview_preset_ops=" << config.preview_preset_ops << "\n";
                std::cout << "preview_display_mode=" << config.preview_display_mode << "\n";
                continue;
            }
            std::string path = config_path;
            if (args.size() >= 3) {
                const std::size_t pos = line.find(sub);
                const std::size_t start = (pos == std::string::npos) ? std::string::npos : line.find_first_not_of(" \t", pos + sub.size());
                if (start == std::string::npos) {
                    std::cout << "missing path\n";
                    continue;
                }
                path = line.substr(start);
            }
            if (sub == "load") {
                std::string error;
                if (!LoadShellConfigFile(path, &config, &error)) {
                    std::cout << error << "\n";
                    continue;
                }
                config_path = path;
                std::cout << "ok\n";
                continue;
            }
            if (sub == "save") {
                std::string error;
                if (!SaveShellConfigFile(path, config, &error)) {
                    std::cout << error << "\n";
                    continue;
                }
                config_path = path;
                std::cout << "ok\n";
                continue;
            }
            std::cout << "unknown config command: " << sub << "\n";
            continue;
        }

        if (cmd == "set") {
            if (args.size() < 3) {
                std::cout << "usage: set glint|outdir|mesh|preset|display <value>\n";
                continue;
            }
            const std::string key = args[1];
            const std::size_t pos = line.find(key);
            const std::size_t start = (pos == std::string::npos) ? std::string::npos : line.find_first_not_of(" \t", pos + key.size());
            if (start == std::string::npos) {
                std::cout << "missing value\n";
                continue;
            }
            const std::string value = line.substr(start);
            if (key == "glint") {
                config.glint_executable = value;
            } else if (key == "outdir") {
                config.output_dir = value;
            } else if (key == "mesh") {
                config.mesh_path = value;
            } else if (key == "preset") {
                config.preview_preset_ops = value;
            } else if (key == "display") {
                if (!IsValidPreviewDisplayMode(value)) {
                    std::cout << "invalid display mode: " << value
                              << " (use default|solid|wireframe|points|wireframe_overlay)\n";
                    continue;
                }
                config.preview_display_mode = value;
            } else {
                std::cout << "unknown key: " << key << "\n";
                continue;
            }
            std::cout << "ok\n";
            continue;
        }

        if (cmd == "simplify" || cmd == "simplifymesh") {
            glint_qem_tool::SimplifyJob job;
            std::string mesh_source;
            std::string load_error;
            if (!LoadSimplifyInputMesh(config, job.input_mesh, &mesh_source, &load_error)) {
                std::cout << "simplify load failed: " << load_error << "\n";
                continue;
            }

            float target_ratio = 0.5f;
            std::string arg_error;
            if (!ParseSimplifyPercentArg(args, &target_ratio, &arg_error)) {
                std::cout << arg_error << "\n";
                continue;
            }

            job.options.target_ratio = target_ratio;
            job.options.emit_collapse_trace = true;
            job.options.compact_output = true;
            ConsoleProgressLogger progress_logger(100u);
            job.options.progress = progress_logger.MakeSink();

            glint_qem_tool::SimplifyJobResult job_result = simplify_backend.Run(job);
            const glint_qem::SimplifyResult& r = job_result.simplify_result;
            PrintSimplifySummary(r, mesh_source);

            if (r.status == glint_qem::SimplifyStatus::kInvalidInput ||
                r.status == glint_qem::SimplifyStatus::kInternalError) {
                std::cout << "simplify save skipped (simplify failed)\n";
                continue;
            }

            const std::string out_obj = BuildSimplifiedObjPath(config.output_dir);
            std::string save_error;
            if (!glint_qem_tool::WriteObjMesh(job_result.output_mesh, out_obj, &save_error)) {
                std::cout << "simplify save failed: " << save_error << "\n";
                continue;
            }

            last_simplified_obj_path = out_obj;
            std::cout << "saved simplified obj: " << last_simplified_obj_path << "\n";
            continue;
        }

        if (cmd == "renderqem" || cmd == "previewsimplified") {
            if (last_simplified_obj_path.empty()) {
                std::cout << "no simplified obj available; run simplify first\n";
                continue;
            }
            if (!std::filesystem::exists(last_simplified_obj_path)) {
                std::cout << "simplified obj not found: " << last_simplified_obj_path << "\n";
                continue;
            }

            ShellConfig preview_cfg = config;
            preview_cfg.mesh_path = last_simplified_obj_path;
            StagePreviewArtifactsResult stage = StagePreviewArtifacts(preview_cfg);
            if (!stage.success) {
                std::cout << stage.error << "\n";
                continue;
            }
            staged_obj_path = stage.staged_obj_path;
            staged_ops_path = stage.staged_ops_path;

            glint_qem_tool::RenderPreviewJob job;
            job.glint_executable = config.glint_executable;
            job.ops_path = staged_ops_path;
            job.output_image_path = BuildPreviewImagePath(config.output_dir);
            job.raytrace = false;
            if (UsesSelectionWireframeOverlay(config.preview_display_mode)) {
                job.extra_args.push_back("--selection-overlay");
            }
            glint_qem_tool::RenderPreviewResult r = render_backend.RenderPreview(job);
            std::cout << "simplified mesh: " << last_simplified_obj_path << "\n";
            std::cout << "staged ops: " << staged_ops_path << "\n";
            std::cout << "renderqem status: " << (r.success ? "success" : "failure") << "\n";
            std::cout << "executed: " << (r.executed ? "yes" : "no") << "\n";
            std::cout << "exit_code: " << r.exit_code << "\n";
            std::cout << "command: " << r.built_command << "\n";
            std::cout << "message: " << r.message << "\n";
            continue;
        }

        if (cmd == "savecmp") {
            StageCompareArtifactsResult cmp = StageCompareArtifacts(config, last_simplified_obj_path);
            if (!cmp.success) {
                std::cout << cmp.error << "\n";
                continue;
            }
            last_compare_ops_path = cmp.compare_ops_path;
            std::cout << "compare ops: " << cmp.compare_ops_path << "\n";
            std::cout << "left mesh: " << cmp.left_mesh_path << "\n";
            std::cout << "right mesh: " << cmp.right_mesh_path << "\n";
            std::cout << "offset_x: " << cmp.offset_x << "\n";
            if (cmp.used_preset) {
                std::cout << "preset: " << cmp.preset_path_used << "\n";
            } else if (!cmp.preset_path_used.empty()) {
                std::cout << "preset: " << cmp.preset_path_used << " (not found; using default camera)\n";
            }
            continue;
        }

        if (cmd == "rendercmp") {
            StageCompareArtifactsResult cmp = StageCompareArtifacts(config, last_simplified_obj_path);
            if (!cmp.success) {
                std::cout << cmp.error << "\n";
                continue;
            }
            last_compare_ops_path = cmp.compare_ops_path;

            glint_qem_tool::RenderPreviewJob job;
            job.glint_executable = config.glint_executable;
            job.ops_path = cmp.compare_ops_path;
            job.output_image_path = BuildPreviewImagePath(config.output_dir);
            job.raytrace = false;
            if (UsesSelectionWireframeOverlay(config.preview_display_mode)) {
                job.extra_args.push_back("--selection-overlay");
            }
            glint_qem_tool::RenderPreviewResult r = render_backend.RenderPreview(job);
            std::cout << "compare ops: " << cmp.compare_ops_path << "\n";
            std::cout << "left mesh: " << cmp.left_mesh_path << "\n";
            std::cout << "right mesh: " << cmp.right_mesh_path << "\n";
            std::cout << "offset_x: " << cmp.offset_x << "\n";
            std::cout << "rendercmp status: " << (r.success ? "success" : "failure") << "\n";
            std::cout << "executed: " << (r.executed ? "yes" : "no") << "\n";
            std::cout << "exit_code: " << r.exit_code << "\n";
            std::cout << "command: " << r.built_command << "\n";
            std::cout << "message: " << r.message << "\n";
            continue;
        }

        if (cmd == "opencmp") {
            StageCompareArtifactsResult cmp = StageCompareArtifacts(config, last_simplified_obj_path);
            if (!cmp.success) {
                std::cout << cmp.error << "\n";
                continue;
            }
            last_compare_ops_path = cmp.compare_ops_path;

            const std::string command = BuildGlintUiOpsCommand(config.glint_executable, cmp.compare_ops_path);
            const int code = std::system(command.c_str());
            std::cout << "compare ops: " << cmp.compare_ops_path << "\n";
            std::cout << "left mesh: " << cmp.left_mesh_path << "\n";
            std::cout << "right mesh: " << cmp.right_mesh_path << "\n";
            std::cout << "offset_x: " << cmp.offset_x << "\n";
            std::cout << "opencmp exit_code: " << code << "\n";
            std::cout << "command: " << command << "\n";
            continue;
        }

        if (cmd == "openqem" || cmd == "previewuisimplified") {
            if (last_simplified_obj_path.empty()) {
                std::cout << "no simplified obj available; run simplify first\n";
                continue;
            }
            if (!std::filesystem::exists(last_simplified_obj_path)) {
                std::cout << "simplified obj not found: " << last_simplified_obj_path << "\n";
                continue;
            }

            ShellConfig preview_cfg = config;
            preview_cfg.mesh_path = last_simplified_obj_path;
            StagePreviewArtifactsResult stage = StagePreviewArtifacts(preview_cfg);
            if (!stage.success) {
                std::cout << stage.error << "\n";
                continue;
            }
            staged_obj_path = stage.staged_obj_path;
            staged_ops_path = stage.staged_ops_path;

            const std::string command = BuildGlintUiOpsCommand(config.glint_executable, staged_ops_path);
            const int code = std::system(command.c_str());

            std::cout << "simplified mesh: " << last_simplified_obj_path << "\n";
            std::cout << "staged ops: " << staged_ops_path << "\n";
            std::cout << "openqem exit_code: " << code << "\n";
            std::cout << "command: " << command << "\n";
            continue;
        }

        if (cmd == "render" || cmd == "previewmesh") {
            StagePreviewArtifactsResult stage = StagePreviewArtifacts(config);
            if (!stage.success) {
                std::cout << stage.error << "\n";
                continue;
            }
            staged_obj_path = stage.staged_obj_path;
            staged_ops_path = stage.staged_ops_path;

            glint_qem_tool::RenderPreviewJob job;
            job.glint_executable = config.glint_executable;
            job.ops_path = staged_ops_path;
            job.output_image_path = BuildPreviewImagePath(config.output_dir);
            job.raytrace = false;
            if (UsesSelectionWireframeOverlay(config.preview_display_mode)) {
                job.extra_args.push_back("--selection-overlay");
            }
            glint_qem_tool::RenderPreviewResult r = render_backend.RenderPreview(job);
            std::cout << "staged ops: " << staged_ops_path << "\n";
            if (!staged_obj_path.empty()) {
                std::cout << "staged mesh: " << staged_obj_path << "\n";
            } else {
                std::cout << "mesh path: " << stage.mesh_for_ops << "\n";
            }
            if (stage.used_preset) {
                std::cout << "preset: " << stage.preset_path_used << "\n";
            } else if (!stage.preset_path_used.empty()) {
                std::cout << "preset: " << stage.preset_path_used << " (not found; using default camera)\n";
            }
            std::cout << "render status: " << (r.success ? "success" : "failure") << "\n";
            std::cout << "executed: " << (r.executed ? "yes" : "no") << "\n";
            std::cout << "exit_code: " << r.exit_code << "\n";
            std::cout << "command: " << r.built_command << "\n";
            std::cout << "message: " << r.message << "\n";
            continue;
        }

        if (cmd == "open" || cmd == "previewui") {
            StagePreviewArtifactsResult stage = StagePreviewArtifacts(config);
            if (!stage.success) {
                std::cout << stage.error << "\n";
                continue;
            }
            staged_obj_path = stage.staged_obj_path;
            staged_ops_path = stage.staged_ops_path;

            const std::string command = BuildGlintUiOpsCommand(config.glint_executable, staged_ops_path);
            const int code = std::system(command.c_str());

            std::cout << "staged ops: " << staged_ops_path << "\n";
            if (!staged_obj_path.empty()) {
                std::cout << "staged mesh: " << staged_obj_path << "\n";
            } else {
                std::cout << "mesh path: " << stage.mesh_for_ops << "\n";
            }
            if (stage.used_preset) {
                std::cout << "preset: " << stage.preset_path_used << "\n";
            } else if (!stage.preset_path_used.empty()) {
                std::cout << "preset: " << stage.preset_path_used << " (not found; using default camera)\n";
            }
            std::cout << "open exit_code: " << code << "\n";
            std::cout << "command: " << command << "\n";
            continue;
        }

        std::cout << "unknown command: " << cmd << " (type 'help')\n";
    }

    return 0;
}
