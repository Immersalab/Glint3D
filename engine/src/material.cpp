#include "material.h"
#include "gl_platform.h"
#include <glm/gtc/type_ptr.hpp>


Material::Material(
    const glm::vec3& diffuse,
    const glm::vec3& specular,
    const glm::vec3& ambient,
    float shininess,
    float roughness,
    float metallic
)
    : diffuse(diffuse)
    , specular(specular)
    , ambient(ambient)
    , shininess(shininess)
    , roughness(roughness)
    , metallic(metallic)
{
    // constructor body can be empty if no extra logic is needed
}

void Material::apply(GLuint shaderProgram, const std::string& uniformName) const {
    GLint locDiffuse = glGetUniformLocation(shaderProgram, (uniformName + ".diffuse").c_str());
    GLint locSpecular = glGetUniformLocation(shaderProgram, (uniformName + ".specular").c_str());
    GLint locAmbient = glGetUniformLocation(shaderProgram, (uniformName + ".ambient").c_str());
    GLint locShine = glGetUniformLocation(shaderProgram, (uniformName + ".shininess").c_str());
    // etc.

    glUniform3fv(locDiffuse, 1, glm::value_ptr(diffuse));
    glUniform3fv(locSpecular, 1, glm::value_ptr(specular));
    glUniform3fv(locAmbient, 1, glm::value_ptr(ambient));
    glUniform1f(locShine, shininess);
}
