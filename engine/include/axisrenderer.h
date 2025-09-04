#ifndef AXIS_RENDERER_H
#define AXIS_RENDERER_H

#include "gl_platform.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

class AxisRenderer {
private:
    GLuint VAO, VBO, shaderProgram;

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
    AxisRenderer();
    void init();
    void render(glm::mat4& modelMatrix, glm::mat4& viewMatrix, glm::mat4& projectionMatrix);
    void cleanup();
};

#endif
