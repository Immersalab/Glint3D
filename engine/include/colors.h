#pragma once
#include <glm/glm.hpp>

namespace Colors {
    // Primary colors
    inline constexpr glm::vec3 Red = glm::vec3(1.0f, 0.1f, 0.1f);
    inline constexpr glm::vec3 Green = glm::vec3(0.1f, 1.0f, 0.1f);
    inline constexpr glm::vec3 Blue = glm::vec3(0.1f, 0.1f, 1.0f);

    // Secondary colors
    inline constexpr glm::vec3 Yellow = glm::vec3(1.0f, 1.0f, 0.1f);
    inline constexpr glm::vec3 Cyan = glm::vec3(0.1f, 1.0f, 1.0f);
    inline constexpr glm::vec3 Magenta = glm::vec3(1.0f, 0.1f, 1.0f);

    // Grayscale tones
    inline constexpr glm::vec3 White = glm::vec3(1.0f);
    inline constexpr glm::vec3 LightGray = glm::vec3(0.75f);
    inline constexpr glm::vec3 Gray = glm::vec3(0.5f);
    inline constexpr glm::vec3 DarkGray = glm::vec3(0.25f);
    inline constexpr glm::vec3 Black = glm::vec3(0.0f);

    // Warm tones
    inline constexpr glm::vec3 Orange = glm::vec3(1.0f, 0.5f, 0.0f);
    inline constexpr glm::vec3 Gold = glm::vec3(1.0f, 0.84f, 0.0f);
    inline constexpr glm::vec3 Coral = glm::vec3(1.0f, 0.5f, 0.31f);
    inline constexpr glm::vec3 Salmon = glm::vec3(0.98f, 0.5f, 0.45f);

    // Cool tones
    inline constexpr glm::vec3 Teal = glm::vec3(0.0f, 0.5f, 0.5f);
    inline constexpr glm::vec3 Navy = glm::vec3(0.0f, 0.0f, 0.5f);
    inline constexpr glm::vec3 Indigo = glm::vec3(0.29f, 0.0f, 0.51f);
    inline constexpr glm::vec3 Purple = glm::vec3(0.5f, 0.0f, 0.5f);

    // Earthy/natural tones
    inline constexpr glm::vec3 Brown = glm::vec3(0.59f, 0.29f, 0.0f);
    inline constexpr glm::vec3 Olive = glm::vec3(0.5f, 0.5f, 0.0f);
    inline constexpr glm::vec3 Forest = glm::vec3(0.13f, 0.55f, 0.13f);
    inline constexpr glm::vec3 Sand = glm::vec3(0.76f, 0.7f, 0.5f);

    // UI highlight suggestions
    inline constexpr glm::vec3 Highlight = glm::vec3(1.0f, 0.9f, 0.2f);
    inline constexpr glm::vec3 Error = Red;
    inline constexpr glm::vec3 Success = Green;
    inline constexpr glm::vec3 Info = Cyan;
}
