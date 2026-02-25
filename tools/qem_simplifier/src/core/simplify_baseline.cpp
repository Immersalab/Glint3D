// Machine Summary Block
// {"file":"tools/qem_simplifier/src/core/simplify_baseline.cpp","purpose":"Implements a baseline deterministic QEM edge-collapse simplifier behind the qem_core public API.","exports":["glint_qem::Simplify","glint_qem::SimplifyInPlace","glint_qem::ToString"],"depends_on":["glint_qem/simplify.h"],"notes":["stl_only","deterministic_qem_baseline","correctness_first_global_rebuild_each_accepted_collapse"]}
// Human Summary
// Baseline Quadric Error Metric (QEM) simplifier implementation. Deterministic and headless, with a correctness-first update strategy that rebuilds derived topology/candidates after each accepted collapse.

#include "glint_qem/simplify.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>

namespace glint_qem {
namespace {

struct Vec3d {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Quadric {
    // Compact symmetric 4x4 storage:
    // [00,01,02,03,11,12,13,22,23,33]
    double m[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
};

struct VertexRec {
    Vec3d p{};
    Quadric q{};
    bool alive = true;
};

struct FaceRec {
    std::uint32_t v[3] = {0u, 0u, 0u};
    bool alive = true;
    Vec3d normal{};
    double area = 0.0;
};

struct EdgeKey {
    std::uint32_t a = 0u;
    std::uint32_t b = 0u;

    bool operator<(const EdgeKey& rhs) const {
        if (a != rhs.a) return a < rhs.a;
        return b < rhs.b;
    }
};

struct TriangleKey {
    std::uint32_t a = 0u;
    std::uint32_t b = 0u;
    std::uint32_t c = 0u;

