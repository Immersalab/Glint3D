#include "light.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

std::string intToString(int value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

Light::Light()
{
}

Light::~Light()
{
    if (m_indicatorVAO) glDeleteVertexArrays(1, &m_indicatorVAO);
    if (m_indicatorVBO) glDeleteBuffers(1, &m_indicatorVBO);
    if (m_arrowVAO) glDeleteVertexArrays(1, &m_arrowVAO);
    if (m_arrowVBO) glDeleteBuffers(1, &m_arrowVBO);
    if (m_spotVAO) glDeleteVertexArrays(1, &m_spotVAO);
    if (m_spotVBO) glDeleteBuffers(1, &m_spotVBO);
    if (m_indicatorShader) glDeleteProgram(m_indicatorShader);
}

void Light::addLight(const glm::vec3& position, const glm::vec3& color, float intensity)
{
    LightSource newLight;
    newLight.type = LightType::POINT;
    newLight.position = position;
    newLight.color = color;
    newLight.intensity = intensity;
    newLight.enabled = true; // default on
    m_lights.push_back(newLight);
}

void Light::addDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
{
    LightSource newLight;
    newLight.type = LightType::DIRECTIONAL;
    newLight.direction = glm::normalize(direction);
    newLight.position = glm::vec3(0.0f); // Not used for directional lights
    newLight.color = color;
    newLight.intensity = intensity;
    newLight.enabled = true;
    m_lights.push_back(newLight);
}

void Light::addSpotLight(const glm::vec3& position, const glm::vec3& direction,
                         const glm::vec3& color, float intensity,
                         float innerConeDeg, float outerConeDeg)
{
    LightSource s;
    s.type = LightType::SPOT;
    s.position = position;
    s.direction = glm::length(direction) > 1e-4f ? glm::normalize(direction) : glm::vec3(0, -1, 0);
    s.color = color;
    s.intensity = intensity;
    s.enabled = true;
    s.innerConeDeg = innerConeDeg;
    s.outerConeDeg = outerConeDeg;
    if (s.outerConeDeg < s.innerConeDeg) std::swap(s.outerConeDeg, s.innerConeDeg);
    m_lights.push_back(s);
}

void Light::applyLights(GLuint shaderProgram) const
{
    glUseProgram(shaderProgram);

    // Send the global ambient
    GLint ambientLoc = glGetUniformLocation(shaderProgram, "globalAmbient");
    if (ambientLoc != -1)
    {
        glUniform4fv(ambientLoc, 1, &m_globalAmbient[0]);
    }

    GLint lightCountLoc = glGetUniformLocation(shaderProgram, "numLights");
    if (lightCountLoc != -1)
    {
        glUniform1i(lightCountLoc, static_cast<int>(m_lights.size()));
    }

    for (size_t i = 0; i < m_lights.size(); i++)
    {
        std::string baseName = "lights[" + std::to_string(i) + "]";
        GLint lightTypeLoc = glGetUniformLocation(shaderProgram, (baseName + ".type").c_str());
        GLint lightPosLoc = glGetUniformLocation(shaderProgram, (baseName + ".position").c_str());
        GLint lightDirLoc = glGetUniformLocation(shaderProgram, (baseName + ".direction").c_str());
        GLint lightColorLoc = glGetUniformLocation(shaderProgram, (baseName + ".color").c_str());
        GLint lightIntensityLoc = glGetUniformLocation(shaderProgram, (baseName + ".intensity").c_str());
        GLint innerLoc = glGetUniformLocation(shaderProgram, (baseName + ".innerCutoff").c_str());
        GLint outerLoc = glGetUniformLocation(shaderProgram, (baseName + ".outerCutoff").c_str());

        float intensity = (m_lights[i].enabled) ? m_lights[i].intensity : 0.0f;

        if (lightTypeLoc != -1)      glUniform1i(lightTypeLoc, static_cast<int>(m_lights[i].type));
        if (lightPosLoc != -1)       glUniform3fv(lightPosLoc, 1, glm::value_ptr(m_lights[i].position));
        if (lightDirLoc != -1)       glUniform3fv(lightDirLoc, 1, glm::value_ptr(m_lights[i].direction));
        if (lightColorLoc != -1)     glUniform3fv(lightColorLoc, 1, glm::value_ptr(m_lights[i].color));
        if (lightIntensityLoc != -1) glUniform1f(lightIntensityLoc, intensity);
        // Spot cone angles (pass cosines of radians)
        if (innerLoc != -1) {
            float innerRad = glm::radians(m_lights[i].innerConeDeg);
            glUniform1f(innerLoc, cosf(innerRad));
        }
        if (outerLoc != -1) {
            float outerRad = glm::radians(m_lights[i].outerConeDeg);
            glUniform1f(outerLoc, cosf(outerRad));
        }
    }
}


size_t Light::getLightCount() const
{
    return m_lights.size();
}

// Initialize indicator geometry (cube for point lights, arrow for directional lights, cone for spot lights)
void Light::initIndicator()
{
    // Cube vertices for point lights
    float cubeVertices[] = {
        // positions for 12 triangles (36 vertices)
        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,

        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,

         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,

        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f
    };

    // Initialize cube geometry for point lights
    glGenVertexArrays(1, &m_indicatorVAO);
    glGenBuffers(1, &m_indicatorVBO);
    glBindVertexArray(m_indicatorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_indicatorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Arrow vertices for directional lights (shaft + arrowhead)
    float arrowVertices[] = {
        // Arrow shaft (thin cylinder approximated with lines)
        0.0f, 0.0f, 0.0f,    0.0f, 0.0f, -0.8f,  // Shaft line
        0.0f, 0.0f, -0.8f,   0.2f, 0.0f, -0.6f,  // Right arrowhead line
        0.0f, 0.0f, -0.8f,  -0.2f, 0.0f, -0.6f,  // Left arrowhead line
        0.0f, 0.0f, -0.8f,   0.0f, 0.2f, -0.6f,  // Top arrowhead line
        0.0f, 0.0f, -0.8f,   0.0f, -0.2f, -0.6f, // Bottom arrowhead line
    };

    // Initialize arrow geometry for directional lights
    glGenVertexArrays(1, &m_arrowVAO);
    glGenBuffers(1, &m_arrowVBO);
    glBindVertexArray(m_arrowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_arrowVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(arrowVertices), arrowVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Spot light cone outline (unit, pointing down -Z)
    {
        std::vector<float> verts;
        auto push = [&](float x, float y, float z){ verts.push_back(x); verts.push_back(y); verts.push_back(z); };
        // Axis line
        push(0,0,0); push(0,0,-0.9f);
        // Circle at z = -0.9
        const int N = 24;
        const float r = 0.3f;
        for (int i=0;i<N;i++) {
            float a0 = (float)i * 6.2831853f / N;
            float a1 = (float)(i+1) * 6.2831853f / N;
            push(r*cosf(a0), r*sinf(a0), -0.9f); push(r*cosf(a1), r*sinf(a1), -0.9f);
        }
        // Spokes from origin to circle (every 90deg)
        for (int i=0;i<4;i++) {
            float a = (float)i * 6.2831853f / 4;
            push(0,0,0); push(r*cosf(a), r*sinf(a), -0.9f);
        }

        glGenVertexArrays(1, &m_spotVAO);
        glGenBuffers(1, &m_spotVBO);
        glBindVertexArray(m_spotVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_spotVBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
}

// Initialize the indicator shader within the Light class
bool Light::initIndicatorShader()
{
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 indicatorColor;
        void main()
        {
            FragColor = vec4(indicatorColor, 1.0);
        }
    )";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Indicator Vertex Shader Compilation Error:\n" << infoLog << std::endl;
        return false;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Indicator Fragment Shader Compilation Error:\n" << infoLog << std::endl;
        return false;
    }

    // Link shader program
    m_indicatorShader = glCreateProgram();
    glAttachShader(m_indicatorShader, vertexShader);
    glAttachShader(m_indicatorShader, fragmentShader);
    glLinkProgram(m_indicatorShader);
    glGetProgramiv(m_indicatorShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_indicatorShader, 512, nullptr, infoLog);
        std::cerr << "Indicator Shader Linking Error:\n" << infoLog << std::endl;
        return false;
    }

    // Clean up shaders (no longer needed once linked)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

// Render visual indicators for each light using the indicator shader stored in the Light class
void Light::renderIndicators(const glm::mat4& view, const glm::mat4& projection, int selectedIndex) const
{
    if (m_indicatorShader == 0)
        return; // Shader not initialized

    glUseProgram(m_indicatorShader);

    GLint viewLoc = glGetUniformLocation(m_indicatorShader, "view");
    GLint projLoc = glGetUniformLocation(m_indicatorShader, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    GLint modelLoc = glGetUniformLocation(m_indicatorShader, "model");
    GLint colorLoc = glGetUniformLocation(m_indicatorShader, "indicatorColor");

    for (size_t i = 0; i < m_lights.size(); i++) {
        glm::mat4 model(1.0f);
        
        if (m_lights[i].type == LightType::POINT) {
            // Render cube at light position
            model = glm::translate(model, m_lights[i].position);
            glBindVertexArray(m_indicatorVAO);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(m_lights[i].color));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        } 
        else if (m_lights[i].type == LightType::DIRECTIONAL) {
            // Calculate transformation to orient arrow in light direction
            glm::vec3 forward(0.0f, 0.0f, -1.0f); // Arrow points down -Z
            glm::vec3 lightDir = glm::normalize(m_lights[i].direction);
            
            // Create rotation matrix to align arrow with light direction
            glm::vec3 axis = glm::cross(forward, lightDir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = acos(glm::clamp(glm::dot(forward, lightDir), -1.0f, 1.0f));
                model = glm::rotate(model, angle, axis);
            } else if (glm::dot(forward, lightDir) < 0) {
                // 180 degree rotation needed
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }
            
            // Position arrow at scene origin for directional lights (they're infinite)
            // In the future, we could position them at the camera or scene bounds
            
            glBindVertexArray(m_arrowVAO);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(m_lights[i].color));
            glLineWidth(3.0f);
            glDrawArrays(GL_LINES, 0, 10); // 5 lines * 2 vertices = 10
        }
        else if (m_lights[i].type == LightType::SPOT) {
            // Translate to light position, orient cone along light direction
            model = glm::translate(model, m_lights[i].position);
            glm::vec3 forward(0.0f, 0.0f, -1.0f);
            glm::vec3 lightDir = glm::normalize(m_lights[i].direction);
            glm::vec3 axis = glm::cross(forward, lightDir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = acos(glm::clamp(glm::dot(forward, lightDir), -1.0f, 1.0f));
                model = glm::rotate(model, angle, axis);
            } else if (glm::dot(forward, lightDir) < 0) {
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }
            glBindVertexArray(m_spotVAO);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(m_lights[i].color));
            glLineWidth(2.0f);
            // Number of vertices: 2 (axis) + 2*N (circle) + 2*4 (spokes) where N=24 -> 2 + 48 + 8 = 58
            glDrawArrays(GL_LINES, 0, 58);
        }
    }
    
    // Draw selection highlight
    if (selectedIndex >= 0 && selectedIndex < (int)m_lights.size()) {
        glm::vec3 selColor(0.2f, 0.7f, 1.0f);
        glUniform3fv(colorLoc, 1, glm::value_ptr(selColor));
        
        if (m_lights[selectedIndex].type == LightType::POINT) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_lights[selectedIndex].position) 
                            * glm::scale(glm::mat4(1.0f), glm::vec3(1.3f));
            glBindVertexArray(m_indicatorVAO);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            GLint polyMode[2];
            glGetIntegerv(GL_POLYGON_MODE, polyMode);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glPolygonMode(GL_FRONT_AND_BACK, polyMode[0]);
        } else if (m_lights[selectedIndex].type == LightType::DIRECTIONAL) {
            // Highlight directional light with thicker lines
            glm::mat4 model(1.0f);
            glm::vec3 forward(0.0f, 0.0f, -1.0f);
            glm::vec3 lightDir = glm::normalize(m_lights[selectedIndex].direction);
            
            glm::vec3 axis = glm::cross(forward, lightDir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = acos(glm::clamp(glm::dot(forward, lightDir), -1.0f, 1.0f));
                model = glm::rotate(model, angle, axis);
            } else if (glm::dot(forward, lightDir) < 0) {
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }
            
            glBindVertexArray(m_arrowVAO);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glLineWidth(5.0f);
            glDrawArrays(GL_LINES, 0, 10);
        } else if (m_lights[selectedIndex].type == LightType::SPOT) {
            glm::mat4 model(1.0f);
            model = glm::translate(model, m_lights[selectedIndex].position);
            glm::vec3 forward(0.0f, 0.0f, -1.0f);
            glm::vec3 lightDir = glm::normalize(m_lights[selectedIndex].direction);
            glm::vec3 axis = glm::cross(forward, lightDir);
            if (glm::length(axis) > 0.001f) {
                axis = glm::normalize(axis);
                float angle = acos(glm::clamp(glm::dot(forward, lightDir), -1.0f, 1.0f));
                model = glm::rotate(model, angle, axis);
            } else if (glm::dot(forward, lightDir) < 0) {
                model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
            }
            glBindVertexArray(m_spotVAO);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glLineWidth(4.0f);
            glDrawArrays(GL_LINES, 0, 58);
        }
    }
    
    glBindVertexArray(0);
}

const LightSource* Light::getFirstLight() const
{
    if (m_lights.empty())
        return nullptr;
    return &m_lights[0];
}

bool Light::removeLightAt(size_t index)
{
    if (index >= m_lights.size()) return false;
    m_lights.erase(m_lights.begin() + index);
    return true;
}
