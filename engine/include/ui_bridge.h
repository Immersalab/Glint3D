#pragma once
#include <string>
#include <vector>
#include <functional>
#include "gizmo.h"
#include "render_system.h"
#include "camera_controller.h"

// Forward declarations
class SceneManager;
class RenderSystem; 
class Light;

// Include needed for unique_ptr
#include "json_ops.h"

// UI-independent state snapshot for any UI implementation
struct UIState {
    // Visibility toggles
    bool showSettingsPanel = true;
    bool showPerfHUD = false;
    bool showGrid = true;
    bool showAxes = true;
    bool showSkybox = false;
    
    // Rendering state
    RenderMode renderMode = RenderMode::Solid;
    ShadingMode shadingMode = ShadingMode::Gouraud;
    bool framebufferSRGBEnabled = true;
    bool denoiseEnabled = false;
    int msaaSamples = 1; // UI exposure of MSAA samples
    
    // Environment/lighting state
    RenderSystem::BackgroundMode backgroundMode = RenderSystem::BackgroundMode::Solid;
    glm::vec3 backgroundSolid{0.10f,0.11f,0.12f};
    glm::vec3 backgroundTop{0.10f,0.11f,0.12f};
    glm::vec3 backgroundBottom{0.10f,0.11f,0.12f};
    std::string backgroundHDRPath;
    float skyboxIntensity = 1.0f;
    float iblIntensity = 1.0f;
    std::string environmentPath;
    
    // Camera state
    CameraState camera;
    float cameraSpeed = 0.5f;
    float sensitivity = 0.1f;
    bool requireRMBToMove = true;
    
    // Selection
    int selectedObjectIndex = -1;
    std::string selectedObjectName;
    int selectedLightIndex = -1;
    int objectCount = 0;
    int lightCount = 0;
    
    // Objects list for hierarchy UI
    std::vector<std::string> objectNames;
    
    // Light details for UI editing
    struct LightUI {
        int type = 0;              // 0=point, 1=directional
        glm::vec3 position{0.0f};  // point
        glm::vec3 direction{0.0f, -1.0f, 0.0f}; // directional
        glm::vec3 color{1.0f};
        float intensity = 1.0f;
        bool enabled = true;
        float innerConeDeg = 15.0f;
        float outerConeDeg = 25.0f;
    };
    std::vector<LightUI> lights;   // mirrors lights for editing
    
    // Gizmo state
    GizmoMode gizmoMode = GizmoMode::Translate;
    GizmoAxis gizmoAxis = GizmoAxis::None;
    bool gizmoLocalSpace = true;
    bool snapEnabled = false;
    float snapTranslate = 0.5f;
    float snapRotateDeg = 15.0f;
    float snapScale = 0.1f;
    
    // Statistics
    RenderStats renderStats;
    
    // Console/log
    std::vector<std::string> consoleLog;

    // Recent files (MRU)
    std::vector<std::string> recentFiles;

    // AI controls
    bool useAI = true;
    std::string aiEndpoint; // e.g. http://127.0.0.1:11434
};

// Command interface for UI actions
enum class UICommand {
    LoadObject,
    RemoveObject,
    DuplicateObject,
    RenameObject,
    SelectObject,
    SetRenderMode,
    SetShadingMode,
    ToggleFramebufferSRGB,
    SetCameraSpeed,
    SetMouseSensitivity,
    AddLight,
    AddPointLight,
    AddDirectionalLight,
    AddSpotLight,
    RemoveLight,
    SelectLight,
    DeleteLight,
    SetLightEnabled,
    SetLightIntensity,
    SetLightDirection,
    SetLightPosition,
    SetLightInnerCone,
    SetLightOuterCone,
    SetGizmoMode,
    ToggleGizmoSpace,
    ToggleSnap,
    ExecuteConsoleCommand,
    ApplyJsonOps,
    RenderToPNG,
    SetMSAASamples,
    SetUseAI,
    SetAIEndpoint,
    SetRequireRMBToMove,
    
    // UI visibility toggles
    ToggleSettingsPanel,
    TogglePerfHUD,
    ToggleGrid,
    ToggleAxes,
    ToggleSkybox,
    
    // IBL/Environment controls
    LoadHDREnvironment,
    SetSkyboxIntensity,
    SetIBLIntensity,
    
    // Scene operations
    CenterCamera,
    SetCameraPreset,
    ResetScene,
    
    // Application control
    CopyShareLink,
    ExitApplication,
    
    // File operations
    ImportModel,
    ImportSceneJSON,
    ExportScene,
    OpenFile
};

struct UICommandData {
    UICommand command;
    std::string stringParam;
    float floatParam = 0.0f;
    int intParam = 0;
    glm::vec3 vec3Param{0.0f};
    bool boolParam = false;
};

// Abstract UI layer interface
class IUILayer {
public:
    virtual ~IUILayer() = default;
    
    virtual bool init(int windowWidth, int windowHeight) = 0;
    virtual void shutdown() = 0;
    virtual void render(const UIState& state) = 0;
    virtual void handleResize(int width, int height) = 0;
    virtual void handleCommand(const UICommandData& cmd) = 0;
    
    // Command callback - UI implementations call this to execute actions
    std::function<void(const UICommandData&)> onCommand;
};

// Bridge class that coordinates between core systems and UI
class UIBridge {
public:
    UIBridge(SceneManager& scene, RenderSystem& renderer, 
             CameraController& camera, Light& lights);
    ~UIBridge() = default;
    
    // UI lifecycle
    void setUILayer(std::unique_ptr<IUILayer> ui) { m_ui = std::move(ui); }
    bool initUI(int windowWidth, int windowHeight);
    void shutdownUI();
    void renderUI();
    void handleResize(int width, int height);
    
    // State management
    UIState buildUIState() const;
    void handleUICommand(const UICommandData& command);
    
    // Console/logging
    void addConsoleMessage(const std::string& message);
    void clearConsoleLog();

    // Light selection for UI state
    void setSelectedLightIndex(int idx) { m_selectedLightIndex = idx; }
    int  getSelectedLightIndex() const { return m_selectedLightIndex; }

    // Settings queried by core
    bool getRequireRMBToMove() const { return m_requireRMBToMove; }
    
    // JSON Ops integration
    bool applyJsonOps(const std::string& json, std::string& error);
    std::string buildShareLink() const;
    std::string sceneToJson() const;
    
private:
    // System references
    SceneManager& m_scene;
    RenderSystem& m_renderer;
    CameraController& m_camera;
    Light& m_lights;
    
    // UI layer
    std::unique_ptr<IUILayer> m_ui;
    
    // State
    std::vector<std::string> m_consoleLog;
    bool m_previewOnly = false;
    bool m_useAI = true;
    std::string m_aiEndpoint;
    bool m_requireRMBToMove = true;
    int  m_selectedLightIndex = -1;

    // JSON ops executor (modularized from UI)
    std::unique_ptr<JsonOpsExecutor> m_ops;
    
    // MRU recent files
    std::vector<std::string> m_recentFiles;
    size_t m_recentMax = 10;
    void loadRecentFiles();
    void saveRecentFiles() const;
    void addRecentFile(const std::string& path);
    bool openFilePath(const std::string& path);
    
    // Command handlers
    void handleLoadObject(const UICommandData& cmd);
    void handleRenderMode(const UICommandData& cmd);
    void handleCameraSettings(const UICommandData& cmd);
    void handleGizmoSettings(const UICommandData& cmd);
    void handleConsoleCommand(const UICommandData& cmd);
};
