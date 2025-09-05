#include "camera_controller.h"
#include "scene_manager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <limits>
#include <cmath>

CameraController::CameraController()
{
    updateVectors();
}

void CameraController::update(float deltaTime)
{
    // This can be used for smooth camera movements, momentum, etc.
    // Currently just updates the vectors to ensure consistency
    updateVectors();
}

void CameraController::moveForward(float distance)
{
    m_camera.position += m_camera.front * distance;
}

void CameraController::moveBackward(float distance)
{
    m_camera.position -= m_camera.front * distance;
}

void CameraController::moveLeft(float distance)
{
    glm::vec3 right = glm::normalize(glm::cross(m_camera.front, m_camera.up));
    m_camera.position -= right * distance;
}

void CameraController::moveRight(float distance)
{
    glm::vec3 right = glm::normalize(glm::cross(m_camera.front, m_camera.up));
    m_camera.position += right * distance;
}

void CameraController::moveUp(float distance)
{
    m_camera.position += m_camera.up * distance;
}

void CameraController::moveDown(float distance)
{
    m_camera.position -= m_camera.up * distance;
}

void CameraController::rotate(float deltaYaw, float deltaPitch)
{
    m_camera.yaw += deltaYaw;
    m_camera.pitch += deltaPitch;
    
    // Constrain pitch to avoid gimbal lock
    m_camera.pitch = std::clamp(m_camera.pitch, -89.0f, 89.0f);
    
    updateVectors();
}

void CameraController::setAngles(float yaw, float pitch)
{
    m_camera.yaw = yaw;
    m_camera.pitch = std::clamp(pitch, -89.0f, 89.0f);
    updateVectors();
}

