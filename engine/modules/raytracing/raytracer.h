// Machine Summary Block
// {"file":"engine/modules/raytracing/raytracer.h","purpose":"Declares the CPU ray tracer used for offline renders and previews.","exports":["Raytracer"],"depends_on":["ray.h","objloader.h","material.h","light.h","glm/glm.hpp"],"notes":["supports_glossy_reflections","builds_BVH_for_acceleration"]}
// Human Summary
// Core ray tracing engine handling BVH generation, glossy reflections, and refraction sampling.

#pragma once
/// @file raytracer.h
/// @brief Defines the CPU ray tracer responsible for offline path rendering in Glint3D.

#ifndef GLINT_ENABLE_RAYTRACING
#define GLINT_ENABLE_RAYTRACING 1
#endif

#include <vector>
#include <glm/glm.hpp>
#include "ray.h"
#include "objloader.h"
#include "material.h"
#include "light.h"

#if GLINT_ENABLE_RAYTRACING
#include "triangle.h"
#include "BVHNode.h"
#include "microfacet_sampling.h"
#include "raytracer_lighting.h"
#include "refraction.h"

/// @brief CPU raytracer capable of loading scene meshes and producing path-traced images.
class Raytracer
{
public:
    /// @brief Constructs an empty raytracer instance.
    Raytracer();

    /// @brief Adds geometry from an ObjLoader into the acceleration structure.
    /// @param loader Mesh data source.
    /// @param transform World transform applied to triangles.
    /// @param reflectivity Reflection strength used during shading.
    /// @param mat Material applied to the geometry.
    void loadModel(const ObjLoader& loader, const glm::mat4& transform, float reflectivity, const Material& mat);

    /// @brief Traces a single ray into the scene and returns the accumulated radiance.
    /// @param r Input ray in world space.
    /// @param lights Active scene lighting configuration.
    /// @param depth Remaining recursion depth (defaults to 3).
    /// @return RGB radiance contribution.
    glm::vec3 traceRay(const Ray& r, const Light& lights, int depth = 3) const;

    /// @brief Renders a full image using the loaded scene geometry.
    /// @param out Output color buffer.
    /// @param W Image width in pixels.
    /// @param H Image height in pixels.
    /// @param camPos Camera position.
    /// @param camFront Camera forward vector.
    /// @param camUp Camera up vector.
    /// @param fovDeg Field of view in degrees.
    /// @param lights Scene lighting data.
    void renderImage(std::vector<glm::vec3>& out,
        int W, int H,
        glm::vec3 camPos,
        glm::vec3 camFront,
        glm::vec3 camUp,
        float fovDeg,
        const Light& lights);
    
    /// @brief Seeds the random generator for deterministic sampling.
    /// @param seed Value used for RNG seeding.
    void setSeed(uint32_t seed) { m_seed = seed; }

    /// @brief Returns the current RNG seed.
    /// @return Seed value.
    uint32_t getSeed() const { return m_seed; }
    
    /// @brief Adjusts reflection samples per pixel for glossy materials.
    /// @param spp Samples per pixel.
    void setReflectionSpp(int spp) { m_reflectionSpp = spp; }

    /// @brief Retrieves the configured reflection sample count.
    /// @return Samples per pixel value.
    int getReflectionSpp() const { return m_reflectionSpp; }

private:
    std::vector<Triangle> triangles;
    glm::vec3 lightPos{0.0f};
    glm::vec3 lightColor{1.0f};
    BVHNode* bvhRoot = nullptr;
    uint32_t m_seed = 0;
    int m_reflectionSpp = 8;
    
    /// @brief Samples glossy reflections using microfacet importance sampling.
    glm::vec3 sampleGlossyReflection(
        const glm::vec3& hitPoint,
        const glm::vec3& viewDir,
        const glm::vec3& normal,
        const Material& material,
        const Light& lights,
        int depth,
        microfacet::SeededRNG& rng
    ) const;
    
    /// @brief Computes refraction contribution for dielectric materials.
    glm::vec3 computeRefraction(
        const glm::vec3& hitPoint,
        const glm::vec3& incident,
        const glm::vec3& normal,
        const Material& material,
        const Light& lights,
        int depth
    ) const;
};

#else // GLINT_ENABLE_RAYTRACING

/// @brief Stub raytracer used when the ray tracing module is disabled at build time.
class Raytracer
{
public:
    Raytracer() = default;
    void loadModel(const ObjLoader&, const glm::mat4&, float, const Material&) {}
    glm::vec3 traceRay(const Ray&, const Light&, int = 3) const { return glm::vec3(0.0f); }
    void renderImage(std::vector<glm::vec3>&, int, int, glm::vec3, glm::vec3, glm::vec3, float, const Light&) {}
    void setSeed(uint32_t) {}
    uint32_t getSeed() const { return 0; }
    void setReflectionSpp(int) {}
    int getReflectionSpp() const { return 0; }
};

#endif // GLINT_ENABLE_RAYTRACING
