// Machine Summary Block
// {"file":"engine/modules/gizmos/grid.h","purpose":"Declares the ground grid renderer for scene visualization.","exports":["Grid"],"depends_on":["glm/glm.hpp","gl_platform.h","shader.h","colors.h"],"notes":["requires_shader_binding_before_render"]}
// Human Summary
// Simple OpenGL grid helper drawing reference lines beneath the scene.

#pragma once
/// @file grid.h
/// @brief Reference grid renderer used to visualize world space under the camera.

#ifndef GLINT_ENABLE_GIZMOS
#define GLINT_ENABLE_GIZMOS 1
#endif

#include <glm/glm.hpp>
#include <vector>

#if GLINT_ENABLE_GIZMOS
#include "gl_platform.h"
#include "shader.h"
#include "colors.h"
#else
class Shader;
#endif

/// @brief Renders a configurable ground grid for editor visuals.
class Grid {
public:
#if GLINT_ENABLE_GIZMOS
    /// @brief Constructs an empty grid helper.
    Grid();

    /// @brief Releases any OpenGL resources owned by the grid.
    ~Grid();
#else
    Grid() = default;
    ~Grid() = default;
#endif

#if GLINT_ENABLE_GIZMOS
    /// @brief Initializes geometry buffers and associates a shader for rendering.
    bool init(Shader* shader, int lineCount = 50, float spacing = 1.0f);
    /// @brief Draws the grid using the provided view/projection matrices.
    void render(const glm::mat4& view, const glm::mat4& projection);
    /// @brief Releases GPU buffers associated with the grid.
    void cleanup();
#else
    bool init(Shader*, int = 50, float = 1.0f) { return true; }
    void render(const glm::mat4&, const glm::mat4&) {}
    void cleanup() {}
#endif

private:
#if GLINT_ENABLE_GIZMOS
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    Shader* m_shader = nullptr;
#else
    Shader* m_shader = nullptr;
#endif
    int m_lineCount = 0;
    float m_spacing = 1.0f;
    std::vector<glm::vec3> m_lineVertices;
};
