// Machine Summary Block
// {"file":"tools/qem_simplifier/src/backends/glint_cli_render_backend.cpp","purpose":"Implements a CLI render backend that builds and executes Glint render commands for preview testing.","exports":["glint_qem_tool::GlintCliRenderBackend"],"depends_on":["glint_qem_tool/glint_cli_render_backend.h","<cstdlib>"],"notes":["legacy_ops_cli_bridge","always_execute","abi_conscious_request_shape"]}
// Human Summary
// Early render bridge that targets the existing Glint CLI. It executes preview requests directly.

#include "glint_qem_tool/glint_cli_render_backend.h"

#include <cstdlib>
#include <sstream>

namespace glint_qem_tool {
namespace {

std::string QuoteArg(const std::string& arg) {
    if (arg.find_first_of(" \t\"") == std::string::npos) {
        return arg;
    }
    std::string quoted = "\"";
    for (char c : arg) {
        if (c == '\"') {
            quoted += "\\\"";
        } else {
            quoted += c;
        }
    }
    quoted += "\"";
    return quoted;
}

std::string BuildCommand(const RenderPreviewJob& job) {
    std::ostringstream cmd;
    cmd << QuoteArg(job.glint_executable);
    if (!job.ops_path.empty()) {
        // Prefer the explicit `glint ops` command path over legacy top-level flags.
        cmd << " ops " << QuoteArg(job.ops_path);
        if (!job.output_image_path.empty()) {
            cmd << " --render " << QuoteArg(job.output_image_path);
        }
    } else if (!job.output_image_path.empty()) {
        // Fallback shape for other future render job styles.
        cmd << " --render " << QuoteArg(job.output_image_path);
    }
    if (job.raytrace) {
        cmd << " --raytrace";
    }
    for (const std::string& extra : job.extra_args) {
        cmd << " " << extra;
    }
    return cmd.str();
}

} // namespace

BackendInfo GlintCliRenderBackend::GetInfo() const {
    BackendInfo info;
    info.kind = BackendKind::kRenderer;
    info.capability_flags = kCapabilityExternalProcess | kCapabilityRenderPreview;
    info.backend_id = "glint_cli_render";
    info.display_name = "Glint CLI Render Bridge";
    info.implementation_version = "0.1.0-scaffold";
    return info;
}

RenderPreviewResult GlintCliRenderBackend::RenderPreview(const RenderPreviewJob& job) {
    RenderPreviewResult result;
    result.built_command = BuildCommand(job);

    if (job.ops_path.empty()) {
        result.success = false;
        result.message = "render request missing ops_path";
        return result;
    }

    const int code = std::system(result.built_command.c_str());
    result.executed = true;
    result.exit_code = code;
    result.success = (code == 0);
    result.message = result.success ? "glint CLI render completed" : "glint CLI render failed";
    return result;
}

} // namespace glint_qem_tool
