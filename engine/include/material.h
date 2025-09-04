#pragma once

#include <string>
#include "gl_platform.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "colors.h"

class Material {
public:
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 ambient;
    float shininess;
    float roughness;
    float metallic;

    Material(
        const glm::vec3& diffuse = Colors::White,
        const glm::vec3& specular = Colors::White,
        const glm::vec3& ambient = Colors::Gray,
        float shininess = 32.0f,
        float roughness = 0.5f,
        float metallic = 0.0f
    );

    void apply(GLuint shaderProgram, const std::string& uniformName = "material") const;
};
