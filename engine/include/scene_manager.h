#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "material.h"
#include "light.h"
#include "objloader.h"
#include "Texture.h"
#include "shader.h"

struct SceneObject
{
    std::string name;
    GLuint   VAO = 0, VBO_positions = 0, VBO_normals = 0, VBO_uvs = 0, VBO_tangents = 0, EBO = 0;
    glm::mat4 modelMatrix{ 1.0f };

    ObjLoader objLoader;
    Texture* texture = nullptr;       // legacy diffuse
    Texture* baseColorTex = nullptr;  // PBR
    Texture* normalTex = nullptr;     // PBR
    Texture* mrTex = nullptr;         // PBR (metallic-roughness)
    Shader* shader = nullptr;

    bool      isStatic = false;
    glm::vec3 color{ 1.0f };
    Material  material;
    // Basic PBR factors
    glm::vec4 baseColorFactor{1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
};

class SceneManager 
{
public:
    SceneManager();
    ~SceneManager();

    // Object management
    bool loadObject(const std::string& name, const std::string& path, 
                   const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f));
    bool removeObject(const std::string& name);
    bool duplicateObject(const std::string& sourceName, const std::string& newName,
                        const glm::vec3* deltaPos = nullptr,
                        const glm::vec3* deltaScale = nullptr, 
                        const glm::vec3* deltaRotDeg = nullptr);
    bool moveObject(const std::string& name, const glm::vec3& delta);
    
    // Selection
    void setSelectedObjectIndex(int index) { m_selectedObjectIndex = index; }
    int getSelectedObjectIndex() const { return m_selectedObjectIndex; }
    std::string getSelectedObjectName() const;
    glm::vec3 getSelectedObjectCenterWorld() const;

    // Material management  
    bool createMaterial(const std::string& name, const Material& material);
    bool assignMaterialToObject(const std::string& objectName, const std::string& materialName);

    // Accessors
    const std::vector<SceneObject>& getObjects() const { return m_objects; }
    std::vector<SceneObject>& getObjects() { return m_objects; }
    SceneObject* findObjectByName(const std::string& name);
    const SceneObject* findObjectByName(const std::string& name) const;

    // Serialization
    std::string toJson() const;
    bool fromJson(const std::string& json);

    // Clear scene
    void clear();

private:
    std::vector<SceneObject> m_objects;
    std::unordered_map<std::string, Material> m_materials;
    int m_selectedObjectIndex = -1;

    void setupObjectOpenGL(SceneObject& obj);
    void cleanupObjectOpenGL(SceneObject& obj);
};