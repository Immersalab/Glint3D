#pragma once
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "gizmo.h"
#include "render_settings.h"

// Forward declarations
struct GLFWwindow;
class SceneManager;
class RenderSystem;
class CameraController;
class Light;
class UIBridge;
class UserInput;
class JsonOpsExecutor;

// Streamlined Application class focused on coordination
class ApplicationCore 
{
public:
    ApplicationCore();
    ~ApplicationCore();

    bool init(const std::string& windowTitle, int width, int height, bool headless = false);
    void run();
    void frame(); // Single frame for Emscripten
    void shutdown();

    // Public API for external usage (main.cpp, JSON Ops, etc.)
    bool loadObject(const std::string& name, const std::string& path, 
                   const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f));
    bool renderToPNG(const std::string& path, int width, int height);
    bool applyJsonOpsV1(const std::string& json, std::string& error);
    std::string buildShareLink() const;
    std::string sceneToJson() const;
    
    // Denoiser support
    void setDenoiseEnabled(bool enabled);
    bool isDenoiseEnabled() const;
    
    // Raytracing mode support
    void setRaytraceMode(bool enabled);
    bool isRaytraceMode() const;
    
    // Reflection samples per pixel support
    void setReflectionSpp(int spp);
    int getReflectionSpp() const;
    
    // Schema validation support
    void setStrictSchema(bool enabled, const std::string& version = "v1.3");
    bool isStrictSchemaEnabled() const;
    
    // Render settings support
    void setRenderSettings(const RenderSettings& settings);
    const RenderSettings& getRenderSettings() const;
    
    // Input callbacks (called by GLFW)
    void handleMouseMove(double xpos, double ypos);
    void handleMouseButton(int button, int action, int mods);
    void handleFramebufferResize(int width, int height);
    void handleKey(int key, int scancode, int action, int mods);
    void handleFileDrop(int count, const char** paths);

    // Getters for external access (UserInput, etc.)
    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    GLFWwindow* getWindow() const { return m_window; }
    
    CameraController& getCameraController() { return *m_camera; }
    const CameraController& getCameraController() const { return *m_camera; }
    
    SceneManager& getSceneManager() { return *m_scene; }
    const SceneManager& getSceneManager() const { return *m_scene; }

private:
    // Core systems
    std::unique_ptr<SceneManager> m_scene;
    std::unique_ptr<RenderSystem> m_renderer;
    std::unique_ptr<CameraController> m_camera; 
    std::unique_ptr<Light> m_lights;
    std::unique_ptr<UIBridge> m_uiBridge;
    std::unique_ptr<JsonOpsExecutor> m_ops;
    
    // Platform/window management
    GLFWwindow* m_window = nullptr;
    int m_windowWidth = 800;
    int m_windowHeight = 600;
    bool m_headless = false;
    
    // Input state
    bool m_leftMousePressed = false;
    bool m_rightMousePressed = false;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_firstMouse = true;
    double m_lastFrameTime = 0.0;
    bool   m_requireRMBToMove = true;

    // Selection
    int m_selectedLightIndex = -1;
    
    // Render settings
    RenderSettings m_renderSettings;

    // Gizmo state
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    GizmoAxis m_gizmoAxis = GizmoAxis::None;
    bool m_gizmoLocal = true;
    bool m_gizmoDragging = false;
    glm::vec3 m_dragOriginWorld{0.0f};
    glm::vec3 m_dragAxisDir{0.0f};
    float m_axisStartS = 0.0f;
    int   m_dragObjectIndex = -1;
    int   m_dragLightIndex = -1;
    glm::mat4 m_modelStart{1.0f};
    
    // Initialization
    bool initGLFW(const std::string& windowTitle, int width, int height);
    bool initGLAD();
    void initCallbacks();
    void createDefaultScene();
    void setWindowIcon();
    
    // GLFW callback wrappers
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void dropCallback(GLFWwindow* window, int count, const char** paths);
    
    // Cleanup
    void cleanupGL();
};
