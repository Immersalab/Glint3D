#include <iostream>
#include <cassert>
#include <glm/glm.hpp>
#include "../../engine/include/brdf.h"

static bool nearlyZero(const glm::vec3& v, float eps = 1e-6f)
{
    return std::abs(v.x) < eps && std::abs(v.y) < eps && std::abs(v.z) < eps;
}

int main()
{
    std::cout << "Running BRDF edge-condition tests...\n";

    const glm::vec3 baseColor(0.8f, 0.7f, 0.6f);
    const float roughness = 0.5f;
    const float metallic = 0.0f;

    // Case 1: N·L <= 0 -> BRDF should be zero
    {
        glm::vec3 N(0, 1, 0);
        glm::vec3 V(0, 1, 0);
        glm::vec3 L(0, -1, 0); // Opposite direction, N·L = -1
        glm::vec3 f = brdf::cookTorrance(N, V, L, baseColor, roughness, metallic);
        assert(nearlyZero(f) && "BRDF must be zero when N·L <= 0");
        std::cout << "✓ N·L <= 0 returns 0" << std::endl;
    }

    // Case 2: N·V <= 0 -> BRDF should be zero
    {
        glm::vec3 N(0, 1, 0);
        glm::vec3 V(0, -1, 0); // Looking away, N·V = -1
        glm::vec3 L(0, 1, 0);
        glm::vec3 f = brdf::cookTorrance(N, V, L, baseColor, roughness, metallic);
        assert(nearlyZero(f) && "BRDF must be zero when N·V <= 0");
        std::cout << "✓ N·V <= 0 returns 0" << std::endl;
    }

    std::cout << "\n✅ BRDF tests passed!\n";
    return 0;
}