    bool operator<(const TriangleKey& rhs) const {
        if (a != rhs.a) return a < rhs.a;
        if (b != rhs.b) return b < rhs.b;
        return c < rhs.c;
    }
};

struct EdgeCandidate {
    EdgeKey key{};
    Vec3d optimal_pos{};
    double cost = std::numeric_limits<double>::infinity();
    std::uint64_t tie_id = 0u;
};

struct CandidateWorse {
    bool operator()(const EdgeCandidate& lhs, const EdgeCandidate& rhs) const {
        if (lhs.cost != rhs.cost) return lhs.cost > rhs.cost;
        if (lhs.key.a != rhs.key.a) return lhs.key.a > rhs.key.a;
        if (lhs.key.b != rhs.key.b) return lhs.key.b > rhs.key.b;
        return lhs.tie_id > rhs.tie_id;
    }
};

using CandidateQueue = std::priority_queue<EdgeCandidate, std::vector<EdgeCandidate>, CandidateWorse>;

struct DerivedState {
    std::vector<std::vector<std::uint32_t>> vertex_faces;
    std::map<EdgeKey, std::uint32_t> edge_incidence;
    CandidateQueue candidates;
    std::uint32_t alive_face_count = 0u;
    std::uint32_t alive_vertex_count = 0u;
};

struct CollapseDecision {
    std::uint32_t keep = 0u;
    std::uint32_t remove = 0u;
    Vec3d new_pos{};
    double cost = 0.0;
};

struct StopReason {
    enum Kind {
        kNone = 0,
        kTargetReached,
        kErrorLimit,
        kMaxCollapses,
        kNoMoreCandidates
    } kind = kNone;
};

Vec3d ToVec3d(const Vec3f& v) {
    return Vec3d{static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z)};
}

Vec3f ToVec3f(const Vec3d& v) {
    return Vec3f{static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)};
}

Vec3d operator+(const Vec3d& a, const Vec3d& b) {
    return Vec3d{a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3d operator-(const Vec3d& a, const Vec3d& b) {
    return Vec3d{a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3d operator*(const Vec3d& v, double s) {
    return Vec3d{v.x * s, v.y * s, v.z * s};
}

double Dot(const Vec3d& a, const Vec3d& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3d Cross(const Vec3d& a, const Vec3d& b) {
    return Vec3d{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

double Length(const Vec3d& v) {
    return std::sqrt(Dot(v, v));
}

double DistanceSquared(const Vec3d& a, const Vec3d& b) {
    const Vec3d d = a - b;
    return Dot(d, d);
}

EdgeKey MakeEdgeKey(std::uint32_t i, std::uint32_t j) {
    return (i < j) ? EdgeKey{i, j} : EdgeKey{j, i};
}

TriangleKey MakeTriangleKey(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    std::array<std::uint32_t, 3> ids = {a, b, c};
    std::sort(ids.begin(), ids.end());
    return TriangleKey{ids[0], ids[1], ids[2]};
}

bool HasRepeatedIndices(const std::uint32_t v[3]) {
    return v[0] == v[1] || v[1] == v[2] || v[0] == v[2];
}

void ZeroQuadric(Quadric& q) {
    for (double& x : q.m) {
        x = 0.0;
    }
}

Quadric AddQuadrics(const Quadric& a, const Quadric& b) {
    Quadric out;
    for (int i = 0; i < 10; ++i) {
        out.m[i] = a.m[i] + b.m[i];
    }
    return out;
}

void AddQuadricInPlace(Quadric& dst, const Quadric& src) {
    for (int i = 0; i < 10; ++i) {
        dst.m[i] += src.m[i];
    }
}

Quadric BuildPlaneQuadric(const Vec3d& n, double d, double weight) {
    Quadric q;
    const double a = n.x;
    const double b = n.y;
    const double c = n.z;
    const double w = weight;
    q.m[0] = w * a * a;
    q.m[1] = w * a * b;
    q.m[2] = w * a * c;
    q.m[3] = w * a * d;
    q.m[4] = w * b * b;
    q.m[5] = w * b * c;
    q.m[6] = w * b * d;
    q.m[7] = w * c * c;
    q.m[8] = w * c * d;
    q.m[9] = w * d * d;
    return q;
}

double EvaluateQuadricCost(const Quadric& q, const Vec3d& p) {
    const double x = p.x;
    const double y = p.y;
    const double z = p.z;
    const double value =
        q.m[0] * x * x +
        2.0 * q.m[1] * x * y +
        2.0 * q.m[2] * x * z +
        2.0 * q.m[3] * x +
        q.m[4] * y * y +
        2.0 * q.m[5] * y * z +
        2.0 * q.m[6] * y +
        q.m[7] * z * z +
        2.0 * q.m[8] * z +
        q.m[9];

    if (!std::isfinite(value)) return std::numeric_limits<double>::infinity();
    if (value < 0.0 && value > -1e-12) return 0.0;
    return value;
}

bool SolveOptimalPosition3x3(const Quadric& q, double det_epsilon, Vec3d& out) {
    const double a00 = q.m[0];
    const double a01 = q.m[1];
    const double a02 = q.m[2];
    const double a10 = q.m[1];
    const double a11 = q.m[4];
    const double a12 = q.m[5];
    const double a20 = q.m[2];
    const double a21 = q.m[5];
    const double a22 = q.m[7];

    const double b0 = -q.m[3];
    const double b1 = -q.m[6];
    const double b2 = -q.m[8];

    const double det =
        a00 * (a11 * a22 - a12 * a21) -
        a01 * (a10 * a22 - a12 * a20) +
        a02 * (a10 * a21 - a11 * a20);

    if (!std::isfinite(det) || std::abs(det) <= det_epsilon) {
        return false;
    }

    const double det_x =
        b0 * (a11 * a22 - a12 * a21) -
        a01 * (b1 * a22 - a12 * b2) +
        a02 * (b1 * a21 - a11 * b2);

    const double det_y =
        a00 * (b1 * a22 - a12 * b2) -
        b0 * (a10 * a22 - a12 * a20) +
        a02 * (a10 * b2 - b1 * a20);

    const double det_z =
        a00 * (a11 * b2 - b1 * a21) -
        a01 * (a10 * b2 - b1 * a20) +
        b0 * (a10 * a21 - a11 * a20);

    const double inv_det = 1.0 / det;
    out = Vec3d{det_x * inv_det, det_y * inv_det, det_z * inv_det};
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

EdgeCandidate BuildEdgeCandidate(const std::vector<VertexRec>& vertices,
                                 const EdgeKey& key,
                                 std::uint64_t tie_id,
                                 const EpsilonPolicy& eps) {
    EdgeCandidate c;
    c.key = key;
    c.tie_id = tie_id;

    const Quadric q = AddQuadrics(vertices[key.a].q, vertices[key.b].q);
    Vec3d pos{};
    const double det_eps = std::max(0.0, static_cast<double>(eps.determinant_epsilon));
    if (SolveOptimalPosition3x3(q, det_eps, pos)) {
        c.optimal_pos = pos;
        c.cost = EvaluateQuadricCost(q, pos);
        return c;
    }

    const Vec3d p0 = vertices[key.a].p;
    const Vec3d p1 = vertices[key.b].p;
    const Vec3d pm = (p0 + p1) * 0.5;
    const double c0 = EvaluateQuadricCost(q, p0);
    const double c1 = EvaluateQuadricCost(q, p1);
    const double c2 = EvaluateQuadricCost(q, pm);

    c.optimal_pos = p0;
    c.cost = c0;
    if (c1 < c.cost) {
        c.optimal_pos = p1;
        c.cost = c1;
    }
    if (c2 < c.cost) {
        c.optimal_pos = pm;
        c.cost = c2;
    }
    return c;
}

bool IsValidTriangleMesh(const IndexedTriangleMesh& mesh) {
    if ((mesh.indices.size() % 3u) != 0u) {
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

void BuildWorkingMesh(const IndexedTriangleMesh& input,
                      std::vector<VertexRec>& vertices,
                      std::vector<FaceRec>& faces) {
    vertices.clear();
    faces.clear();
    vertices.reserve(input.positions.size());
    faces.reserve(input.indices.size() / 3u);

    for (const Vec3f& p : input.positions) {
        VertexRec v;
        v.p = ToVec3d(p);
        v.alive = true;
        ZeroQuadric(v.q);
        vertices.push_back(v);
    }

    for (std::size_t i = 0; i + 2 < input.indices.size(); i += 3) {
        FaceRec f;
        f.v[0] = input.indices[i + 0];
        f.v[1] = input.indices[i + 1];
        f.v[2] = input.indices[i + 2];
        f.alive = true;
        faces.push_back(f);
    }
}

bool ComputeFaceGeometry(FaceRec& face,
                         const std::vector<VertexRec>& vertices,
                         double area_epsilon) {
    if (!face.alive) return false;
    if (HasRepeatedIndices(face.v)) {
        face.alive = false;
        return false;
    }
    if (!vertices[face.v[0]].alive || !vertices[face.v[1]].alive || !vertices[face.v[2]].alive) {
        face.alive = false;
        return false;
    }

    const Vec3d& p0 = vertices[face.v[0]].p;
    const Vec3d& p1 = vertices[face.v[1]].p;
    const Vec3d& p2 = vertices[face.v[2]].p;
    const Vec3d cross = Cross(p1 - p0, p2 - p0);
    const double cross_len = Length(cross);
    const double area = 0.5 * cross_len;
    if (!std::isfinite(area) || area <= area_epsilon) {
        face.alive = false;
        face.area = 0.0;
        face.normal = Vec3d{};
        return false;
    }

    face.area = area;
    face.normal = cross * (1.0 / cross_len);
    return true;
}

DerivedState RebuildDerivedState(std::vector<VertexRec>& vertices,
                                 std::vector<FaceRec>& faces,
                                 const SimplifyOptions& options) {
    DerivedState state;
    state.vertex_faces.assign(vertices.size(), {});

    for (VertexRec& v : vertices) {
        ZeroQuadric(v.q);
        if (v.alive) {
            ++state.alive_vertex_count;
        }
    }

    const double area_eps = std::max(0.0, static_cast<double>(options.epsilon.area_epsilon));
    for (std::uint32_t face_id = 0u; face_id < static_cast<std::uint32_t>(faces.size()); ++face_id) {
        FaceRec& face = faces[face_id];
        if (!face.alive) continue;
        if (!ComputeFaceGeometry(face, vertices, area_eps)) continue;

        const double d = -Dot(face.normal, vertices[face.v[0]].p);
        const Quadric fq = BuildPlaneQuadric(face.normal, d, face.area);
        AddQuadricInPlace(vertices[face.v[0]].q, fq);
        AddQuadricInPlace(vertices[face.v[1]].q, fq);
        AddQuadricInPlace(vertices[face.v[2]].q, fq);

        state.vertex_faces[face.v[0]].push_back(face_id);
        state.vertex_faces[face.v[1]].push_back(face_id);
        state.vertex_faces[face.v[2]].push_back(face_id);

        ++state.alive_face_count;
        ++state.edge_incidence[MakeEdgeKey(face.v[0], face.v[1])];
        ++state.edge_incidence[MakeEdgeKey(face.v[1], face.v[2])];
        ++state.edge_incidence[MakeEdgeKey(face.v[2], face.v[0])];
    }

    std::uint64_t tie = 0u;
    for (const auto& kv : state.edge_incidence) {
        const EdgeKey& key = kv.first;
        if (!vertices[key.a].alive || !vertices[key.b].alive) continue;
        state.candidates.push(BuildEdgeCandidate(vertices, key, tie++, options.epsilon));
    }

    return state;
}

bool EdgeExistsAndManifoldOkay(const DerivedState& state, const EdgeKey& key) {
    const auto it = state.edge_incidence.find(key);
    if (it == state.edge_incidence.end()) return false;
    return it->second >= 1u && it->second <= 2u;
}

bool ValidateCollapse(const CollapseDecision& d,
                      const std::vector<VertexRec>& vertices,
                      const std::vector<FaceRec>& faces,
                      const DerivedState& state,
                      const SimplifyOptions& options) {
    if (d.keep == d.remove) return false;
    if (d.keep >= vertices.size() || d.remove >= vertices.size()) return false;
    if (!vertices[d.keep].alive || !vertices[d.remove].alive) return false;
    if (!EdgeExistsAndManifoldOkay(state, MakeEdgeKey(d.keep, d.remove))) return false;

    const double area_eps = std::max(0.0, static_cast<double>(options.epsilon.area_epsilon));
    const double flip_cos = static_cast<double>(options.epsilon.normal_flip_cos_epsilon);
    const double pos_eps = std::max(0.0, static_cast<double>(options.epsilon.position_merge_epsilon));
    const double pos_eps_sq = pos_eps * pos_eps;

    std::set<std::uint32_t> affected_faces;
    for (std::uint32_t face_id : state.vertex_faces[d.keep]) affected_faces.insert(face_id);
    for (std::uint32_t face_id : state.vertex_faces[d.remove]) affected_faces.insert(face_id);
    if (affected_faces.empty()) return false;

    std::set<TriangleKey> local_keys;
    std::set<std::uint32_t> local_neighbors;

    for (std::uint32_t face_id : affected_faces) {
        if (face_id >= faces.size()) return false;
        const FaceRec& face = faces[face_id];
        if (!face.alive) continue;

        std::uint32_t nv[3] = {face.v[0], face.v[1], face.v[2]};
        bool touched = false;
        bool contains_keep = false;
        for (int i = 0; i < 3; ++i) {
            if (nv[i] == d.remove) {
                nv[i] = d.keep;
                touched = true;
            }
            if (nv[i] == d.keep) {
                contains_keep = true;
            } else if (nv[i] != d.remove) {
                local_neighbors.insert(nv[i]);
            }
        }
        if (!touched && !contains_keep) continue;
        if (HasRepeatedIndices(nv)) continue; // triangle is removed by collapse

        const Vec3d p0 = (nv[0] == d.keep) ? d.new_pos : vertices[nv[0]].p;
        const Vec3d p1 = (nv[1] == d.keep) ? d.new_pos : vertices[nv[1]].p;
        const Vec3d p2 = (nv[2] == d.keep) ? d.new_pos : vertices[nv[2]].p;
        const Vec3d cross = Cross(p1 - p0, p2 - p0);
        const double cross_len = Length(cross);
        const double area = 0.5 * cross_len;
        if (!std::isfinite(area) || area <= area_eps) {
            return false;
        }

        const Vec3d new_normal = cross * (1.0 / cross_len);
        if (face.area > area_eps && Dot(face.normal, new_normal) < flip_cos) {
            return false;
        }

        const TriangleKey tk = MakeTriangleKey(nv[0], nv[1], nv[2]);
        if (!local_keys.insert(tk).second) {
            return false;
        }
    }

    if (pos_eps_sq > 0.0) {
        for (std::uint32_t n : local_neighbors) {
            if (n >= vertices.size()) continue;
            if (!vertices[n].alive) continue;
            if (DistanceSquared(d.new_pos, vertices[n].p) <= pos_eps_sq) {
                return false;
            }
        }
    }

    return true;
}

void ApplyCollapse(const CollapseDecision& d,
                   std::vector<VertexRec>& vertices,
                   std::vector<FaceRec>& faces) {
    vertices[d.keep].p = d.new_pos;
    vertices[d.remove].alive = false;

    for (FaceRec& face : faces) {
        if (!face.alive) continue;
        bool touched = false;
        for (int i = 0; i < 3; ++i) {
            if (face.v[i] == d.remove) {
                face.v[i] = d.keep;
                touched = true;
            }
        }
        if (touched && HasRepeatedIndices(face.v)) {
            face.alive = false;
        }
    }
}

std::uint32_t CountReferencedAliveVertices(const std::vector<VertexRec>& vertices,
                                           const std::vector<FaceRec>& faces) {
    std::vector<bool> referenced(vertices.size(), false);
    for (const FaceRec& face : faces) {
        if (!face.alive) continue;
        referenced[face.v[0]] = true;
        referenced[face.v[1]] = true;
        referenced[face.v[2]] = true;
    }

    std::uint32_t count = 0u;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        if (vertices[i].alive && referenced[i]) {
            ++count;
        }
    }
    return count;
}

void BuildOutputMesh(const std::vector<VertexRec>& vertices,
                     const std::vector<FaceRec>& faces,
                     const SimplifyOptions& options,
                     IndexedTriangleMesh& output,
                     SimplifyResult& result) {
    output.positions.clear();
    output.indices.clear();
    result.old_to_new_vertex_map.clear();

    if (options.compact_output) {
        result.old_to_new_vertex_map.assign(vertices.size(), std::numeric_limits<std::uint32_t>::max());

        std::vector<bool> referenced(vertices.size(), false);
        for (const FaceRec& face : faces) {
            if (!face.alive) continue;
            referenced[face.v[0]] = true;
            referenced[face.v[1]] = true;
            referenced[face.v[2]] = true;
        }

        output.positions.reserve(vertices.size());
        for (std::uint32_t i = 0u; i < static_cast<std::uint32_t>(vertices.size()); ++i) {
            if (!vertices[i].alive || !referenced[i]) continue;
            result.old_to_new_vertex_map[i] = static_cast<std::uint32_t>(output.positions.size());
            output.positions.push_back(ToVec3f(vertices[i].p));
        }

        output.indices.reserve(faces.size() * 3u);
        for (const FaceRec& face : faces) {
            if (!face.alive) continue;
            output.indices.push_back(result.old_to_new_vertex_map[face.v[0]]);
            output.indices.push_back(result.old_to_new_vertex_map[face.v[1]]);
            output.indices.push_back(result.old_to_new_vertex_map[face.v[2]]);
        }
        return;
    }

    output.positions.reserve(vertices.size());
    for (const VertexRec& v : vertices) {
        output.positions.push_back(ToVec3f(v.p));
    }

    output.indices.reserve(faces.size() * 3u);
    for (const FaceRec& face : faces) {
        if (!face.alive) continue;
        output.indices.push_back(face.v[0]);
        output.indices.push_back(face.v[1]);
        output.indices.push_back(face.v[2]);
    }
}

std::optional<std::uint32_t> ResolveTargetTriangleCount(std::uint32_t input_triangles,
                                                        const SimplifyOptions& options) {
    std::optional<std::uint32_t> target = options.target_triangle_count;
    if (options.target_ratio.has_value()) {
        float ratio = *options.target_ratio;
        if (!std::isfinite(ratio)) ratio = 1.0f;
        ratio = std::max(0.0f, std::min(1.0f, ratio));
        const double scaled = static_cast<double>(input_triangles) * static_cast<double>(ratio);
        const std::uint32_t ratio_target = static_cast<std::uint32_t>(std::floor(scaled + 1e-12));
        if (target.has_value()) {
            target = std::min(*target, ratio_target);
        } else {
            target = ratio_target;
        }
    }
    return target;
}

void FillTerminalStats(const IndexedTriangleMesh& input,
                       const std::vector<VertexRec>& vertices,
                       const std::vector<FaceRec>& faces,
                       const IndexedTriangleMesh& output,
                       SimplifyResult& result) {
    result.stats.input_vertex_count = static_cast<std::uint32_t>(input.positions.size());
    result.stats.input_triangle_count = static_cast<std::uint32_t>(input.indices.size() / 3u);
    result.stats.output_triangle_count = static_cast<std::uint32_t>(output.indices.size() / 3u);
    result.stats.output_vertex_count = CountReferencedAliveVertices(vertices, faces);
}

CollapseDecision MakeDecisionFromCandidate(const EdgeCandidate& c) {
    CollapseDecision d;
    d.keep = c.key.a;   // deterministic: keep lower ID
    d.remove = c.key.b; // deterministic: remove higher ID
    d.new_pos = c.optimal_pos;
    d.cost = c.cost;
    return d;
}

void EmitProgressEvent(const SimplifyOptions& options,
                       const SimplifyResult& result,
                       std::uint32_t input_triangles,
                       std::uint32_t current_triangles,
                       const std::optional<std::uint32_t>& target_triangles,
                       const std::chrono::steady_clock::time_point& started,
                       double latest_edge_cost,
                       bool final_event) {
    if (options.progress.callback == nullptr) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    SimplifyProgressEvent event{};
    event.input_triangle_count = input_triangles;
    event.current_triangle_count = current_triangles;
    event.has_target_triangle_count = target_triangles.has_value();
    event.target_triangle_count = target_triangles.value_or(0u);
    event.attempted_collapses = result.stats.attempted_collapses;
    event.accepted_collapses = result.stats.accepted_collapses;
    event.rejected_collapses = result.stats.rejected_collapses;
    event.latest_edge_cost = latest_edge_cost;
    event.max_edge_cost = result.stats.final_max_edge_error;
    event.elapsed_seconds = std::chrono::duration<double>(now - started).count();
    event.is_final = final_event;

    options.progress.callback(&event, options.progress.user_data);
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

    if (!IsValidTriangleMesh(input)) {
        result.status = SimplifyStatus::kInvalidInput;
        result.message = "qem: invalid indexed triangle mesh";
        result.stats.input_vertex_count = static_cast<std::uint32_t>(input.positions.size());
        result.stats.input_triangle_count = static_cast<std::uint32_t>(input.indices.size() / 3u);
        result.stats.output_vertex_count = static_cast<std::uint32_t>(output.positions.size());
        result.stats.output_triangle_count = static_cast<std::uint32_t>(output.indices.size() / 3u);
        return result;
    }

    std::vector<VertexRec> vertices;
    std::vector<FaceRec> faces;
    BuildWorkingMesh(input, vertices, faces);

    const std::optional<std::uint32_t> target_triangles =
        ResolveTargetTriangleCount(static_cast<std::uint32_t>(input.indices.size() / 3u), options);
    const std::uint32_t input_triangles = static_cast<std::uint32_t>(input.indices.size() / 3u);
    const std::uint32_t max_collapses =
        options.max_collapses.value_or(std::numeric_limits<std::uint32_t>::max());
    const std::optional<double> max_error = options.max_error;
    const bool emit_progress = (options.progress.callback != nullptr);
    const std::uint32_t progress_interval =
        std::max<std::uint32_t>(1u, options.progress.accepted_collapse_interval);
    std::uint32_t next_progress_log = progress_interval;
    const auto started = std::chrono::steady_clock::now();
    double latest_progress_cost = 0.0;

    StopReason stop_reason;

    for (;;) {
        DerivedState derived = RebuildDerivedState(vertices, faces, options);

        if (emit_progress) {
            if (result.stats.accepted_collapses == 0u && result.stats.attempted_collapses == 0u) {
                if (options.progress.emit_initial) {
                    EmitProgressEvent(options,
                                      result,
                                      input_triangles,
                                      derived.alive_face_count,
                                      target_triangles,
                                      started,
                                      latest_progress_cost,
                                      false);
                }
            } else if (result.stats.accepted_collapses >= next_progress_log) {
                EmitProgressEvent(options,
                                  result,
                                  input_triangles,
                                  derived.alive_face_count,
                                  target_triangles,
                                  started,
                                  latest_progress_cost,
                                  false);
                while (result.stats.accepted_collapses >= next_progress_log) {
                    next_progress_log += progress_interval;
                }
            }
        }

        if (target_triangles.has_value() && derived.alive_face_count <= *target_triangles) {
            stop_reason.kind = StopReason::kTargetReached;
            break;
        }

        if (result.stats.accepted_collapses >= max_collapses) {
            stop_reason.kind = StopReason::kMaxCollapses;
            break;
        }

        bool accepted_this_epoch = false;
        while (!derived.candidates.empty()) {
            const EdgeCandidate candidate = derived.candidates.top();
            derived.candidates.pop();

            if (!std::isfinite(candidate.cost)) {
                ++result.stats.attempted_collapses;
                ++result.stats.rejected_collapses;
                continue;
            }

            if (max_error.has_value() && candidate.cost > *max_error) {
                stop_reason.kind = StopReason::kErrorLimit;
                break;
            }

            ++result.stats.attempted_collapses;
            const CollapseDecision d = MakeDecisionFromCandidate(candidate);
            if (!ValidateCollapse(d, vertices, faces, derived, options)) {
                ++result.stats.rejected_collapses;
                continue;
            }

            ApplyCollapse(d, vertices, faces);
            ++result.stats.accepted_collapses;
            result.stats.accumulated_error += d.cost;
            if (d.cost > result.stats.final_max_edge_error) {
                result.stats.final_max_edge_error = d.cost;
            }
            latest_progress_cost = d.cost;

            if (options.emit_collapse_trace) {
                CollapseEvent evt;
                evt.step = result.stats.accepted_collapses - 1u;
                evt.kept_vertex = d.keep;
                evt.removed_vertex = d.remove;
                evt.new_position = ToVec3f(d.new_pos);
                evt.cost = d.cost;
                result.collapse_trace.push_back(evt);
            }

            accepted_this_epoch = true;
            break;
        }

        if (stop_reason.kind == StopReason::kErrorLimit) {
            break;
        }
        if (!accepted_this_epoch) {
            stop_reason.kind = StopReason::kNoMoreCandidates;
            break;
        }
    }

    BuildOutputMesh(vertices, faces, options, output, result);
    FillTerminalStats(input, vertices, faces, output, result);

    switch (stop_reason.kind) {
        case StopReason::kTargetReached:
            result.status = SimplifyStatus::kStoppedByTarget;
            result.message = "qem: target triangle count reached";
            break;
        case StopReason::kErrorLimit:
            result.status = SimplifyStatus::kStoppedByErrorLimit;
            result.message = "qem: stopped by error limit";
            break;
        case StopReason::kMaxCollapses:
            result.status = SimplifyStatus::kStoppedByMaxCollapses;
            result.message = "qem: stopped by max collapses";
            break;
        case StopReason::kNoMoreCandidates:
            if (result.stats.accepted_collapses == 0u) {
                result.status = SimplifyStatus::kNoReductionPossible;
                result.message = "qem: no valid collapses found";
            } else {
                result.status = SimplifyStatus::kSuccess;
                result.message = "qem: simplified (no further valid collapses)";
            }
            break;
        case StopReason::kNone:
        default:
            result.status = SimplifyStatus::kSuccess;
            result.message = "qem: simplified";
            break;
    }

    if (options.emit_collapse_trace && result.collapse_trace.size() != result.stats.accepted_collapses) {
        result.status = SimplifyStatus::kInternalError;
        result.message = "qem: collapse trace size mismatch";
    }

    if (emit_progress && options.progress.emit_final) {
        EmitProgressEvent(options,
                          result,
                          input_triangles,
                          result.stats.output_triangle_count,
                          target_triangles,
                          started,
                          latest_progress_cost,
                          true);
    }

    return result;
}

SimplifyResult SimplifyInPlace(IndexedTriangleMesh& mesh,
                               const SimplifyOptions& options) {
    IndexedTriangleMesh out;
    SimplifyResult result = Simplify(mesh, options, out);
    if (result.status != SimplifyStatus::kInvalidInput && result.status != SimplifyStatus::kInternalError) {
        mesh = std::move(out);
    }
    return result;
}

} // namespace glint_qem
