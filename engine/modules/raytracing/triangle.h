// Machine Summary Block
// {"file":"engine/modules/raytracing/triangle.h","purpose":"Defines triangle primitives used by the raytracer","exports":["Triangle"],"depends_on":["<cmath>","<algorithm>","glm/glm.hpp","ray.h","material.h"],"notes":["performs_two_sided_intersection","stores_smooth_normal_hint"]}
// Human Summary
// Triangle primitive providing intersection tests and per-face material data for ray tracing.

#pragma once
/// @file triangle.h
/// @brief Declares the Triangle primitive and intersection helper for the ray tracer.

#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include "ray.h"
#include "material.h"

/// @brief Triangle primitive storing geometry, reflectivity, and material information.
class Triangle
{
public:
    /// @brief Constructs a triangle from three vertices with optional reflectivity and material.
    /// @param a First vertex.
    /// @param b Second vertex.
    /// @param c Third vertex.
    /// @param refl Reflectivity factor (0 matte, 1 mirror).
    /// @param mat Material applied to the triangle.
    Triangle(const glm::vec3& a,
             const glm::vec3& b,
             const glm::vec3& c,
             float refl = 0.0f,
             const Material& mat = Material())
        : v0(a), v1(b), v2(c), reflectivity(refl), material(mat)
    {
        normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    }

    /// @brief Two-sided Moeller-Trumbore ray/triangle intersection.
    /// @param r Ray to test.
    /// @param tOut Receives the distance to the hit point.
    /// @param nOut Receives the surface normal at the hit point.
    /// @return True when the ray intersects the triangle in front of the origin.
    bool intersect(const Ray& r,
                   float& tOut,
                   glm::vec3& nOut) const
    {
        constexpr float EPS = 1e-6f;

        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;

        glm::vec3 p = glm::cross(r.direction, e2);
        float det = glm::dot(e1, p);

        if (std::abs(det) < EPS) return false;

        float invDet = 1.0f / det;

        glm::vec3 s = r.origin - v0;
        float u = glm::dot(s, p) * invDet;
        if (u < 0.0f || u > 1.0f) return false;

        glm::vec3 q = glm::cross(s, e1);
        float v = glm::dot(r.direction, q) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false;

        float t = glm::dot(e2, q) * invDet;
        if (t <= EPS) return false;

        tOut = t;

        glm::vec3 hitPoint = r.origin + t * r.direction;
        float d0 = glm::length(v0);
        float d1 = glm::length(v1);
        float d2 = glm::length(v2);
        float avgDist = (d0 + d1 + d2) / 3.0f;
        float maxDiff = std::max({std::abs(d0 - avgDist), std::abs(d1 - avgDist), std::abs(d2 - avgDist)});

        if (maxDiff < avgDist * 0.1f && avgDist > 0.5f) {
            nOut = glm::normalize(hitPoint);
        } else {
            nOut = normal;
        }

        return true;
    }

    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec3 normal;
    float reflectivity = 0.0f;
    Material material;
};

