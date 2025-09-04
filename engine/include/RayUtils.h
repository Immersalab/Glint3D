#pragma once

#include "ray.h"
#include <glm/glm.hpp>

// Ray vs AABB intersection test
bool rayIntersectsAABB(
    const Ray& ray,
    const glm::vec3& aabbMin,
    const glm::vec3& aabbMax,
    float& t);
