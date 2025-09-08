#pragma once

#include <glm/glm.hpp>

namespace brdf
{
    // Cook–Torrance with Beckmann NDF, Cook–Torrance geometry term, Schlick Fresnel.
    // Inputs are all in world space and normalized except baseColor.
    // Returns BRDF value (not multiplied by light color/intensity), the caller should multiply by NdotL and light radiance.
    glm::vec3 cookTorrance(
        const glm::vec3& N,
        const glm::vec3& V,
        const glm::vec3& L,
        const glm::vec3& baseColor,
        float roughness,
        float metallic);
}

