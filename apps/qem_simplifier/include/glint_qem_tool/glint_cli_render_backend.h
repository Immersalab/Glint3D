// Machine Summary Block
// {"file":"apps/qem_simplifier/include/glint_qem_tool/glint_cli_render_backend.h","purpose":"Declares a render backend that bridges preview requests to the existing Glint CLI via command construction and execution.","exports":["GlintCliRenderBackend"],"depends_on":["glint_qem_tool/backends.h"],"notes":["always_execute","std_system_execution"]}
// Human Summary
// CLI-based render backend for early integration testing. It executes Glint preview commands directly.

#pragma once

#include "glint_qem_tool/backends.h"

namespace glint_qem_tool {

class GlintCliRenderBackend final : public IRenderBackend {
public:
    BackendInfo GetInfo() const override;
    RenderPreviewResult RenderPreview(const RenderPreviewJob& job) override;
};

} // namespace glint_qem_tool
