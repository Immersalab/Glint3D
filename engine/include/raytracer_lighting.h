#pragma once
#include <glm/glm.hpp>
#include "light.h"
#include "material.h"
#include "ray.h"

// Forward declaration
class Raytracer;

namespace raytracer {

    // Light sampling result
    struct LightSample {
        glm::vec3 direction;      // Direction from hit point to light
        glm::vec3 color;          // Light color * intensity
        float distance;           // Distance to light (infinite for directional)
        bool valid;               // Whether this light contributes
    };

    // Material evaluation result
    struct MaterialEval {
        glm::vec3 diffuse;        // Diffuse contribution
        glm::vec3 specular;       // Specular contribution
        glm::vec3 color;          // Final combined color
    };

    class LightingSystem {
    public:
        // Sample a specific light for a surface point
        static LightSample sampleLight(
            const LightSource& light,
            const glm::vec3& hitPoint,
            const glm::vec3& normal
        );

        // Evaluate material response to light
        static MaterialEval evaluateMaterial(
            const Material& material,
            const glm::vec3& normal,
            const glm::vec3& viewDir,
            const glm::vec3& lightDir,
            const glm::vec3& lightColor
        );

        // Compute ambient lighting contribution
        static glm::vec3 computeAmbient(
            const Material& material,
            const glm::vec4& globalAmbient
        );

        // Check if point is in shadow for a light
        static bool isInShadow(
            const glm::vec3& hitPoint,
            const glm::vec3& lightDir,
            float lightDistance,
            const Raytracer& raytracer
        );

        // Complete lighting calculation for a surface point
        static glm::vec3 computeLighting(
            const glm::vec3& hitPoint,
            const glm::vec3& normal,
            const glm::vec3& viewDir,
            const Material& material,
            const Light& lights,
            const Raytracer& raytracer
        );
    };

    // Helper functions for material properties
    namespace material {
        // Get base diffuse color accounting for metallic workflow
        glm::vec3 getBaseColor(const Material& mat);
        
        // Get effective ambient color
        glm::vec3 getAmbientColor(const Material& mat);
        
        // Get fresnel F0 for metallic workflow
        glm::vec3 getF0(const Material& mat);
    }
}