#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <glm/glm.hpp>
#include "ray.h"                
#include "material.h"          


class Triangle
{
public:
    /* v0–v1–v2 can be in ANY order – normals are generated */
    Triangle(const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        float refl = 0.0f,
        const Material& mat = Material())    // <-- Added material param
        : v0(a), v1(b), v2(c), reflectivity(refl), material(mat)
    {
        /* geometric normal (unit length) */
        normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    }

    // Möller–Trumbore -- TWO-SIDED intersection test
    bool intersect(const Ray& r,
        float& tOut,
        glm::vec3& nOut) const
    {
        constexpr float EPS = 1e-6f;

        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;

        /* calculate determinant */
        glm::vec3 p = glm::cross(r.direction, e2);
        float det = glm::dot(e1, p);

        if (std::abs(det) < EPS) return false;  // ray ‖ triangle

        float invDet = 1.0f / det;

        /* calculate barycentric u */
        glm::vec3 s = r.origin - v0;
        float u = glm::dot(s, p) * invDet;
        if (u < 0.0f || u > 1.0f) return false;

        /* calculate barycentric v */
        glm::vec3 q = glm::cross(s, e1);
        float v = glm::dot(r.direction, q) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false;

        /* distance to intersection */
        float t = glm::dot(e2, q) * invDet;
        if (t <= EPS) return false;  // behind ray start

        /* hit! */
        tOut = t;
        nOut = normal;  // already normalized
        return true;
    }

public:
    // Data
    glm::vec3 v0, v1, v2;
    glm::vec3 normal;         // Face normal (unit vector)
    float reflectivity = 0.0f; // 0 = matte, 1 = mirror
    Material material;         // NEW: material data
};

#endif /* TRIANGLE_H */
