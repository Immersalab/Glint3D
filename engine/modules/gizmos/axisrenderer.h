// Machine Summary Block
// {"file":"engine/modules/gizmos/axisrenderer.h","purpose":"Declares axis renderer used for world-space reference visuals.","exports":["AxisRenderer"],"depends_on":["glm/glm.hpp","gl_platform.h"],"notes":["embeds_shader_sources","renders XYZ axes"]}
// Human Summary
// Utility renderer that draws colored XYZ axes for orientation feedback.

#pragma once
/// @file axisrenderer.h
/// @brief Draws colored XYZ axes using embedded GLSL programs for debugging orientation.

#ifndef GLINT_ENABLE_GIZMOS
#define GLINT_ENABLE_GIZMOS 1
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#if GLINT_ENABLE_GIZMOS
#include "gl_platform.h"
#include <iostream>

/// @brief Simple renderer that draws an XYZ axis triad using fixed shader programs.
class AxisRenderer {
private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint shaderProgram = 0;

    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 ourColor;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        ourColor = aColor;
    })";

    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 ourColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(ourColor, 1.0);
    })";

public:
    /// @brief Constructs an uninitialized axis renderer.
    AxisRenderer();

    /// @brief Compiles shaders and prepares GPU buffers.
    void init();

    /// @brief Renders axes using provided model/view/projection matrices.
    /// @param modelMatrix Transform applied to the axis geometry.
    /// @param viewMatrix Camera view matrix.
    /// @param projectionMatrix Camera projection matrix.
    void render(glm::mat4& modelMatrix, glm::mat4& viewMatrix, glm::mat4& projectionMatrix);

    /// @brief Releases GPU resources allocated during init().
    void cleanup();
};

#else

/// @brief Stubbed axis renderer used when gizmos are disabled.
class AxisRenderer {
public:
    AxisRenderer() = default;
    void init() {}
    void render(glm::mat4&, glm::mat4&, glm::mat4&) {}
    void cleanup() {}
};

#endif // GLINT_ENABLE_GIZMOS