void CameraController::setTarget(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
{
    m_camera.position = position;
    m_camera.up = up;
    
    glm::vec3 direction = glm::normalize(target - position);
    m_camera.front = direction;
    
    // Calculate yaw and pitch from direction vector
    m_camera.yaw = glm::degrees(atan2(direction.z, direction.x));
    // Positive pitch looks upward; compute consistently with updateVectors()
    m_camera.pitch = glm::degrees(asin(direction.y));
    
    updateVectors();
}

void CameraController::setFrontUp(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up)
{
    m_camera.position = position;
    m_camera.front = glm::normalize(front);
    m_camera.up = glm::normalize(up);
    
    // Calculate yaw and pitch from front vector
    m_camera.yaw = glm::degrees(atan2(m_camera.front.z, m_camera.front.x));
    m_camera.pitch = glm::degrees(asin(m_camera.front.y));
}

void CameraController::setLens(float fovDeg, float nearZ, float farZ)
{
    m_camera.fov = fovDeg;
    m_camera.nearClip = nearZ;
    m_camera.farClip = farZ;
}

void CameraController::updateVectors()
{
    // Calculate the new front vector from yaw and pitch
    glm::vec3 front;
    front.x = cos(glm::radians(m_camera.yaw)) * cos(glm::radians(m_camera.pitch));
    front.y = sin(glm::radians(m_camera.pitch));
    front.z = sin(glm::radians(m_camera.yaw)) * cos(glm::radians(m_camera.pitch));
    m_camera.front = glm::normalize(front);
    
    // Re-calculate the right and up vectors
    glm::vec3 right = glm::normalize(glm::cross(m_camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_camera.up = glm::normalize(glm::cross(right, m_camera.front));
}

void CameraController::setCameraPreset(CameraPreset preset, const SceneManager& scene,
                                      const glm::vec3& customTarget, float fov, float margin)
{
    // Calculate scene bounding sphere
    glm::vec3 center(0.0f);
    float radius = 5.0f; // Default fallback radius
    
    // Calculate bounding box of all scene objects
    const auto& objects = scene.getObjects();
    if (!objects.empty()) {
        glm::vec3 minBounds(std::numeric_limits<float>::max());
        glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
        
        bool hasValidBounds = false;
        
        for (const auto& obj : objects) {
            // Transform object bounds to world space
            glm::vec3 objMin = obj.objLoader.getMinBounds();
            glm::vec3 objMax = obj.objLoader.getMaxBounds();
            
            // Apply object transform to bounds (simplified - assumes uniform scaling)
            glm::vec3 objCenter = (objMin + objMax) * 0.5f;
            glm::vec3 objSize = objMax - objMin;
            
            // Transform center
            glm::vec4 worldCenter = obj.modelMatrix * glm::vec4(objCenter, 1.0f);
            
            // Approximate transformed size (works for uniform scaling)
            glm::vec3 scale = glm::vec3(
                glm::length(glm::vec3(obj.modelMatrix[0])),
                glm::length(glm::vec3(obj.modelMatrix[1])),
                glm::length(glm::vec3(obj.modelMatrix[2]))
            );
            glm::vec3 worldSize = objSize * scale;
            
            glm::vec3 worldMin = glm::vec3(worldCenter) - worldSize * 0.5f;
            glm::vec3 worldMax = glm::vec3(worldCenter) + worldSize * 0.5f;
            
            if (hasValidBounds) {
                minBounds = glm::min(minBounds, worldMin);
                maxBounds = glm::max(maxBounds, worldMax);
            } else {
                minBounds = worldMin;
                maxBounds = worldMax;
                hasValidBounds = true;
            }
        }
        
        if (hasValidBounds) {
            center = (minBounds + maxBounds) * 0.5f;
            glm::vec3 size = maxBounds - minBounds;
            radius = glm::length(size) * 0.5f; // Bounding sphere radius
        }
    }
    
    // Use custom target if provided (not zero vector)
    if (glm::length(customTarget) > 0.001f) {
        center = customTarget;
    }

    // Compute required camera distance to frame bounding sphere given vertical FOV
    const float fovRad = glm::radians(fov);
    const float dist = (radius * (1.0f + margin)) / std::max(0.0001f, std::tan(fovRad * 0.5f));

    // Calculate camera position based on preset
    glm::vec3 dirFromCenter(0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f); // Default up vector

    switch (preset) {
        case CameraPreset::Front:   dirFromCenter = glm::vec3(0.0f, 0.0f,  1.0f); break;
        case CameraPreset::Back:    dirFromCenter = glm::vec3(0.0f, 0.0f, -1.0f); break;
        case CameraPreset::Left:    dirFromCenter = glm::vec3(-1.0f, 0.0f, 0.0f); break;
        case CameraPreset::Right:   dirFromCenter = glm::vec3( 1.0f, 0.0f, 0.0f); break;
        case CameraPreset::Top:
            dirFromCenter = glm::vec3(0.0f, 1.0f, 0.0f);
            up = glm::vec3(0.0f, 0.0f, -1.0f); // Roll so +Z is up on screen
            break;
        case CameraPreset::Bottom:
            dirFromCenter = glm::vec3(0.0f, -1.0f, 0.0f);
            up = glm::vec3(0.0f, 0.0f,  1.0f); // Roll so +Z is up on screen
            break;
        case CameraPreset::IsoFL: // Isometric Front-Left (from +Y looking toward origin)
            dirFromCenter = glm::normalize(glm::vec3(-1.0f, 1.0f,  1.0f));
            break;
        case CameraPreset::IsoBR: // Isometric Back-Right
            dirFromCenter = glm::normalize(glm::vec3( 1.0f, 1.0f, -1.0f));
            break;
    }

    const glm::vec3 position = center + dirFromCenter * dist;

    // Set camera state
    setTarget(position, center, up);
    setLens(fov, m_camera.nearClip, m_camera.farClip);
}

const char* CameraController::presetName(CameraPreset preset)
{
    switch (preset) {
        case CameraPreset::Front: return "Front";
        case CameraPreset::Back: return "Back";
        case CameraPreset::Left: return "Left";
        case CameraPreset::Right: return "Right";
        case CameraPreset::Top: return "Top";
        case CameraPreset::Bottom: return "Bottom";
        case CameraPreset::IsoFL: return "Iso Front-Left";
        case CameraPreset::IsoBR: return "Iso Back-Right";
        default: return "Unknown";
    }
}

CameraPreset CameraController::presetFromHotkey(int key)
{
    switch (key) {
        case 1: return CameraPreset::Front;
        case 2: return CameraPreset::Back;
        case 3: return CameraPreset::Left;
        case 4: return CameraPreset::Right;
        case 5: return CameraPreset::Top;
        case 6: return CameraPreset::Bottom;
        case 7: return CameraPreset::IsoFL;
        case 8: return CameraPreset::IsoBR;
        default: return CameraPreset::Front; // Default fallback
    }
}
