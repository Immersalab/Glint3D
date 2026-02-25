// Machine Summary Block
// {"file":"tools/qem_simplifier/include/glint_qem/types.h","purpose":"Defines core QEM mesh/value types and status codes independent of Glint and UI/render backends.","exports":["Vec3f","IndexedTriangleMesh","SimplifyStatus"],"depends_on":["<cstdint>","<vector>"],"notes":["stl_only","plugin_boundary_avoids_glm"]}
// Human Summary
// Minimal core types for the QEM simplifier API. These are intentionally independent from Glint mesh/render types.

#pragma once

#include <cstdint>
#include <vector>

namespace glint_qem {

struct Vec3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct IndexedTriangleMesh {
    std::vector<Vec3f> positions;
    std::vector<std::uint32_t> indices; // triangle list: 3 * triangle_count
};

enum class SimplifyStatus : std::uint32_t {
    kSuccess = 0,
    kInvalidInput,
    kNoReductionPossible,
    kStoppedByTarget,
    kStoppedByErrorLimit,
    kStoppedByMaxCollapses,
    kInternalError
};

} // namespace glint_qem
