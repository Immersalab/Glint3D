#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "gl_platform.h"

struct LightSource {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    bool enabled;
};

class Light {
public:
    Light();
    ~Light();

    // Existing functions
    void addLight(const glm::vec3& position, const glm::vec3& color, float intensity);
    void applyLights(GLuint shaderProgram) const;
    size_t getLightCount() const;

    // New functions for indicator visualization
    void initIndicator();         // Create geometry (a small cube)
    bool initIndicatorShader();   // Compile and link the indicator shader
    void renderIndicators(const glm::mat4& view, const glm::mat4& projection, int selectedIndex) const;

    glm::vec4 m_globalAmbient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    std::vector<LightSource> m_lights;

    const LightSource* getFirstLight() const;
    bool removeLightAt(size_t index);

private:

    // Indicator geometry
    GLuint m_indicatorVAO = 0;
    GLuint m_indicatorVBO = 0;

    // Indicator shader program stored in the Light class
    GLuint m_indicatorShader = 0;
};
