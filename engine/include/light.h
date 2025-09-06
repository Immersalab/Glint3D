#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "gl_platform.h"

enum class LightType {
    POINT = 0,
    DIRECTIONAL = 1,
    SPOT = 2
};

struct LightSource {
    LightType type;
    glm::vec3 position;      // For point lights
    glm::vec3 direction;     // For directional/spot lights (normalized)
    glm::vec3 color;
    float intensity;
    bool enabled;
    // Spot light cone angles (degrees)
    float innerConeDeg = 15.0f;
    float outerConeDeg = 25.0f;
    
    LightSource() : type(LightType::POINT), position(0.0f), direction(0.0f, -1.0f, 0.0f), 
                    color(1.0f), intensity(1.0f), enabled(true) {}
};

class Light {
public:
    Light();
    ~Light();

    // Existing functions
    void addLight(const glm::vec3& position, const glm::vec3& color, float intensity);
    void addDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity);
    void addSpotLight(const glm::vec3& position, const glm::vec3& direction,
                      const glm::vec3& color, float intensity,
                      float innerConeDeg, float outerConeDeg);
    void applyLights(GLuint shaderProgram) const;
    size_t getLightCount() const;

    // New functions for indicator visualization
    void initIndicator();         // Create geometry (cube for point lights, arrow for directional)
    bool initIndicatorShader();   // Compile and link the indicator shader
    void renderIndicators(const glm::mat4& view, const glm::mat4& projection, int selectedIndex) const;

    glm::vec4 m_globalAmbient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    std::vector<LightSource> m_lights;

    const LightSource* getFirstLight() const;
    bool removeLightAt(size_t index);

private:

    // Indicator geometry for point lights (cube)
    GLuint m_indicatorVAO = 0;
    GLuint m_indicatorVBO = 0;

    // Indicator geometry for directional lights (arrow)
    GLuint m_arrowVAO = 0;
    GLuint m_arrowVBO = 0;

    // Indicator geometry for spot lights (cone outline)
    GLuint m_spotVAO = 0;
    GLuint m_spotVBO = 0;

    // Indicator shader program stored in the Light class
    GLuint m_indicatorShader = 0;
};
