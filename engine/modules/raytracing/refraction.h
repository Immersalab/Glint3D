// Machine Summary Block
// {"file":"engine/modules/raytracing/refraction.h","purpose":"Declares refraction and Fresnel helpers for ray tracing","exports":["refraction::refract","refraction::fresnelSchlick","refraction::fresnelExact","refraction::determineMediaTransition"],"depends_on":["glm/glm.hpp","<cmath>"],"notes":["uses_schlick_approximation","handles_media_transition"]}
// Human Summary
// Refraction and Fresnel helpers supporting transmissive shading in the ray tracer.
#pragma once
/// @file refraction.h
/// @brief Refraction and Fresnel helper functions for ray tracing.

#include <glm/glm.hpp>
#include <cmath>

namespace refraction {

/**
 * Computes refracted ray direction using Snell's law
 * @param incident Incident ray direction (normalized)
 * @param normal Surface normal (normalized)
 * @param ior1 Index of refraction of the incident medium (e.g., 1.0 for air)
 * @param ior2 Index of refraction of the transmission medium (e.g., 1.5 for glass)
 * @param refracted Output refracted direction (normalized)
 * @return true if refraction occurs, false if total internal reflection
 */
bool refract(const glm::vec3& incident, const glm::vec3& normal, float ior1, float ior2, glm::vec3& refracted);

/**
 * Computes Fresnel reflectance using Schlick's approximation
 * @param cosTheta Cosine of angle between incident ray and normal
 * @param ior1 Index of refraction of first medium
 * @param ior2 Index of refraction of second medium
 * @return Reflectance factor (0 = all transmission, 1 = all reflection)
 */
float fresnelSchlick(float cosTheta, float ior1, float ior2);

/**
 * Computes exact Fresnel reflectance using Fresnel equations
 * @param cosTheta1 Cosine of incident angle
 * @param cosTheta2 Cosine of transmitted angle (use refract() to compute)
 * @param ior1 Index of refraction of first medium
 * @param ior2 Index of refraction of second medium
 * @return Reflectance factor (0 = all transmission, 1 = all reflection)
 */
float fresnelExact(float cosTheta1, float cosTheta2, float ior1, float ior2);

/**
 * Determines if we're entering or exiting a material based on ray direction and normal
 * @param incident Incident ray direction (normalized)
 * @param normal Surface normal (normalized)
 * @param materialIor IOR of the material
 * @param ior1 Output: IOR of incident medium
 * @param ior2 Output: IOR of transmission medium
 * @param adjustedNormal Output: normal adjusted for entering/exiting
 */
void determineMediaTransition(const glm::vec3& incident, const glm::vec3& normal, 
                             float materialIor, float& ior1, float& ior2, glm::vec3& adjustedNormal);

} // namespace refraction
