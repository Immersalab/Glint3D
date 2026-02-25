// Machine Summary Block
// {"file":"tools/qem_simplifier/include/glint_qem_tool/mesh_preview_staging.h","purpose":"Declares helpers for staging meshes and JsonOps files so the QEM tool can preview via existing Glint CLI commands.","exports":["PreviewSceneStagingOptions","WriteObjMesh","WritePreviewOpsForMesh"],"depends_on":["glint_qem/types.h","<string>"],"notes":["reuses_glint_json_ops_load","render_with_specified_mesh_path"]}
// Human Summary
// Small staging helpers that convert an in-memory mesh into files Glint already understands (OBJ + JsonOps), avoiding a custom preview API too early.

#pragma once

#include <string>
#include <vector>

#include "glint_qem/types.h"

namespace glint_qem_tool {

enum class PreviewDisplayMode {
    kBackendDefault = 0,
    kSolid,
    kWireframe,
    kPoints
};

struct PreviewSceneStagingOptions {
    std::string object_name = "qem_preview";
    bool emit_camera = true;
    bool emit_light = false; // reserved; current ops rely on default scene lighting
    bool emit_select_loaded_object = false; // enables Glint's existing selection wireframe overlay path
    PreviewDisplayMode display_mode = PreviewDisplayMode::kBackendDefault;
    double camera_pos[3] = {5.0, 4.0, 5.0};
    double camera_target[3] = {0.0, 0.0, 0.0};
    double camera_up[3] = {0.0, 1.0, 0.0};
    double camera_fov_deg = 45.0;
};

bool WriteObjMesh(const glint_qem::IndexedTriangleMesh& mesh,
                  const std::string& obj_path,
                  std::string* error = nullptr);

bool WritePreviewOpsForMesh(const std::string& mesh_path,
                            const std::string& ops_path,
                            const PreviewSceneStagingOptions& options = {},
                            std::string* error = nullptr);

// Compose a preview JsonOps file by generating a `load` op for the mesh and appending the
// `ops` array from a preset JsonOps document (typically camera + lights exported from Glint UI).
bool WritePreviewOpsForMeshWithPreset(const std::string& mesh_path,
                                      const std::string& preset_ops_path,
                                      const std::string& ops_path,
                                      const PreviewSceneStagingOptions& options = {},
                                      std::string* error = nullptr);

// Compose a compare preview JsonOps file with two meshes loaded side-by-side.
// `offset_x` places the left mesh at -offset_x and the right mesh at +offset_x.
bool WriteCompareOpsForMeshes(const std::string& left_mesh_path,
                              const std::string& right_mesh_path,
                              const std::string& ops_path,
                              double offset_x,
                              const PreviewSceneStagingOptions& options = {},
                              std::string* error = nullptr);

bool WriteCompareOpsForMeshesWithPreset(const std::string& left_mesh_path,
                                        const std::string& right_mesh_path,
                                        const std::string& preset_ops_path,
                                        const std::string& ops_path,
                                        double offset_x,
                                        const PreviewSceneStagingOptions& options = {},
                                        std::string* error = nullptr);

} // namespace glint_qem_tool
