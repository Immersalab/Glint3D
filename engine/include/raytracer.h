#pragma once
#include <vector>
#include "triangle.h"
#include "ray.h"
#include "objloader.h"
#include "material.h"
#include "BVHNode.h"
#include "light.h"  
#include "microfacet_sampling.h"
#include "raytracer_lighting.h"
#include <glm/glm.hpp>

class Raytracer
{
public:
    Raytracer();

    void loadModel(const ObjLoader& loader, const glm::mat4& transform, float reflectivity, const Material& mat);

    glm::vec3 traceRay(const Ray& r, const Light& lights, int depth = 3) const;  
    void renderImage(std::vector<glm::vec3>& out,
        int W, int H,
        glm::vec3 camPos,
        glm::vec3 camFront,
        glm::vec3 camUp,
        float fovDeg,
        const Light& lights);
    
    // Seed support for deterministic random sampling
    void setSeed(uint32_t seed) { m_seed = seed; }
    uint32_t getSeed() const { return m_seed; }
    
    // Set reflection samples per pixel for glossy reflections
    void setReflectionSpp(int spp) { m_reflectionSpp = spp; }
    int getReflectionSpp() const { return m_reflectionSpp; }  

private:
    std::vector<Triangle> triangles;
    glm::vec3 lightPos, lightColor;
    BVHNode* bvhRoot;
    uint32_t m_seed = 0;
    int m_reflectionSpp = 8; // Default reflection samples per pixel
    
    // Helper method for glossy reflection sampling
    glm::vec3 sampleGlossyReflection(
        const glm::vec3& hitPoint,
        const glm::vec3& viewDir,
        const glm::vec3& normal,
        const Material& material,
        const Light& lights,
        int depth,
        microfacet::SeededRNG& rng
    ) const;
};
