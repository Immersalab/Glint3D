// Machine Summary Block
// {"file":"engine/modules/raytracing/microfacet_sampling.h","purpose":"Provides microfacet sampling helpers for the raytracer","exports":["microfacet::SeededRNG","microfacet::sampleBeckmannNormal","microfacet::sampleBeckmannNormalsStratified","microfacet::shouldUsePerfectMirror","microfacet::reflect"],"depends_on":["glm/glm.hpp","<vector>","<random>","<algorithm>"],"notes":["beckmann_distribution_sampling","deterministic_rng_support"]}
// Human Summary
// Deterministic microfacet sampling utilities used for glossy reflections.

#pragma once
/// @file microfacet_sampling.h
/// @brief Microfacet sampling helpers for Cook-Torrance shading in the ray tracer.

#include <algorithm>
#include <random>
#include <vector>
#include <glm/glm.hpp>

namespace microfacet
{
    /// @brief Deterministic RNG wrapper that supports seeding and stratified sampling.
    class SeededRNG
    {
    public:
        /// @brief Constructs the RNG with an optional seed.
        explicit SeededRNG(uint32_t seed = 0) : m_rng(seed) {}
        
        /// @brief Updates the RNG seed.
        /// @param seed Seed value used to reinitialize the generator.
        void setSeed(uint32_t seed) { m_rng.seed(seed); }
        
        /// @brief Generates a uniform float in [0,1).
        /// @return Random float sample.
        float uniform() { return m_uniform(m_rng); }
        
        /// @brief Returns a stratified sample within [0,1).
        /// @param sample Index of the stratum.
        /// @param totalSamples Total number of stratifications.
        /// @param jitter Optional jitter applied within the stratum.
        /// @return Stratified sample clamped to below 1.0.
        float stratified(int sample, int totalSamples, float jitter = 0.0f) {
            float base = (static_cast<float>(sample) + jitter) / static_cast<float>(totalSamples);
            return std::min(0.999999f, base);
        }
        
    private:
        std::mt19937 m_rng;
        std::uniform_real_distribution<float> m_uniform{0.0f, 1.0f};
    };

    /// @brief Samples a microfacet normal using the Beckmann distribution.
    /// @param normal Surface normal in world space (normalized).
    /// @param roughness Material roughness value.
    /// @param rng Random number generator producing uniform samples.
    /// @return Sampled microfacet normal in world space.
    glm::vec3 sampleBeckmannNormal(
        const glm::vec3& normal,
        float roughness,
        SeededRNG& rng
    );
    
    /// @brief Generates multiple Beckmann-distributed normals with stratification.
    /// @param normal Surface normal in world space (normalized).
    /// @param roughness Material roughness value.
    /// @param sampleCount Number of samples requested.
    /// @param rng Random number generator producing uniform samples.
    /// @return Collection of sampled microfacet normals.
    std::vector<glm::vec3> sampleBeckmannNormalsStratified(
        const glm::vec3& normal,
        float roughness,
        int sampleCount,
        SeededRNG& rng
    );
    
    /// @brief Indicates whether a perfect mirror BRDF should be used instead of microfacet sampling.
    /// @param roughness Material roughness value.
    /// @return True when the surface is sufficiently smooth for mirror behavior.
    inline bool shouldUsePerfectMirror(float roughness) {
        return roughness < 0.01f;
    }
    
    /// @brief Computes reflection direction from an incident vector and normal.
    /// @param incident Incident direction (normalized).
    /// @param normal Surface normal (normalized).
    /// @return Reflected direction.
    inline glm::vec3 reflect(const glm::vec3& incident, const glm::vec3& normal) {
        return incident - 2.0f * glm::dot(incident, normal) * normal;
    }
}
