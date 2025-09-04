#pragma once

#include <string>
#include <glm/glm.hpp>

// Simple physically-based material description used across importers and renderers.
// Textures are specified as file paths here; runtime maps them to Texture* via the cache.
struct PBRMaterial
{
    glm::vec4 baseColorFactor{1.0f}; // sRGB base color + alpha
    float metallicFactor{1.0f};
    float roughnessFactor{1.0f};

    std::string baseColorTex;   // color/albedo (sRGB)
    std::string normalTex;      // tangent-space normal
    std::string mrTex;          // metallic-roughness packed (G=roughness, B=metallic)
    std::string aoTex;          // optional occlusion map (R=AO)
};

