#include "scene_manager.h"
#include "gl_platform.h"
#include "mesh_loader.h"
#include "texture_cache.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

SceneManager::SceneManager() 
{
}

SceneManager::~SceneManager() 
{
    clear();
}

bool SceneManager::loadObject(const std::string& name, const std::string& path, 
                             const glm::vec3& position, const glm::vec3& scale)
{
    // Check if object with this name already exists
    if (findObjectByName(name) != nullptr) {
        std::cerr << "Object with name '" << name << "' already exists\n";
        return false;
    }

    SceneObject obj;
    obj.name = name;
    
    // Load mesh data
    obj.objLoader.load(path.c_str());

    // Set transform
    glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
    obj.modelMatrix = translateMat * scaleMat;

    // Setup OpenGL resources
    setupObjectOpenGL(obj);

    // Load textures if they exist
    std::string directory = path.substr(0, path.find_last_of('/'));
    if (directory == path) directory = "."; // No directory found
    
    // Try to find and load associated textures
    TextureCache& texCache = TextureCache::instance();
    
    // Look for common texture naming patterns
    std::string baseName = path.substr(path.find_last_of('/') + 1);
    baseName = baseName.substr(0, baseName.find_last_of('.'));
    
    // Try diffuse/albedo texture
    std::vector<std::string> diffuseNames = {
        directory + "/" + baseName + "_diffuse.png",
        directory + "/" + baseName + "_albedo.png", 
        directory + "/" + baseName + "_basecolor.png",
        directory + "/" + baseName + ".png",
        directory + "/" + baseName + ".jpg"
    };
    
    for (const auto& texPath : diffuseNames) {
        if (std::ifstream(texPath).good()) {
            obj.baseColorTex = texCache.get(texPath, false);
            obj.texture = obj.baseColorTex; // legacy fallback
            break;
        }
    }
    
    m_objects.push_back(std::move(obj));
    return true;
}

bool SceneManager::removeObject(const std::string& name)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const SceneObject& obj) { return obj.name == name; });
    
    if (it == m_objects.end()) {
        return false;
    }
    
    cleanupObjectOpenGL(*it);
    
    // Update selection if removing selected object
    int index = std::distance(m_objects.begin(), it);
    if (m_selectedObjectIndex == index) {
        m_selectedObjectIndex = -1;
    } else if (m_selectedObjectIndex > index) {
        m_selectedObjectIndex--;
    }
    
    m_objects.erase(it);
    return true;
}

bool SceneManager::duplicateObject(const std::string& sourceName, const std::string& newName,
                                  const glm::vec3* deltaPos, const glm::vec3* deltaScale, 
                                  const glm::vec3* deltaRotDeg)
{
    const SceneObject* source = findObjectByName(sourceName);
    if (!source) {
        return false;
    }
    
    if (findObjectByName(newName) != nullptr) {
        std::cerr << "Object with name '" << newName << "' already exists\n";
        return false;
    }
    
    SceneObject newObj = *source; // Copy construct
    newObj.name = newName;
    
    // Apply deltas to transform
    if (deltaPos || deltaScale || deltaRotDeg) {
        glm::mat4 transform = newObj.modelMatrix;
        
        if (deltaPos) {
            transform = glm::translate(transform, *deltaPos);
        }
        
        if (deltaRotDeg) {
            transform = glm::rotate(transform, glm::radians(deltaRotDeg->x), glm::vec3(1,0,0));
            transform = glm::rotate(transform, glm::radians(deltaRotDeg->y), glm::vec3(0,1,0)); 
            transform = glm::rotate(transform, glm::radians(deltaRotDeg->z), glm::vec3(0,0,1));
        }
        
        if (deltaScale) {
            transform = glm::scale(transform, *deltaScale);
        }
        
        newObj.modelMatrix = transform;
    }
    
    // Setup new OpenGL resources (don't share VAO/VBO)
    setupObjectOpenGL(newObj);
    
    m_objects.push_back(std::move(newObj));
    return true;
}

bool SceneManager::moveObject(const std::string& name, const glm::vec3& delta)
{
    SceneObject* obj = findObjectByName(name);
    if (!obj) {
        return false;
    }
    
    obj->modelMatrix = glm::translate(obj->modelMatrix, delta);
    return true;
}

std::string SceneManager::getSelectedObjectName() const
{
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_objects.size()) {
        return m_objects[m_selectedObjectIndex].name;
    }
    return "";
}

