// Machine Summary Block
// {"file":"apps/qem_simplifier/include/glint_qem_tool/obj_mesh_io.h","purpose":"Declares OBJ mesh IO helpers for the external QEM tool layer (not qem_core).","exports":["ReadObjMesh"],"depends_on":["glint_qem/types.h","<string>"],"notes":["tinyobjloader_adapter_boundary","qem_core_remains_in_memory_only"]}
// Human Summary
// Tool-layer OBJ import helpers used to load meshes into the QEM core API without coupling qem_core to file IO.

#pragma once

#include <string>

#include "glint_qem/types.h"

namespace glint_qem_tool {

// Loads OBJ positions + triangle indices into qem_core mesh format.
// Requires tinyobjloader header to be available at build time; otherwise returns false with an explanatory error.
bool ReadObjMesh(const std::string& obj_path,
                 glint_qem::IndexedTriangleMesh& out_mesh,
                 std::string* error = nullptr);

} // namespace glint_qem_tool

