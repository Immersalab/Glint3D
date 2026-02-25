// Machine Summary Block
// {"file":"tools/qem_simplifier/src/backends/qem_core_simplifier_backend.cpp","purpose":"Implements the in-process simplifier backend by forwarding jobs to the qem_core stub API.","exports":["glint_qem_tool::QemCoreSimplifierBackend"],"depends_on":["glint_qem_tool/qem_core_simplifier_backend.h"],"notes":["backend_interface_smoke_path"]}
// Human Summary
// Simple backend adapter that wraps the qem_core API so the shell UI can target a stable interface.

#include "glint_qem_tool/qem_core_simplifier_backend.h"

namespace glint_qem_tool {

BackendInfo QemCoreSimplifierBackend::GetInfo() const {
    BackendInfo info;
    info.kind = BackendKind::kSimplifier;
    info.capability_flags = kCapabilityDeterministic | kCapabilityCollapseTrace;
    info.backend_id = "qem_core_baseline";
    info.display_name = "QEM Core Baseline Simplifier";
    info.implementation_version = "0.2.0-baseline";
    return info;
}

SimplifyJobResult QemCoreSimplifierBackend::Run(const SimplifyJob& job) {
    SimplifyJobResult out;
    out.simplify_result = glint_qem::Simplify(job.input_mesh, job.options, out.output_mesh);
    return out;
}

} // namespace glint_qem_tool
