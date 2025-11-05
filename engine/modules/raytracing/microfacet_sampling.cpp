#include "microfacet_sampling.h"
#include <cmath>
#include <algorithm>

namespace
{
    constexpr float PI = 3.14159265358979323846f;
    
    // Create orthonormal basis from normal vector
    // Returns tangent and bitangent vectors perpendicular to normal
    void createOrthonormalBasis(const glm::vec3& normal, glm::vec3& tangent, glm::vec3& bitangent) {
        // Choose axis with smallest component to avoid near-parallel vectors
        glm::vec3 up = (std::abs(normal.y) < 0.999f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        tangent = glm::normalize(glm::cross(normal, up));
        bitangent = glm::cross(normal, tangent);
    }
    
    // Transform direction from tangent space to world space
    glm::vec3 tangentToWorld(const glm::vec3& tangentDir, const glm::vec3& normal, 
                           const glm::vec3& tangent, const glm::vec3& bitangent) {
        return tangentDir.x * tangent + tangentDir.y * bitangent + tangentDir.z * normal;
    }
}

namespace microfacet
{
    glm::vec3 sampleBeckmannNormal(const glm::vec3& normal, float roughness, SeededRNG& rng)
    {
        // Clamp roughness to avoid singularities
        const float alpha = std::max(0.001f, roughness * roughness);
        
        // Generate two uniform random numbers
        const float u1 = rng.uniform();
        const float u2 = rng.uniform();
        
        // Sample Beckmann distribution in spherical coordinates
        // theta is the angle from the normal (polar angle)
        // phi is the azimuthal angle
        
        // Beckmann distribution sampling:
        // tan²(theta) = -alpha² * ln(u1)
        // phi = 2π * u2
        
        const float tan2Theta = -alpha * alpha * std::log(std::max(1e-6f, u1));
        const float cosTheta = 1.0f / std::sqrt(1.0f + tan2Theta);
        const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        
        const float phi = 2.0f * PI * u2;
        const float cosPhi = std::cos(phi);
        const float sinPhi = std::sin(phi);
        
        // Convert to Cartesian coordinates in tangent space
        const glm::vec3 microfacetTangent(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
        
        // Create orthonormal basis around the surface normal
        glm::vec3 tangent, bitangent;
        createOrthonormalBasis(normal, tangent, bitangent);
        
        // Transform from tangent space to world space
        return glm::normalize(tangentToWorld(microfacetTangent, normal, tangent, bitangent));
    }
    
    std::vector<glm::vec3> sampleBeckmannNormalsStratified(
        const glm::vec3& normal, 
        float roughness, 
        int sampleCount, 
        SeededRNG& rng)
    {
        std::vector<glm::vec3> samples;
        samples.reserve(sampleCount);
        
        // Clamp roughness to avoid singularities
        const float alpha = std::max(0.001f, roughness * roughness);
        
        // Create orthonormal basis around the surface normal
        glm::vec3 tangent, bitangent;
        createOrthonormalBasis(normal, tangent, bitangent);
        
        // Generate stratified samples
        for (int i = 0; i < sampleCount; ++i) {
            // Stratified sampling with jitter
            const float jitter1 = rng.uniform();
            const float jitter2 = rng.uniform();
            
            const float u1 = rng.stratified(i, sampleCount, jitter1);
            const float u2 = rng.stratified(i, sampleCount, jitter2);
            
            // Sample Beckmann distribution
            const float tan2Theta = -alpha * alpha * std::log(std::max(1e-6f, u1));
            const float cosTheta = 1.0f / std::sqrt(1.0f + tan2Theta);
            const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
            
            const float phi = 2.0f * PI * u2;
            const float cosPhi = std::cos(phi);
            const float sinPhi = std::sin(phi);
            
            // Convert to Cartesian coordinates in tangent space
            const glm::vec3 microfacetTangent(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            
            // Transform to world space and add to samples
            samples.push_back(glm::normalize(tangentToWorld(microfacetTangent, normal, tangent, bitangent)));
        }
        
        return samples;
    }
}