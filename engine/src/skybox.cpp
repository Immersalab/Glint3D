#include "skybox.h"
#include "shader.h"
#include "stb_image.h"
#include <iostream>

namespace {
    // Skybox cube vertices (positions only)
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
}

Skybox::Skybox()
    : m_VAO(0)
    , m_VBO(0)
    , m_cubemapTexture(0)
    , m_shader(nullptr)
    , m_enabled(true)
    , m_initialized(false)
    , m_useGradient(true)
    , m_intensity(1.0f)
    , m_topColor(0.2f, 0.4f, 0.8f)      // Sky blue
    , m_bottomColor(0.8f, 0.9f, 1.0f)   // Light blue/white
    , m_horizonColor(0.9f, 0.8f, 0.7f)  // Warm horizon
{
}

Skybox::~Skybox()
{
    cleanup();
}

bool Skybox::init()
{
    if (m_initialized) return true;
    
    // Create and compile shader
    m_shader = new Shader();
    if (!m_shader->loadFromStrings(
        // Vertex shader
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 TexCoords;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    TexCoords = aPos;\n"
        "    vec4 pos = projection * view * vec4(aPos, 1.0);\n"
        "    gl_Position = pos.xyww;\n"  // Ensure skybox is always at far plane
        "}\n",
        
        // Fragment shader
        "#version 330 core\n"
        "in vec3 TexCoords;\n"
        "out vec4 FragColor;\n"
        "uniform samplerCube skybox;\n"
        "uniform bool useGradient;\n"
        "uniform vec3 topColor;\n"
        "uniform vec3 bottomColor;\n"
        "uniform vec3 horizonColor;\n"
        "uniform float intensity;\n"
        "void main() {\n"
        "    if (useGradient) {\n"
        "        float t = normalize(TexCoords).y;\n"
        "        vec3 color;\n"
        "        if (t > 0.0) {\n"
        "            // Upper hemisphere: interpolate from horizon to top\n"
        "            float factor = smoothstep(0.0, 1.0, t);\n"
        "            color = mix(horizonColor, topColor, factor);\n"
        "        } else {\n"
        "            // Lower hemisphere: interpolate from horizon to bottom\n"
        "            float factor = smoothstep(0.0, -1.0, t);\n"
        "            color = mix(horizonColor, bottomColor, factor);\n"
        "        }\n"
        "        FragColor = vec4(color * intensity, 1.0);\n"
        "    } else {\n"
        "        FragColor = texture(skybox, TexCoords) * vec4(vec3(intensity), 1.0);\n"
        "    }\n"
        "}\n"
    )) {
        std::cerr << "Failed to create skybox shader" << std::endl;
        delete m_shader;
        m_shader = nullptr;
        return false;
    }
    
    setupCube();
    createProceduralSkybox();
    
    m_initialized = true;
    return true;
}

void Skybox::setupCube()
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void Skybox::createProceduralSkybox()
{
    // Create a simple 1x1 white texture for gradient mode
    glGenTextures(1, &m_cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
    
    unsigned char whitePixel[3] = {255, 255, 255};
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    m_useGradient = true;
}

bool Skybox::loadCubemap(const std::vector<std::string>& faces)
{
    if (faces.size() != 6) {
        std::cerr << "Skybox: Need exactly 6 faces for cubemap" << std::endl;
        return false;
    }
    
    if (m_cubemapTexture != 0) {
        glDeleteTextures(1, &m_cubemapTexture);
    }
    
    glGenTextures(1, &m_cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
    
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 4) format = GL_RGBA;
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Skybox: Failed to load texture: " << faces[i] << std::endl;
            stbi_image_free(data);
            return false;
        }
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    m_useGradient = false;
    return true;
}

void Skybox::setGradient(const glm::vec3& topColor, const glm::vec3& bottomColor, const glm::vec3& horizonColor)
{
    m_topColor = topColor;
    m_bottomColor = bottomColor;
    m_horizonColor = horizonColor.x == 0.0f && horizonColor.y == 0.0f && horizonColor.z == 0.0f 
        ? glm::mix(topColor, bottomColor, 0.5f) 
        : horizonColor;
    m_useGradient = true;
}

void Skybox::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_enabled || !m_initialized || !m_shader) return;
    
    // Remove translation from the view matrix
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    
    // Render skybox last (after all other geometry)
    glDepthFunc(GL_LEQUAL);
    
    m_shader->use();
    m_shader->setMat4("view", skyboxView);
    m_shader->setMat4("projection", projection);
    m_shader->setBool("useGradient", m_useGradient);
    m_shader->setVec3("topColor", m_topColor);
    m_shader->setVec3("bottomColor", m_bottomColor);
    m_shader->setVec3("horizonColor", m_horizonColor);
    m_shader->setFloat("intensity", m_intensity);
    
    // Bind cubemap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
    m_shader->setInt("skybox", 0);
    
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    // Restore depth function
    glDepthFunc(GL_LESS);
}

void Skybox::cleanup()
{
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO != 0) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    if (m_cubemapTexture != 0) {
        glDeleteTextures(1, &m_cubemapTexture);
        m_cubemapTexture = 0;
    }
    if (m_shader) {
        delete m_shader;
        m_shader = nullptr;
    }
    m_initialized = false;
}