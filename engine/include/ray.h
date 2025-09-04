#ifndef RAY_H
#define RAY_H


#include "gl_platform.h"
#include <glm/glm.hpp>

class Ray {
public:
    glm::vec3 origin;    // Ray start position
    glm::vec3 direction; // Ray direction (normalized)

    Ray(const glm::vec3& orig, const glm::vec3& dir)
        : origin(orig), direction(glm::normalize(dir)) {} // Normalize direction
};

#endif
