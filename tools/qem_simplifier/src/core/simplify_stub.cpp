// Machine Summary Block
// {"file":"tools/qem_simplifier/src/core/simplify_stub.cpp","purpose":"Provides a stub qem_core implementation used to validate external UI/backend integration before real QEM math is implemented.","exports":["glint_qem::Simplify","glint_qem::SimplifyInPlace","glint_qem::ToString"],"depends_on":["glint_qem/simplify.h"],"notes":["pass_through_output","basic_input_validation","deterministic_stub_trace"]}
// Human Summary
// Pass-through stub implementation for the QEM core API. It validates mesh input and returns deterministic placeholder results.

#include "glint_qem/simplify.h"

#include <utility>

namespace glint_qem {
namespace {

bool IsValidTriangleMesh(const IndexedTriangleMesh& mesh) {

    if ((mesh.indices.size() % 3u) != 0u) 
    {
        return false;
    }

    const std::uint32_t vertex_count = static_cast<std::uint32_t>(mesh.positions.size());
    for (std::uint32_t index : mesh.indices) {
        if (index >= vertex_count) {
            return false;
        }
    }
    return true;
}

void FillStats(const IndexedTriangleMesh& input,
               const IndexedTriangleMesh& output,
               SimplifyResult& result) {
    result.stats.input_vertex_count = static_cast<std::uint32_t>(input.positions.size());
    result.stats.input_triangle_count = static_cast<std::uint32_t>(input.indices.size() / 3u);
    result.stats.output_vertex_count = static_cast<std::uint32_t>(output.positions.size());
    result.stats.output_triangle_count = static_cast<std::uint32_t>(output.indices.size() / 3u);
}

} // namespace

const char* ToString(SimplifyStatus status) {
    switch (status) {
        case SimplifyStatus::kSuccess: return "success";
        case SimplifyStatus::kInvalidInput: return "invalid_input";
        case SimplifyStatus::kNoReductionPossible: return "no_reduction_possible";
        case SimplifyStatus::kStoppedByTarget: return "stopped_by_target";
        case SimplifyStatus::kStoppedByErrorLimit: return "stopped_by_error_limit";
        case SimplifyStatus::kStoppedByMaxCollapses: return "stopped_by_max_collapses";
        case SimplifyStatus::kInternalError: return "internal_error";
    }
    return "unknown";
}

SimplifyResult Simplify(const IndexedTriangleMesh& input,
                        const SimplifyOptions& options,
                        IndexedTriangleMesh& output) {
    SimplifyResult result;
    output = input;

    if (!IsValidTriangleMesh(input)) 
    {
        result.status = SimplifyStatus::kInvalidInput;
        result.message = "stub: invalid indexed triangle mesh";
        FillStats(input, output, result);
        return result;
    }

    FillStats(input, output, result);
    result.status = SimplifyStatus::kSuccess;
    result.message = "stub: no simplification performed";

    if (options.emit_collapse_trace && input.positions.size() >= 2u) {
        const Vec3f& a = input.positions[0];
        const Vec3f& b = input.positions[1];
        CollapseEvent evt;
        evt.step = 0;
        evt.kept_vertex = 0;
        evt.removed_vertex = 1;
        evt.new_position = Vec3f{(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f};
        evt.cost = 0.0;
        result.collapse_trace.push_back(evt);
    }

    if (options.compact_output) {
        result.old_to_new_vertex_map.resize(input.positions.size());
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(result.old_to_new_vertex_map.size()); ++i) {
            result.old_to_new_vertex_map[i] = i;
        }
    }

    if (options.target_triangle_count.has_value()) {
        const std::uint32_t current_triangles = static_cast<std::uint32_t>(input.indices.size() / 3u);
        if (current_triangles <= *options.target_triangle_count) {
            result.status = SimplifyStatus::kStoppedByTarget;
            result.message = "stub: mesh already satisfies target triangle count";
        }
    }

    return result;
}

SimplifyResult SimplifyInPlace(IndexedTriangleMesh& mesh,
                               const SimplifyOptions& options) {
    IndexedTriangleMesh output;
    SimplifyResult result = Simplify(mesh, options, output);
    if (result.status != SimplifyStatus::kInvalidInput && result.status != SimplifyStatus::kInternalError) {
        mesh = std::move(output);
    }
    return result;
}

} // namespace glint_qem