glm::vec3 SceneManager::getSelectedObjectCenterWorld() const
{
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_objects.size()) {
        const SceneObject& obj = m_objects[m_selectedObjectIndex];
        // Extract translation from model matrix
        return glm::vec3(obj.modelMatrix[3]);
    }
    return glm::vec3(0.0f);
}

bool SceneManager::createMaterial(const std::string& name, const Material& material)
{
    m_materials[name] = material;
    return true;
}

bool SceneManager::assignMaterialToObject(const std::string& objectName, const std::string& materialName)
{
    SceneObject* obj = findObjectByName(objectName);
    if (!obj) {
        return false;
    }
    
    auto it = m_materials.find(materialName);
    if (it == m_materials.end()) {
        return false;
    }
    
    obj->material = it->second;
    return true;
}

SceneObject* SceneManager::findObjectByName(const std::string& name)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const SceneObject& obj) { return obj.name == name; });
    return (it != m_objects.end()) ? &(*it) : nullptr;
}

const SceneObject* SceneManager::findObjectByName(const std::string& name) const
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const SceneObject& obj) { return obj.name == name; });
    return (it != m_objects.end()) ? &(*it) : nullptr;
}

void SceneManager::clear()
{
    for (auto& obj : m_objects) {
        cleanupObjectOpenGL(obj);
    }
    m_objects.clear();
    m_materials.clear();
    m_selectedObjectIndex = -1;
}

void SceneManager::setupObjectOpenGL(SceneObject& obj)
{
    if (obj.objLoader.getVertCount() == 0) {
        return;
    }

    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO_positions);
    glGenBuffers(1, &obj.VBO_normals);
    glGenBuffers(1, &obj.VBO_uvs);
    if (obj.objLoader.getIndexCount() > 0) {
        glGenBuffers(1, &obj.EBO);
    }

    glBindVertexArray(obj.VAO);

    // Positions
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, obj.objLoader.getVertCount() * 3 * sizeof(float), 
                 obj.objLoader.getPositions(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Normals
    if (obj.objLoader.getNormals()) {
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_normals);
        glBufferData(GL_ARRAY_BUFFER, obj.objLoader.getVertCount() * 3 * sizeof(float),
                     obj.objLoader.getNormals(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);
    }

    // UVs
    if (obj.objLoader.hasTexcoords()) {
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_uvs);
        glBufferData(GL_ARRAY_BUFFER, obj.objLoader.getVertCount() * 2 * sizeof(float),
                     obj.objLoader.getTexcoords(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);
    }

    // Indices
    if (obj.objLoader.getIndexCount() > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.objLoader.getIndexCount() * sizeof(unsigned int),
                     obj.objLoader.getFaces(), GL_STATIC_DRAW);
    }

    glBindVertexArray(0);
}

void SceneManager::cleanupObjectOpenGL(SceneObject& obj)
{
    if (obj.VAO) {
        glDeleteVertexArrays(1, &obj.VAO);
        obj.VAO = 0;
    }
    if (obj.VBO_positions) {
        glDeleteBuffers(1, &obj.VBO_positions);
        obj.VBO_positions = 0;
    }
    if (obj.VBO_normals) {
        glDeleteBuffers(1, &obj.VBO_normals);
        obj.VBO_normals = 0;
    }
    if (obj.VBO_uvs) {
        glDeleteBuffers(1, &obj.VBO_uvs);
        obj.VBO_uvs = 0;
    }
    if (obj.VBO_tangents) {
        glDeleteBuffers(1, &obj.VBO_tangents);
        obj.VBO_tangents = 0;
    }
    if (obj.EBO) {
        glDeleteBuffers(1, &obj.EBO);
        obj.EBO = 0;
    }
}

std::string SceneManager::toJson() const
{
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"objects\": [\n";
    
    for (size_t i = 0; i < m_objects.size(); ++i) {
        const auto& obj = m_objects[i];
        oss << "    {\n";
        oss << "      \"name\": \"" << obj.name << "\",\n";
        oss << "      \"transform\": {\n";
        
        // Extract position from model matrix
        glm::vec3 pos = glm::vec3(obj.modelMatrix[3]);
        oss << "        \"position\": [" << pos.x << "," << pos.y << "," << pos.z << "]\n";
        
        oss << "      }\n";
        oss << "    }";
        if (i < m_objects.size() - 1) oss << ",";
        oss << "\n";
    }
    
    oss << "  ]\n";
    oss << "}";
    return oss.str();
}