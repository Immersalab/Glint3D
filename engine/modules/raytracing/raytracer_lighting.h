// Machine Summary Block
// {"file":"engine/modules/raytracing/raytracer_lighting.h","purpose":"Declares lighting helpers for the raytracing module","exports":["raytracer::LightingSystem","raytracer::material::getBaseColor","raytracer::material::getAmbientColor","raytracer::material::getF0"],"depends_on":["glm/glm.hpp","light.h","material.h","ray.h"],"notes":["samples_lights","performs_shadow_queries"]}
// Human Summary
// Lighting utilities that evaluate materials and lights for the ray tracer.

#pragma once
/// @file raytracer_lighting.h
/// @brief Lighting helpers supporting the ray tracer shading pipeline.

#include <glm/glm.hpp>
#include "light.h"
#include "material.h"
#include "ray.h"

class Raytracer;

namespace raytracer {

    /// @brief Describes a sampled contribution from a light source.
    struct LightSample {
        glm::vec3 direction{0.0f}; ///< Direction from hit point to light.
        glm::vec3 color{0.0f};     ///< Light radiance contribution.
        float distance = 0.0f;     ///< Distance to the light (infinity for directional).
        bool valid = false;        ///< Indicates whether the light contributes.
    };

    /// @brief Captures diffuse/specular evaluation for a material.
    struct MaterialEval {
        glm::vec3 diffuse{0.0f};  ///< Diffuse lighting term.
        glm::vec3 specular{0.0f}; ///< Specular lighting term.
        glm::vec3 color{0.0f};    ///< Combined color contribution.
    };

    /// @brief Static lighting utilities used by the raytracer.
    class LightingSystem {
    public:
        /// @brief Samples a light source for a given surface point.
        static LightSample sampleLight(
            const LightSource& light,
            const glm::vec3& hitPoint,
            const glm::vec3& normal
        );

        /// @brief Evaluates material response for an incident light direction.
        static MaterialEval evaluateMaterial(
            const Material& material,
            const glm::vec3& normal,
            const glm::vec3& viewDir,
            const glm::vec3& lightDir,
            const glm::vec3& lightColor
        );

        /// @brief Computes ambient lighting contribution.
        static glm::vec3 computeAmbient(
            const Material& material,
            const glm::vec4& globalAmbient
        );

        /// @brief Determines whether a surface point is shadowed for a light.
        static bool isInShadow(
            const glm::vec3& hitPoint,
            const glm::vec3& lightDir,
            float lightDistance,
            const Raytracer& raytracer
        );

        /// @brief Computes the full lighting contribution for a shading point.
        static glm::vec3 computeLighting(
            const glm::vec3& hitPoint,
            const glm::vec3& normal,
            const glm::vec3& viewDir,
            const Material& material,
            const Light& lights,
            const Raytracer& raytracer
        );
    };

    namespace material {
        /// @brief Returns the base color used for diffuse lighting in the metallic workflow.
        glm::vec3 getBaseColor(const Material& mat);
        
        /// @brief Computes the ambient color contribution of the material.
        glm::vec3 getAmbientColor(const Material& mat);
        
        /// @brief Computes the Fresnel F0 term for the material.
        glm::vec3 getF0(const Material& mat);
    }
}
