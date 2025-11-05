#pragma once
#include <glm/glm.hpp>
#include "render_system.h"
#include "config_defaults.h"

// Forward declarations
class SceneManager;

enum class CameraPreset {
    Front = 0,  // Hotkey 1
    Back,       // Hotkey 2  
    Left,       // Hotkey 3
    Right,      // Hotkey 4
    Top,        // Hotkey 5
    Bottom,     // Hotkey 6
    IsoFL,      // Isometric Front-Left, Hotkey 7
    IsoBR       // Isometric Back-Right, Hotkey 8
};

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
    
    // Orbit camera functionality
    void orbitAroundTarget(float deltaYaw, float deltaPitch, const glm::vec3& target);
    void setOrbitTarget(const glm::vec3& target);
    void setOrbitDamping(float damping) { m_orbitDamping = damping; }
    void setOrbitDistance(float distance) { m_orbitDistance = distance; }
    float getOrbitDamping() const { return m_orbitDamping; }
    float getOrbitDistance() const { return m_orbitDistance; }
    const glm::vec3& getOrbitTarget() const { return m_orbitTarget; }
    
    // Settings
    void setSpeed(float speed) { m_speed = speed; }
    float getSpeed() const { return m_speed; }
    
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    float getSensitivity() const { return m_sensitivity; }
    
    // State access
    void setCameraState(const CameraState& state) { m_camera = state; }
    const CameraState& getCameraState() const { return m_camera; }
    CameraState& getCameraState() { return m_camera; }
    
    // Camera presets
    void setCameraPreset(CameraPreset preset, const SceneManager& scene, 
                        const glm::vec3& customTarget = glm::vec3(0.0f),
                        float fov = Defaults::CameraPresetFovDeg,
                        float margin = Defaults::CameraPresetMargin);
    
    // Static utility functions
    static const char* presetName(CameraPreset preset);
    static CameraPreset presetFromHotkey(int key); // 1-8 keys

private:
    CameraState m_camera;
    float m_speed = 0.5f;
    float m_sensitivity = 0.1f;
    
    // Orbit camera state
    glm::vec3 m_orbitTarget{0.0f, 0.0f, 0.0f};
    float m_orbitDistance = 5.0f;
    float m_orbitDamping = 0.85f; // 0.0 = no damping, 1.0 = instant
    float m_targetYaw = 0.0f;
    float m_targetPitch = 0.0f;
    float m_yawVelocity = 0.0f;
    float m_pitchVelocity = 0.0f;
    
    void updateVectors();
    void updateOrbitPosition();
};
