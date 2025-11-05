// Machine Summary Block
// {"file":"engine/core/application/application_core.h","purpose":"Declares the ApplicationCore interface managing engine lifecycle and rendering entry points.","exports":["ApplicationCore"],"depends_on":["render_settings.h","glm/glm.hpp","gizmo.h"],"notes":["exposes_public_runtime_controls"]}
// Human Summary
// Application facade used by CLI and UI paths to bootstrap the renderer, scenes, and utility helpers.

#pragma once
/// @file application_core.h
/// @brief Declares the ApplicationCore orchestration facade for Glint3D.
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

/// @brief Coordinates initialization, runtime loop, and high-level engine interactions.
class ApplicationCore 
{
public:
    /// @brief Constructs an uninitialized application facade.
    ApplicationCore();

    /// @brief Destroys the application facade and releases owned resources.
    ~ApplicationCore();

    /// @brief Initializes the platform window and core subsystems.
    /// @param windowTitle Title to display in the primary window.
    /// @param width Requested window width in pixels.
    /// @param height Requested window height in pixels.
    /// @param headless When true, skips window creation for off-screen rendering.
    /// @return True if all subsystems initialized successfully.
    bool init(const std::string& windowTitle, int width, int height, bool headless = false);

    /// @brief Runs the interactive main loop until shutdown is requested.
    void run();

    /// @brief Advances a single frame; useful for external event loops (e.g., Emscripten).
    void frame();

    /// @brief Shuts down rendering, scene, and platform subsystems.
    void shutdown();

    /// @brief Loads an object into the active scene.
    /// @param name Friendly name for the object inside the scene graph.
    /// @param path Filesystem path to the object source asset.
    /// @param position World-space position for the object origin.
    /// @param scale Uniform or per-axis scale applied at load time.
    /// @return True if the object was loaded and added to the scene.
    bool loadObject(const std::string& name, const std::string& path, 
                   const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f));

    /// @brief Renders the current scene to a PNG on disk.
    /// @param path Destination image path.
    /// @param width Output width in pixels.
    /// @param height Output height in pixels.
    /// @return True if rendering and file write succeeded.
    bool renderToPNG(const std::string& path, int width, int height);

    /// @brief Applies a JSON Ops v1 script to the active scene.
    /// @param json JSON document describing operations to run.
    /// @param error Populated with an error message when the script fails.
    /// @return True if the operations were applied successfully.
    bool applyJsonOpsV1(const std::string& json, std::string& error);

    /// @brief Builds a shareable link that encodes the current scene payload.
    /// @return Encoded link string.
    std::string buildShareLink() const;

    /// @brief Serializes the active scene to a JSON document.
    /// @return JSON representation of the current scene.
    std::string sceneToJson() const;
    
    /// @brief Enables or disables the denoiser.
    /// @param enabled True to enable denoising.
    void setDenoiseEnabled(bool enabled);

    /// @brief Reports whether denoising is currently enabled.
    /// @return True when the denoiser runs during rendering.
    bool isDenoiseEnabled() const;
    
    /// @brief Toggles the ray-tracing rendering mode.
    /// @param enabled True to enable ray tracing.
    void setRaytraceMode(bool enabled);

    /// @brief Indicates whether ray-tracing mode is active.
    /// @return True if ray tracing is enabled.
    bool isRaytraceMode() const;
    
    /// @brief Adjusts the number of reflection samples per pixel.
    /// @param spp Samples-per-pixel value to apply.
    void setReflectionSpp(int spp);

    /// @brief Returns the current reflection samples-per-pixel count.
    /// @return Active reflection SPP value.
    int getReflectionSpp() const;
    
    /// @brief Configures schema validation behavior for JSON inputs.
    /// @param enabled True to enforce schema validation.
    /// @param version Schema version identifier to validate against.
    void setStrictSchema(bool enabled, const std::string& version = "v1.3");

    /// @brief Reports whether schema validation is active.
    /// @return True when JSON inputs are validated against the schema.
    bool isStrictSchemaEnabled() const;
    
    /// @brief Applies a full render settings payload.
    /// @param settings Settings structure to copy into the engine.
    void setRenderSettings(const RenderSettings& settings);

    /// @brief Retrieves the active render settings view.
    /// @return Constant reference to the stored render settings.
    const RenderSettings& getRenderSettings() const;
    
    /// @brief Handles GLFW mouse movement callbacks.
    /// @param xpos Cursor X coordinate in screen space.
    /// @param ypos Cursor Y coordinate in screen space.
    void handleMouseMove(double xpos, double ypos);

    /// @brief Handles GLFW mouse button callbacks.
    /// @param button GLFW mouse button identifier.
    /// @param action GLFW press/release action.
    /// @param mods Modifier bitmask supplied by GLFW.
    void handleMouseButton(int button, int action, int mods);

    /// @brief Handles framebuffer resize callbacks.
    /// @param width New framebuffer width in pixels.
    /// @param height New framebuffer height in pixels.
    void handleFramebufferResize(int width, int height);

    /// @brief Handles keyboard input from GLFW.
    /// @param key GLFW keycode.
    /// @param scancode Scan code supplied by GLFW.
    /// @param action GLFW action (press/release/repeat).
    /// @param mods Modifier bitmask supplied by GLFW.
    void handleKey(int key, int scancode, int action, int mods);

    /// @brief Handles drag-and-drop file events.
    /// @param count Number of paths supplied.
    /// @param paths UTF-8 encoded file paths.
    void handleFileDrop(int count, const char** paths);

    /// @brief Returns the current window width.
    /// @return Window width in pixels.
    int getWindowWidth() const { return m_windowWidth; }

    /// @brief Returns the current window height.
    /// @return Window height in pixels.
    int getWindowHeight() const { return m_windowHeight; }

    /// @brief Provides direct access to the GLFW window handle.
    /// @return Pointer to the GLFW window, or nullptr when headless.
    GLFWwindow* getWindow() const { return m_window; }
    
    /// @brief Retrieves the primary camera controller.
    /// @return Mutable camera controller reference.
    CameraController& getCameraController() { return *m_camera; }

    /// @brief Retrieves the primary camera controller.
    /// @return Immutable camera controller reference.
    const CameraController& getCameraController() const { return *m_camera; }
    
    /// @brief Retrieves the active scene manager.
    /// @return Mutable scene manager reference.
    SceneManager& getSceneManager() { return *m_scene; }

    /// @brief Retrieves the active scene manager.
    /// @return Immutable scene manager reference.
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
