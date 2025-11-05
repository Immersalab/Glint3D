// Machine Summary Block
// {"file":"engine/modules/raytracing/RayUtils.h","purpose":"Declares utility intersection helpers for ray tracing","exports":["rayIntersectsAABB"],"depends_on":["ray.h","glm/glm.hpp"],"notes":["used_by_BVH","expects_normalized_direction"]}
// Human Summary
// Intersection helpers for the ray tracing module.

#pragma once
/// @file RayUtils.h
/// @brief Utility functions supporting ray intersection tests.

#ifndef GLINT_ENABLE_RAYTRACING
#define GLINT_ENABLE_RAYTRACING 1
#endif

#include "ray.h"
#include <glm/glm.hpp>
#include <algorithm>

/// @brief Tests a ray against an axis-aligned bounding box.
/// @param ray Ray to test (direction assumed normalized).
/// @param aabbMin Minimum corner of the box.
/// @param aabbMax Maximum corner of the box.
/// @param t Receives the distance to the first intersection.
/// @return True if the ray intersects the volume.
#if GLINT_ENABLE_RAYTRACING
bool rayIntersectsAABB(
    const Ray& ray,
    const glm::vec3& aabbMin,
    const glm::vec3& aabbMax,
    float& t);
#else
inline bool rayIntersectsAABB(
    const Ray& ray,
    const glm::vec3& aabbMin,
    const glm::vec3& aabbMax,
    float& t)
{
    float tmin = (aabbMin.x - ray.origin.x) / ray.direction.x;
    float tmax = (aabbMax.x - ray.origin.x) / ray.direction.x;
    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (aabbMin.y - ray.origin.y) / ray.direction.y;
    float tymax = (aabbMax.y - ray.origin.y) / ray.direction.y;
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (aabbMin.z - ray.origin.z) / ray.direction.z;
    float tzmax = (aabbMax.z - ray.origin.z) / ray.direction.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    t = tmin;
    if (t < 0) {
        t = tmax;
        if (t < 0)
            return false;
    }

    return true;
}
#endif
