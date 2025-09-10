#pragma once

#include <glm/glm.hpp>
#include <random>

namespace microfacet
{
    // Utility class for deterministic random number generation with seed support
    class SeededRNG
    {
    public:
        explicit SeededRNG(uint32_t seed = 0) : m_rng(seed) {}
        
        void setSeed(uint32_t seed) { m_rng.seed(seed); }
        
        // Generate uniform float in [0, 1)
        float uniform() { return m_uniform(m_rng); }
        
        // Generate uniform float in [0, 1) - stratified sampling helper
        float stratified(int sample, int totalSamples, float jitter = 0.0f) {
            float base = (float(sample) + jitter) / float(totalSamples);
            return std::min(0.999999f, base); // Avoid exact 1.0
        }
        
    private:
        std::mt19937 m_rng;
        std::uniform_real_distribution<float> m_uniform{0.0f, 1.0f};
    };

    // Sample a microfacet normal using Beckmann distribution
    // Returns the sampled microfacet normal in world space
    glm::vec3 sampleBeckmannNormal(
        const glm::vec3& normal,           // Surface normal
        float roughness,                   // Material roughness [0, 1]
        SeededRNG& rng                     // Random number generator
    );
    
    // Sample multiple Beckmann normals with stratified sampling
    // Returns vector of sampled microfacet normals
    std::vector<glm::vec3> sampleBeckmannNormalsStratified(
        const glm::vec3& normal,           // Surface normal
        float roughness,                   // Material roughness [0, 1]
        int sampleCount,                   // Number of samples to generate
        SeededRNG& rng                     // Random number generator
    );
    
    // Check if roughness is close enough to zero for perfect mirror fallback
    inline bool shouldUsePerfectMirror(float roughness) {
        return roughness < 0.01f; // Threshold for mirror fallback
    }
    
    // Compute reflection direction for a given incident direction and microfacet normal
    inline glm::vec3 reflect(const glm::vec3& incident, const glm::vec3& normal) {
        return incident - 2.0f * glm::dot(incident, normal) * normal;
    }
}