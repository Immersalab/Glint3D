// Machine Summary Block
// {"file":"engine/modules/raytracing/brdf.h","purpose":"Declares BRDF utility functions for ray tracing shading","exports":["brdf::cookTorrance"],"depends_on":["glm/glm.hpp"],"notes":["cook_torrance_beckmann","requires_normalized_vectors"]}
// Human Summary
// Physically based BRDF helpers used by the raytracer shading pipeline.

#pragma once
/// @file brdf.h
/// @brief Cook-Torrance BRDF helpers for ray tracing shading.

#include <glm/glm.hpp>

namespace brdf
{
    /// @brief Computes the Cook-Torrance microfacet BRDF with Beckmann distribution.
    /// @param N Surface normal (normalized).
    /// @param V View vector (normalized).
    /// @param L Light vector (normalized).
    /// @param baseColor Base albedo color.
    /// @param roughness Surface roughness in [0,1].
    /// @param metallic Metallic factor in [0,1].
    /// @return Evaluated BRDF value (radiance multiplier).
    glm::vec3 cookTorrance(
        const glm::vec3& N,
        const glm::vec3& V,
        const glm::vec3& L,
        const glm::vec3& baseColor,
        float roughness,
        float metallic);
}
