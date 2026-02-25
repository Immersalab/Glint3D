// Machine Summary Block
// {"file":"tools/qem_simplifier/src/backends/obj_mesh_io_tinyobj.cpp","purpose":"Implements OBJ import for the QEM external tool via tinyobjloader when available.","exports":["glint_qem_tool::ReadObjMesh"],"depends_on":["glint_qem_tool/obj_mesh_io.h"],"notes":["tool_layer_io","graceful_no_header_error","positions_plus_triangle_indices_only"]}
// Human Summary
// OBJ loader wrapper for the QEM external tool. Uses tinyobjloader if present on the include path; otherwise returns a clear error.

#include "glint_qem_tool/obj_mesh_io.h"

#include <cstdint>
#include <string>
#include <vector>

#if __has_include(<tiny_obj_loader.h>)
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLINT_QEM_TINYOBJ_AVAILABLE 1
#else
#define GLINT_QEM_TINYOBJ_AVAILABLE 0
#endif

namespace glint_qem_tool {

bool ReadObjMesh(const std::string& obj_path,
                 glint_qem::IndexedTriangleMesh& out_mesh,
                 std::string* error) {
    out_mesh.positions.clear();
    out_mesh.indices.clear();

#if !GLINT_QEM_TINYOBJ_AVAILABLE
    if (error) {
        *error = "tinyobjloader header not found; install tiny_obj_loader.h and add its include path to tools/qem_simplifier";
    }
    (void)obj_path;
    return false;
#else
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.vertex_color = false;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(obj_path, config)) {
        std::string msg;
        if (!reader.Warning().empty()) {
            msg += reader.Warning();
            if (!reader.Error().empty()) msg += " | ";
        }
        if (!reader.Error().empty()) msg += reader.Error();
        if (msg.empty()) msg = "tinyobjloader parse failed";
        if (error) *error = msg;
        return false;
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    if (attrib.vertices.size() < 3u || (attrib.vertices.size() % 3u) != 0u) {
        if (error) *error = "OBJ vertex position buffer is invalid";
        return false;
    }

    out_mesh.positions.reserve(attrib.vertices.size() / 3u);
    for (std::size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
        glint_qem::Vec3f p;
        p.x = static_cast<float>(attrib.vertices[i + 0]);
        p.y = static_cast<float>(attrib.vertices[i + 1]);
        p.z = static_cast<float>(attrib.vertices[i + 2]);
        out_mesh.positions.push_back(p);
    }

    for (const tinyobj::shape_t& shape : shapes) {
        std::size_t index_offset = 0;
        for (std::size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            const int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                if (error) *error = "OBJ contains non-triangular faces after triangulation";
                return false;
            }
            for (int v = 0; v < 3; ++v) {
                const tinyobj::index_t idx = shape.mesh.indices[index_offset + static_cast<std::size_t>(v)];
                if (idx.vertex_index < 0) {
                    if (error) *error = "OBJ contains invalid negative vertex index";
                    return false;
                }
                const std::uint32_t vi = static_cast<std::uint32_t>(idx.vertex_index);
                if (vi >= out_mesh.positions.size()) {
                    if (error) *error = "OBJ vertex index out of range";
                    return false;
                }
                out_mesh.indices.push_back(vi);
            }
            index_offset += 3u;
        }
    }

    if (out_mesh.indices.empty()) {
        if (error) *error = "OBJ has no triangle faces";
        return false;
    }
    if (error && !reader.Warning().empty()) {
        *error = reader.Warning();
    }
    return true;
#endif
}

} // namespace glint_qem_tool

