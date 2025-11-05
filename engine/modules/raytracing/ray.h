// Machine Summary Block
// {"file":"engine/modules/raytracing/ray.h","purpose":"Defines the Ray struct used across ray tracing helpers","exports":["Ray"],"depends_on":["gl_platform.h","glm/glm.hpp"],"notes":["direction_normalized_in_constructor"]}
// Human Summary
// Basic ray representation with origin and normalized direction.

#pragma once
/// @file ray.h
/// @brief Declares the Ray utility type shared across ray tracing helpers.

#include "gl_platform.h"
#include <glm/glm.hpp>

/// @brief Represents a ray with origin and normalized direction.
class Ray {
public:
    glm::vec3 origin{0.0f};    ///< Ray start position.
    glm::vec3 direction{0.0f}; ///< Ray direction (normalized).

    /// @brief Constructs a ray and normalizes the supplied direction.
    /// @param orig Ray origin.
    /// @param dir Direction vector to be normalized.
    Ray(const glm::vec3& orig, const glm::vec3& dir)
        : origin(orig), direction(glm::normalize(dir)) {}
};
