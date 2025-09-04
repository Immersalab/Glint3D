#pragma once
#include <vector>
#include "triangle.h"
#include "ray.h"
#include "objloader.h"
#include "material.h"
#include "BVHNode.h"
#include "light.h"  
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

private:
    std::vector<Triangle> triangles;
    glm::vec3 lightPos, lightColor;
    BVHNode* bvhRoot;
};
