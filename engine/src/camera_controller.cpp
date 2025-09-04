#include "camera_controller.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

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
    m_camera.pitch = glm::degrees(asin(-direction.y));
    
    updateVectors();
}

void CameraController::setFrontUp(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up)
{
    m_camera.position = position;
    m_camera.front = glm::normalize(front);
    m_camera.up = glm::normalize(up);
    
    // Calculate yaw and pitch from front vector
    m_camera.yaw = glm::degrees(atan2(m_camera.front.z, m_camera.front.x));
    m_camera.pitch = glm::degrees(asin(-m_camera.front.y));
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