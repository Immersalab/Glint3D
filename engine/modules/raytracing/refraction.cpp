#include "refraction.h"
#include <algorithm>

namespace refraction {

bool refract(const glm::vec3& incident, const glm::vec3& normal, float ior1, float ior2, glm::vec3& refracted) {
    float eta = ior1 / ior2;
    float cosI = -glm::dot(normal, incident);
    float sinT2 = eta * eta * (1.0f - cosI * cosI);
    
    // Check for total internal reflection
    if (sinT2 > 1.0f) {
        return false;
    }
    
    float cosT = std::sqrt(1.0f - sinT2);
    refracted = eta * incident + (eta * cosI - cosT) * normal;
    refracted = glm::normalize(refracted);
    
    return true;
}

float fresnelSchlick(float cosTheta, float ior1, float ior2) {
    // Schlick's approximation
    float r0 = (ior1 - ior2) / (ior1 + ior2);
    r0 = r0 * r0;
    
    float cosTheta_clamped = std::max(0.0f, std::min(1.0f, cosTheta));
    float oneMinusCos = 1.0f - cosTheta_clamped;
    float oneMinusCos2 = oneMinusCos * oneMinusCos;
    float oneMinusCos5 = oneMinusCos2 * oneMinusCos2 * oneMinusCos;
    
    return r0 + (1.0f - r0) * oneMinusCos5;
}

float fresnelExact(float cosTheta1, float cosTheta2, float ior1, float ior2) {
    // Exact Fresnel equations
    float rs = (ior1 * cosTheta1 - ior2 * cosTheta2) / (ior1 * cosTheta1 + ior2 * cosTheta2);
    float rp = (ior1 * cosTheta2 - ior2 * cosTheta1) / (ior1 * cosTheta2 + ior2 * cosTheta1);
    
    return 0.5f * (rs * rs + rp * rp);
}

void determineMediaTransition(const glm::vec3& incident, const glm::vec3& normal, 
                             float materialIor, float& ior1, float& ior2, glm::vec3& adjustedNormal) {
    // Determine if ray is entering or exiting the material
    // If dot product is positive, ray is hitting back face (exiting)
    // If dot product is negative, ray is hitting front face (entering)
    
    float dotProduct = glm::dot(incident, normal);
    
    if (dotProduct > 0.0f) {
        // Ray is exiting the material (hitting back face)
        ior1 = materialIor;    // From material
        ior2 = 1.0f;           // To air
        adjustedNormal = -normal;  // Flip normal for back face
    } else {
        // Ray is entering the material (hitting front face)
        ior1 = 1.0f;           // From air
        ior2 = materialIor;    // To material
        adjustedNormal = normal;   // Use normal as is
    }
}

} // namespace refraction