#pragma once

#include "gl_platform.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

class Shader;

class Skybox {
public:
    Skybox();
    ~Skybox();
    
    // Initialize skybox with procedural gradient or color
    bool init();
    
    // Load skybox from cubemap textures (6 faces)
    bool loadCubemap(const std::vector<std::string>& faces);
    
    // Set procedural gradient colors
    void setGradient(const glm::vec3& topColor, const glm::vec3& bottomColor, const glm::vec3& horizonColor = glm::vec3(0.0f));
    
    // Render the skybox
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // Enable/disable skybox rendering
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // Set skybox intensity/brightness
    void setIntensity(float intensity) { m_intensity = intensity; }
    float getIntensity() const { return m_intensity; }
    
    // Cleanup
    void cleanup();

private:
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_cubemapTexture;
    Shader* m_shader;
    
    bool m_enabled;
    bool m_initialized;
    bool m_useGradient;
    float m_intensity;
    
    // Gradient colors
    glm::vec3 m_topColor;
    glm::vec3 m_bottomColor;
    glm::vec3 m_horizonColor;
    
    void setupCube();
    void createProceduralSkybox();
    GLuint loadCubemapTexture(const std::vector<std::string>& faces);
};