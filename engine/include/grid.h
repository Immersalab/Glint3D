#pragma once

#include "gl_platform.h"
#include <glm/glm.hpp>
#include <vector>
#include "shader.h"
#include "colors.h"

class Grid {
public:
    Grid();
    ~Grid();

    // Initialize with shader, number of lines in each direction, and spacing between lines
    bool init(Shader* shader, int lineCount = 50, float spacing = 1.0f);

    // Render the grid using view/projection matrices
    void render(const glm::mat4& view, const glm::mat4& projection);

    // Cleanup OpenGL resources
    void cleanup();

private:
    GLuint m_VAO, m_VBO;
    Shader* m_shader;
    int m_lineCount;
    float m_spacing;
    std::vector<glm::vec3> m_lineVertices;
};
