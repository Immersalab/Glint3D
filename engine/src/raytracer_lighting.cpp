#include "raytracer_lighting.h"
#include "raytracer.h"
#include "brdf.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace raytracer {

    LightSample LightingSystem::sampleLight(
        const LightSource& light,
        const glm::vec3& hitPoint,
        const glm::vec3& normal)
    {
        LightSample sample;
        sample.valid = false;

        if (!light.enabled) {
            return sample;
        }

        switch (light.type) {
            case LightType::DIRECTIONAL: {
                sample.direction = -glm::normalize(light.direction);
                sample.distance = std::numeric_limits<float>::infinity();
                sample.color = light.color * light.intensity;
                sample.valid = glm::dot(sample.direction, normal) > 0.0f;
                break;
            }
            
            case LightType::POINT: {
                glm::vec3 lightVec = light.position - hitPoint;
                sample.distance = glm::length(lightVec);
                if (sample.distance > 1e-6f) {
                    sample.direction = lightVec / sample.distance;
                    // Inverse square falloff for point lights
                    float attenuation = 1.0f / (1.0f + 0.1f * sample.distance + 0.01f * sample.distance * sample.distance);
                    sample.color = light.color * light.intensity * attenuation;
                    sample.valid = glm::dot(sample.direction, normal) > 0.0f;
                }
                break;
            }
            
            case LightType::SPOT: {
                glm::vec3 lightVec = light.position - hitPoint;
                sample.distance = glm::length(lightVec);
                if (sample.distance > 1e-6f) {
                    sample.direction = lightVec / sample.distance;
                    
                    // Check if point is within cone
                    float cosTheta = glm::dot(-sample.direction, glm::normalize(light.direction));
                    float innerCone = std::cos(glm::radians(light.innerConeDeg));
                    float outerCone = std::cos(glm::radians(light.outerConeDeg));
                    
                    if (cosTheta > outerCone) {
                        float attenuation = 1.0f / (1.0f + 0.1f * sample.distance + 0.01f * sample.distance * sample.distance);
                        
                        // Smooth falloff between inner and outer cone
                        if (cosTheta < innerCone) {
                            float falloff = (cosTheta - outerCone) / (innerCone - outerCone);
                            attenuation *= falloff * falloff;
                        }
                        
                        sample.color = light.color * light.intensity * attenuation;
                        sample.valid = glm::dot(sample.direction, normal) > 0.0f;
                    }
                }
                break;
            }
        }

        return sample;
    }

    MaterialEval LightingSystem::evaluateMaterial(
        const Material& material,
        const glm::vec3& normal,
        const glm::vec3& viewDir,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor)
    {
        MaterialEval eval;
        
        glm::vec3 baseColor = material::getBaseColor(material);
        float NdotL = std::max(0.0f, glm::dot(normal, lightDir));
        
        // Separate diffuse and specular evaluation for proper PBR workflow
        
        // Diffuse component (Lambertian)
        glm::vec3 diffuseAlbedo = baseColor * (1.0f - material.metallic); // Metals have no diffuse
        eval.diffuse = (diffuseAlbedo / glm::pi<float>()) * lightColor * NdotL;
        
        // Specular component (Cook-Torrance)
        glm::vec3 brdfSpecular = brdf::cookTorrance(normal, viewDir, lightDir, baseColor, material.roughness, material.metallic);
        eval.specular = brdfSpecular * lightColor * NdotL;
        
        // Combined final color
        eval.color = eval.diffuse + eval.specular;
        
        return eval;
    }

    glm::vec3 LightingSystem::computeAmbient(
        const Material& material,
        const glm::vec4& globalAmbient)
    {
        // Proper PBR ambient calculation
        glm::vec3 baseColor = material::getBaseColor(material);
        
        // For metals, ambient comes from F0 (fresnel reflectance at normal incidence)
        // For non-metals, ambient comes from diffuse albedo
        glm::vec3 f0 = material::getF0(material);
        glm::vec3 ambientContrib = glm::mix(baseColor, f0, material.metallic);
        
        // Apply global ambient with reasonable intensity
        glm::vec3 ambientLight = glm::vec3(globalAmbient) * globalAmbient.w;
        return ambientContrib * ambientLight * 0.1f; // Keep ambient subtle
    }

    bool LightingSystem::isInShadow(
        const glm::vec3& hitPoint,
        const glm::vec3& lightDir,
        float lightDistance,
        const Raytracer& raytracer)
    {
        // Offset to avoid self-intersection
        Ray shadowRay(hitPoint + lightDir * 0.001f, lightDir);
        
        // Check intersection - implementation depends on raytracer's public interface
        // For now, we'll return false (no shadows) to focus on lighting
        // TODO: Add proper shadow ray intersection
        return false;
    }

    glm::vec3 LightingSystem::computeLighting(
        const glm::vec3& hitPoint,
        const glm::vec3& normal,
        const glm::vec3& viewDir,
        const Material& material,
        const Light& lights,
        const Raytracer& raytracer)
    {
        glm::vec3 color(0.0f);

        // Add ambient lighting
        color += computeAmbient(material, lights.m_globalAmbient);

        // Process each light
        for (const auto& light : lights.m_lights) {
            LightSample sample = sampleLight(light, hitPoint, normal);
            
            if (sample.valid) {
                // Check for shadows (currently disabled for debugging)
                bool inShadow = false; // isInShadow(hitPoint, sample.direction, sample.distance, raytracer);
                
                if (!inShadow) {
                    MaterialEval eval = evaluateMaterial(material, normal, viewDir, sample.direction, sample.color);
                    color += eval.color;
                }
            }
        }

        return color;
    }

    namespace material {
        glm::vec3 getBaseColor(const Material& mat) {
            // For PBR workflow, diffuse is the base albedo color
            // Ensure minimum brightness for visibility
            glm::vec3 baseColor = mat.diffuse;
            
            // Ensure we have some color visibility
            if (glm::length(baseColor) < 0.1f) {
                baseColor = glm::vec3(0.5f); // Default to middle gray
            }
            
            return baseColor;
        }
        
        glm::vec3 getAmbientColor(const Material& mat) {
            // Use base color for ambient, but dimmer
            return getBaseColor(mat) * 0.2f;
        }
        
        glm::vec3 getF0(const Material& mat) {
            // Standard PBR F0 calculation
            const glm::vec3 dielectricF0(0.04f);
            return glm::mix(dielectricF0, getBaseColor(mat), mat.metallic);
        }
    }
}