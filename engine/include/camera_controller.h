#pragma once
#include <glm/glm.hpp>
#include "render_system.h"

class CameraController 
{
public:
    CameraController();
    ~CameraController() = default;

    void update(float deltaTime);
    
    // Movement
    void moveForward(float distance);
    void moveBackward(float distance);
    void moveLeft(float distance);
    void moveRight(float distance);
    void moveUp(float distance);
    void moveDown(float distance);
    
    // Rotation
    void rotate(float deltaYaw, float deltaPitch);
    void setAngles(float yaw, float pitch);
    
    // Target/Look-at
    void setTarget(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);
    void setFrontUp(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up);
    void setLens(float fovDeg, float nearZ, float farZ);
    
    // Settings
    void setSpeed(float speed) { m_speed = speed; }
    float getSpeed() const { return m_speed; }
    
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    float getSensitivity() const { return m_sensitivity; }
    
    // State access
    void setCameraState(const CameraState& state) { m_camera = state; }
    const CameraState& getCameraState() const { return m_camera; }
    CameraState& getCameraState() { return m_camera; }

private:
    CameraState m_camera;
    float m_speed = 0.5f;
    float m_sensitivity = 0.1f;
    
    void updateVectors();
};