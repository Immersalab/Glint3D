// Machine Summary Block
// {"file":"tools/qem_simplifier/include/glint_qem/simplify.h","purpose":"Declares the public QEM simplification API surface used by adapters and external tools.","exports":["SimplifyOptions","SimplifyResult","Simplify","SimplifyInPlace","ToString"],"depends_on":["glint_qem/types.h","<optional>","<vector>"],"notes":["stub_impl_backed_initially","determinism_policy_explicit"]}
// Human Summary
// Public API contract for QEM simplification. The current implementation is a stub used to validate UI/backend integration flow.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "glint_qem/types.h"

namespace glint_qem {

struct EpsilonPolicy {
    float area_epsilon = 1e-12f;
    float determinant_epsilon = 1e-10f;
    float position_merge_epsilon = 1e-9f;
    float normal_flip_cos_epsilon = -0.999f;
};

struct DeterminismPolicy {
    bool deterministic = true;
    std::uint64_t stable_seed = 0;
    bool stable_priority_queue_tiebreak = true;
    bool stable_edge_enumeration = true;
    bool stable_vertex_reindex_on_output = true;
};

enum class SimplifyProgressStage : std::uint8_t {
    kUnknown = 0,
    kInitializing,
    kRebuildDerivedState, // normals/quadrics/edge candidate rebuild
    kEvaluateCandidates,  // error evaluation + collapse validation
    kApplyCollapse,
    kFinalizeOutput,
    kComplete
};

struct SimplifyProgressEvent {
    SimplifyProgressStage stage = SimplifyProgressStage::kUnknown;
    std::uint32_t input_triangle_count = 0;
    std::uint32_t current_triangle_count = 0;
    std::uint32_t target_triangle_count = 0;
    bool has_target_triangle_count = false;
    std::uint32_t attempted_collapses = 0;
    std::uint32_t accepted_collapses = 0;
    std::uint32_t rejected_collapses = 0;
    double latest_edge_cost = 0.0;
    double max_edge_cost = 0.0;
    double elapsed_seconds = 0.0;
    bool is_final = false;
};

using SimplifyProgressCallback = void(*)(const SimplifyProgressEvent* event, void* user_data);

struct SimplifyProgressSink {
    SimplifyProgressCallback callback = nullptr;
    void* user_data = nullptr;
    std::uint32_t accepted_collapse_interval = 100u;
    bool emit_initial = true;
    bool emit_final = true;
};

struct SimplifyOptions {
    std::optional<std::uint32_t> target_triangle_count;
    std::optional<float> target_ratio;
    std::optional<double> max_error;
    std::optional<std::uint32_t> max_collapses;
    bool compact_output = true;
    bool emit_collapse_trace = false;
    SimplifyProgressSink progress{};
    EpsilonPolicy epsilon{};
    DeterminismPolicy determinism{};
};

struct SimplifyStats {
    std::uint32_t input_vertex_count = 0;
    std::uint32_t input_triangle_count = 0;
    std::uint32_t output_vertex_count = 0;
    std::uint32_t output_triangle_count = 0;
    std::uint32_t attempted_collapses = 0;
    std::uint32_t accepted_collapses = 0;
    std::uint32_t rejected_collapses = 0;
    double final_max_edge_error = 0.0;
    double accumulated_error = 0.0;
};

struct CollapseEvent {
    std::uint32_t step = 0;
    std::uint32_t kept_vertex = 0;
    std::uint32_t removed_vertex = 0;
    Vec3f new_position{};
    double cost = 0.0;
};

struct SimplifyResult {
    SimplifyStatus status = SimplifyStatus::kInternalError;
    SimplifyStats stats{};
    std::vector<std::uint32_t> old_to_new_vertex_map;
    std::vector<CollapseEvent> collapse_trace;
    const char* message = "";
};

const char* ToString(SimplifyStatus status);

SimplifyResult Simplify(const IndexedTriangleMesh& input,
                        const SimplifyOptions& options,
                        IndexedTriangleMesh& output);

SimplifyResult SimplifyInPlace(IndexedTriangleMesh& mesh,
                               const SimplifyOptions& options);

} // namespace glint_qem
