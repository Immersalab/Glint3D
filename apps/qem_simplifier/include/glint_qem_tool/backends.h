// Machine Summary Block
// {"file":"apps/qem_simplifier/include/glint_qem_tool/backends.h","purpose":"Declares in-process backend interfaces for simplification and Glint rendering preview orchestration.","exports":["ISimplifyBackend","IRenderBackend","SimplifyJob","RenderPreviewJob"],"depends_on":["glint_qem/simplify.h","glint_qem_tool/backend_contracts.h","<string>","<vector>"],"notes":["interface_first","plugin_abi_conscious","external_ui_shell_consumer"]}
// Human Summary
// In-process backend interfaces used by the external UI shell. They are intentionally narrow and capability-based to ease migration to a real plugin system later.

#pragma once

#include <string>
#include <vector>

#include "glint_qem/simplify.h"
#include "glint_qem_tool/backend_contracts.h"

namespace glint_qem_tool {

struct SimplifyJob {
    glint_qem::IndexedTriangleMesh input_mesh;
    glint_qem::SimplifyOptions options{};
};

struct SimplifyJobResult {
    glint_qem::IndexedTriangleMesh output_mesh;
    glint_qem::SimplifyResult simplify_result{};
};

struct RenderPreviewJob {
    std::string glint_executable = "glint";
    std::string working_directory;
    std::string ops_path;
    std::string output_image_path = "qem_preview.png";
    bool raytrace = false;
    std::vector<std::string> extra_args;
};

struct RenderPreviewResult {
    bool success = false;
    bool executed = false;
    int exit_code = 0;
    std::string built_command;
    std::string message;
};

class ISimplifyBackend {
public:
    virtual ~ISimplifyBackend() = default;
    virtual BackendInfo GetInfo() const = 0;
    virtual SimplifyJobResult Run(const SimplifyJob& job) = 0;
};

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;
    virtual BackendInfo GetInfo() const = 0;
    virtual RenderPreviewResult RenderPreview(const RenderPreviewJob& job) = 0;
};

} // namespace glint_qem_tool
