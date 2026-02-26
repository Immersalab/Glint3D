// Machine Summary Block
// {"file":"apps/qem_simplifier/src/backends/mesh_preview_staging.cpp","purpose":"Implements OBJ and JsonOps staging helpers used by the QEM external tool to preview meshes through Glint CLI.","exports":["glint_qem_tool::WriteObjMesh","glint_qem_tool::WritePreviewOpsForMesh"],"depends_on":["glint_qem_tool/mesh_preview_staging.h","<filesystem>","<fstream>"],"notes":["writes_ops_load_set_camera","forward_slash_paths_for_json"]}
// Human Summary
// Writes temporary preview artifacts in formats Glint already supports, so the QEM tool can reuse `glint ops ... --render ...`.

#include "glint_qem_tool/mesh_preview_staging.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace glint_qem_tool {
namespace {

std::string JsonEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string NormalizeForJsonPath(const std::filesystem::path& p) {
    return p.lexically_normal().generic_string();
}

const char* ToGlintRenderModeString(PreviewDisplayMode mode) {
    switch (mode) {
        case PreviewDisplayMode::kPoints: return "points";
        case PreviewDisplayMode::kWireframe: return "wireframe";
        case PreviewDisplayMode::kSolid: return "solid";
        case PreviewDisplayMode::kBackendDefault:
        default:
            return nullptr;
    }
}

std::string Trim(const std::string& s) {
    std::size_t first = 0;
    while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first]))) {
        ++first;
    }
    std::size_t last = s.size();
    while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1]))) {
        --last;
    }
    return s.substr(first, last - first);
}

