// Machine Summary Block
// {"file":"apps/qem_simplifier/include/glint_qem_tool/qem_core_simplifier_backend.h","purpose":"Declares the in-process simplifier backend that wraps the stub qem_core API.","exports":["QemCoreSimplifierBackend"],"depends_on":["glint_qem_tool/backends.h"],"notes":["pass_through_stub_initially"]}
// Human Summary
// Thin backend wrapper around the current qem_core stub implementation. Keeps the shell/UI dependent on an interface rather than a concrete core call.

#pragma once

#include "glint_qem_tool/backends.h"

namespace glint_qem_tool {

class QemCoreSimplifierBackend final : public ISimplifyBackend {
public:
    BackendInfo GetInfo() const override;
    SimplifyJobResult Run(const SimplifyJob& job) override;
};

} // namespace glint_qem_tool
