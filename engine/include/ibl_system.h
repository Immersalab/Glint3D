#pragma once

#include "gl_platform.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class Shader;

class IBLSystem {
public:
    IBLSystem();
    ~IBLSystem();
    
    bool init();
    bool loadHDREnvironment(const std::string& hdrPath);
    
    void generateIrradianceMap();
    void generatePrefilterMap();
    void generateBRDFLUT();
    
    void bindIBLTextures() const;
    
    void setIntensity(float intensity) { m_intensity = intensity; }
    float getIntensity() const { return m_intensity; }
    
    GLuint getEnvironmentMap() const { return m_environmentMap; }
    GLuint getIrradianceMap() const { return m_irradianceMap; }
    GLuint getPrefilterMap() const { return m_prefilterMap; }
    GLuint getBRDFLUT() const { return m_brdfLUT; }
    
    void cleanup();

private:
    // Textures
    GLuint m_environmentMap;
    GLuint m_irradianceMap;
    GLuint m_prefilterMap;
    GLuint m_brdfLUT;
    
    // Framebuffers and renderbuffers for convolution
    GLuint m_captureFramebuffer;
    GLuint m_captureRenderbuffer;
    
    // Shaders
    Shader* m_equirectToCubemapShader;
    Shader* m_irradianceShader;
    Shader* m_prefilterShader;
    Shader* m_brdfShader;
    
    // Cube for rendering
    GLuint m_cubeVAO;
    GLuint m_quadVAO;
    
    float m_intensity;
    bool m_initialized;
    
    void setupCube();
    void setupQuad();
    void createShaders();
    GLuint loadHDRTexture(const std::string& path);
    void renderCube();
    void renderQuad();
};