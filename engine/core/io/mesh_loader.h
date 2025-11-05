#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "pbr_material.h"

struct MeshData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> tangents;
    std::vector<unsigned> indices; // triangles
    glm::vec3 minBound{0}, maxBound{0};
};

// Unified entry to load mesh + optional PBR material. Chooses parser by extension.
bool LoadMeshFromFile(const std::string& path, MeshData& out, PBRMaterial* pbrOut = nullptr, std::string* error = nullptr);
