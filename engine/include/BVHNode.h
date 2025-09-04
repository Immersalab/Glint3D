#pragma once
#include <vector>
#include "triangle.h"
#include "ray.h"
#include <glm/glm.hpp>

class BVHNode
{
public:
    glm::vec3 boundsMin;
    glm::vec3 boundsMax;
    std::vector<const Triangle*> triangles;
    BVHNode* left = nullptr;
    BVHNode* right = nullptr;

    ~BVHNode();

    bool intersect(const Ray& ray, const Triangle*& outTri, float& outT, glm::vec3& outNormal) const;

    bool intersectAny(const Ray& ray, const Triangle*& outTri, float& tOut) const;

};
