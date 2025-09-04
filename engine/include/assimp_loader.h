#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "pbr_material.h"

// Import a triangulated mesh and basic PBR material via Assimp.
// - positions/normals always filled (normals auto-generated if missing)
// - uvs/tangents optional (set pointers to receive); v will be flipped if flipUV is true
// - minBound/maxBound computed across all meshes after baking transforms
// - pbrOut optional basic PBR material (factors + texture relative paths)
// Returns false if Assimp disabled or import fails.
bool AssimpImportMesh(const std::string& path,
                      std::vector<glm::vec3>& positions,
                      std::vector<unsigned>& indices,
                      std::vector<glm::vec3>& normals,
                      glm::vec3& minBound,
                      glm::vec3& maxBound,
                      std::string* error = nullptr,
                      std::vector<glm::vec2>* uvs = nullptr,
                      std::vector<glm::vec3>* tangents = nullptr,
                      PBRMaterial* pbrOut = nullptr,
                      bool flipUV = true);