bool ReadTextFile(const std::string& path, std::string* out, std::string* error) {
    if (out == nullptr) {
        if (error) *error = "read failed: null output buffer";
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        if (error) *error = "could not open file: " + path;
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    if (!in.good() && !in.eof()) {
        if (error) *error = "read error: " + path;
        return false;
    }
    *out = ss.str();
    return true;
}

bool ExtractOpsArrayBody(const std::string& json, std::string* body, std::string* error) {
    if (body == nullptr) {
        if (error) *error = "preset parse failed: null body pointer";
        return false;
    }

    const std::size_t key = json.find("\"ops\"");
    if (key == std::string::npos) {
        if (error) *error = "preset parse failed: missing \"ops\" key";
        return false;
    }
    const std::size_t colon = json.find(':', key + 5);
    if (colon == std::string::npos) {
        if (error) *error = "preset parse failed: malformed \"ops\" entry";
        return false;
    }

    std::size_t array_start = colon + 1;
    while (array_start < json.size() && std::isspace(static_cast<unsigned char>(json[array_start]))) {
        ++array_start;
    }
    if (array_start >= json.size() || json[array_start] != '[') {
        if (error) *error = "preset parse failed: \"ops\" is not an array";
        return false;
    }

    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    std::size_t array_end = std::string::npos;
    for (std::size_t i = array_start; i < json.size(); ++i) {
        const char c = json[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (in_string) {
            if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }
        if (c == '"') {
            in_string = true;
            continue;
        }
        if (c == '[') {
            ++depth;
            continue;
        }
        if (c == ']') {
            --depth;
            if (depth == 0) {
                array_end = i;
                break;
            }
            if (depth < 0) {
                if (error) *error = "preset parse failed: malformed array nesting";
                return false;
            }
        }
    }

    if (array_end == std::string::npos || array_end <= array_start) {
        if (error) *error = "preset parse failed: unterminated ops array";
        return false;
    }

    *body = Trim(json.substr(array_start + 1, array_end - array_start - 1));
    return true;
}

bool WritePreviewOpsDocument(const std::string& mesh_path,
                             const std::string& ops_path,
                             const PreviewSceneStagingOptions& options,
                             const std::string& extra_ops_body,
                             std::string* error) {
    if (mesh_path.empty()) {
        if (error) *error = "Ops staging failed: mesh path is empty";
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(ops_path).parent_path(), ec);

    std::ofstream out(ops_path, std::ios::binary);
    if (!out.is_open()) {
        if (error) *error = "Ops staging failed: could not open output file";
        return false;
    }

    const std::string mesh_json_path = JsonEscape(NormalizeForJsonPath(std::filesystem::path(mesh_path)));
    const std::string object_name = JsonEscape(options.object_name);

    out << "{\n";
    out << "  \"ops\": [\n";
    out << "    { \"op\": \"load\", \"path\": \"" << mesh_json_path << "\", \"name\": \"" << object_name << "\" }";

    if (const char* render_mode = ToGlintRenderModeString(options.display_mode)) {
        out << ",\n";
        out << "    { \"op\": \"set_render_mode\", \"mode\": \"" << render_mode << "\" }";
    }

    if (options.emit_camera) {
        out << ",\n";
        out << "    { \"op\": \"set_camera\", "
            << "\"position\": [" << options.camera_pos[0] << ", " << options.camera_pos[1] << ", " << options.camera_pos[2] << "], "
            << "\"target\": [" << options.camera_target[0] << ", " << options.camera_target[1] << ", " << options.camera_target[2] << "], "
            << "\"up\": [" << options.camera_up[0] << ", " << options.camera_up[1] << ", " << options.camera_up[2] << "], "
            << "\"fov_deg\": " << options.camera_fov_deg << " }";
    }

    const std::string trimmed_extra = Trim(extra_ops_body);
    if (!trimmed_extra.empty()) {
        out << ",\n";
        out << "    " << trimmed_extra;
    }
    if (options.emit_select_loaded_object) {
        out << ",\n";
        out << "    { \"op\": \"select\", \"name\": \"" << object_name << "\" }";
    }
    out << "\n";
    out << "  ]\n";
    out << "}\n";

    if (!out.good()) {
        if (error) *error = "Ops staging failed: write error";
        return false;
    }
    return true;
}

bool WriteCompareOpsDocument(const std::string& left_mesh_path,
                             const std::string& right_mesh_path,
                             const std::string& ops_path,
                             double offset_x,
                             const PreviewSceneStagingOptions& options,
                             const std::string& extra_ops_body,
                             std::string* error) {
    if (left_mesh_path.empty() || right_mesh_path.empty()) {
        if (error) *error = "Ops staging failed: compare mesh path is empty";
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(ops_path).parent_path(), ec);

    std::ofstream out(ops_path, std::ios::binary);
    if (!out.is_open()) {
        if (error) *error = "Ops staging failed: could not open output file";
        return false;
    }

    const std::string left_path = JsonEscape(NormalizeForJsonPath(std::filesystem::path(left_mesh_path)));
    const std::string right_path = JsonEscape(NormalizeForJsonPath(std::filesystem::path(right_mesh_path)));
    const std::string left_name = "qem_original";
    const std::string right_name = "qem_simplified";

    out << "{\n";
    out << "  \"ops\": [\n";
    out << "    { \"op\": \"load\", \"path\": \"" << left_path << "\", \"name\": \"" << left_name << "\" },\n";
    out << "    { \"op\": \"transform\", \"name\": \"" << left_name << "\", \"setPosition\": [" << (-offset_x) << ", 0, 0] },\n";
    out << "    { \"op\": \"load\", \"path\": \"" << right_path << "\", \"name\": \"" << right_name << "\" },\n";
    out << "    { \"op\": \"transform\", \"name\": \"" << right_name << "\", \"setPosition\": [" << (offset_x) << ", 0, 0] }";

    if (const char* render_mode = ToGlintRenderModeString(options.display_mode)) {
        out << ",\n";
        out << "    { \"op\": \"set_render_mode\", \"mode\": \"" << render_mode << "\" }";
    }

    if (options.emit_camera) {
        out << ",\n";
        out << "    { \"op\": \"set_camera\", "
            << "\"position\": [" << options.camera_pos[0] << ", " << options.camera_pos[1] << ", " << options.camera_pos[2] << "], "
            << "\"target\": [" << options.camera_target[0] << ", " << options.camera_target[1] << ", " << options.camera_target[2] << "], "
            << "\"up\": [" << options.camera_up[0] << ", " << options.camera_up[1] << ", " << options.camera_up[2] << "], "
            << "\"fov_deg\": " << options.camera_fov_deg << " }";
    }

    const std::string trimmed_extra = Trim(extra_ops_body);
    if (!trimmed_extra.empty()) {
        out << ",\n";
        out << "    " << trimmed_extra;
    }
    if (options.emit_select_loaded_object) {
        out << ",\n";
        out << "    { \"op\": \"select\", \"name\": \"" << right_name << "\" }";
    }
    out << "\n";
    out << "  ]\n";
    out << "}\n";

    if (!out.good()) {
        if (error) *error = "Ops staging failed: write error";
        return false;
    }
    return true;
}

} // namespace

bool WriteObjMesh(const glint_qem::IndexedTriangleMesh& mesh,
                  const std::string& obj_path,
                  std::string* error) {
    if ((mesh.indices.size() % 3u) != 0u) {
        if (error) *error = "OBJ staging failed: index count is not divisible by 3";
        return false;
    }

    for (std::uint32_t idx : mesh.indices) {
        if (idx >= mesh.positions.size()) {
            if (error) *error = "OBJ staging failed: index out of range";
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(obj_path).parent_path(), ec);

    std::ofstream out(obj_path, std::ios::binary);
    if (!out.is_open()) {
        if (error) *error = "OBJ staging failed: could not open output file";
        return false;
    }

    out << "# glint_qem staged preview mesh\n";
    for (const auto& p : mesh.positions) {
        out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    }
    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        out << "f "
            << (mesh.indices[i + 0] + 1u) << ' '
            << (mesh.indices[i + 1] + 1u) << ' '
            << (mesh.indices[i + 2] + 1u) << '\n';
    }

    if (!out.good()) {
        if (error) *error = "OBJ staging failed: write error";
        return false;
    }

    return true;
}

bool WritePreviewOpsForMesh(const std::string& mesh_path,
                            const std::string& ops_path,
                            const PreviewSceneStagingOptions& options,
                            std::string* error) {
    return WritePreviewOpsDocument(mesh_path, ops_path, options, "", error);
}

bool WritePreviewOpsForMeshWithPreset(const std::string& mesh_path,
                                      const std::string& preset_ops_path,
                                      const std::string& ops_path,
                                      const PreviewSceneStagingOptions& options,
                                      std::string* error) {
    if (preset_ops_path.empty()) {
        if (error) *error = "Ops staging failed: preset ops path is empty";
        return false;
    }

    std::string preset_json;
    std::string preset_error;
    if (!ReadTextFile(preset_ops_path, &preset_json, &preset_error)) {
        if (error) *error = "Ops staging failed: preset read failed: " + preset_error;
        return false;
    }

    std::string preset_ops_body;
    if (!ExtractOpsArrayBody(preset_json, &preset_ops_body, &preset_error)) {
        if (error) *error = "Ops staging failed: preset parse failed: " + preset_error;
        return false;
    }

    return WritePreviewOpsDocument(mesh_path, ops_path, options, preset_ops_body, error);
}

bool WriteCompareOpsForMeshes(const std::string& left_mesh_path,
                              const std::string& right_mesh_path,
                              const std::string& ops_path,
                              double offset_x,
                              const PreviewSceneStagingOptions& options,
                              std::string* error) {
    return WriteCompareOpsDocument(left_mesh_path, right_mesh_path, ops_path, offset_x, options, "", error);
}

bool WriteCompareOpsForMeshesWithPreset(const std::string& left_mesh_path,
                                        const std::string& right_mesh_path,
                                        const std::string& preset_ops_path,
                                        const std::string& ops_path,
                                        double offset_x,
                                        const PreviewSceneStagingOptions& options,
                                        std::string* error) {
    if (preset_ops_path.empty()) {
        if (error) *error = "Ops staging failed: preset ops path is empty";
        return false;
    }

    std::string preset_json;
    std::string preset_error;
    if (!ReadTextFile(preset_ops_path, &preset_json, &preset_error)) {
        if (error) *error = "Ops staging failed: preset read failed: " + preset_error;
        return false;
    }

    std::string preset_ops_body;
    if (!ExtractOpsArrayBody(preset_json, &preset_ops_body, &preset_error)) {
        if (error) *error = "Ops staging failed: preset parse failed: " + preset_error;
        return false;
    }

    return WriteCompareOpsDocument(left_mesh_path, right_mesh_path, ops_path, offset_x, options, preset_ops_body, error);
}

} // namespace glint_qem_tool
