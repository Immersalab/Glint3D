#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <cstdint>
#include "gl_platform.h"
#include "gizmo.h"

// Forward declarations
class SceneManager;
class Light; 
class Raytracer;
class AxisRenderer;
class Grid;
class Gizmo;
class Skybox;
struct SceneObject;

class Shader;

enum class RenderToneMapMode {
    Linear = 0,
    Reinhard,
    Filmic,
    ACES
};

enum class RenderMode {
    Points = 0,
    Wireframe = 1, 
    Solid = 2,
    Raytrace = 3
};

enum class ShadingMode {
    Flat = 0,
    Gouraud = 1
};

struct CameraState {
    glm::vec3 position{0.0f, 0.0f, 10.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float fov = 45.0f;
    float nearClip = 0.1f;
    float farClip = 100.0f;
    float yaw = -90.0f;
    float pitch = 0.0f;
};

struct RenderStats {
    int drawCalls = 0;
    size_t totalTriangles = 0;
    int uniqueMaterialKeys = 0;
    size_t uniqueTextures = 0;
    float texturesMB = 0.0f;
    float geometryMB = 0.0f;
    float vramMB = 0.0f;
    int topSharedCount = 0;
    std::string topSharedKey;
};

class RenderSystem 
{
public:
    RenderSystem();
    ~RenderSystem();

    bool init(int windowWidth, int windowHeight);
    void shutdown();

    // Main render call
    void render(const SceneManager& scene, const Light& lights);
    
    // Offscreen rendering
    bool renderToTexture(const SceneManager& scene, const Light& lights, 
                        GLuint textureId, int width, int height);
    bool renderToPNG(const SceneManager& scene, const Light& lights,
                    const std::string& path, int width, int height);

    // Camera management
    void setCamera(const CameraState& camera) { m_camera = camera; }
    const CameraState& getCamera() const { return m_camera; }
    CameraState& getCamera() { return m_camera; }
    
    void updateViewMatrix();
    void updateProjectionMatrix(int windowWidth, int windowHeight);
    
    glm::mat4 getViewMatrix() const { return m_viewMatrix; }
    glm::mat4 getProjectionMatrix() const { return m_projectionMatrix; }

    // Render modes
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    RenderMode getRenderMode() const { return m_renderMode; }
    
    void setShadingMode(ShadingMode mode) { m_shadingMode = mode; }
    ShadingMode getShadingMode() const { return m_shadingMode; }

    // Settings
    void setFramebufferSRGBEnabled(bool enabled) { m_framebufferSRGBEnabled = enabled; }
    bool isFramebufferSRGBEnabled() const { return m_framebufferSRGBEnabled; }

    // Presentation/appearance
    void setBackgroundColor(const glm::vec3& c) { m_backgroundColor = c; }
    const glm::vec3& getBackgroundColor() const { return m_backgroundColor; }
    bool loadSkybox(const std::string& path); // simple loader/enable toggle
    void setExposure(float v) { m_exposure = v; }
    float getExposure() const { return m_exposure; }
    void setToneMapping(RenderToneMapMode m) { m_tonemap = m; }
    RenderToneMapMode getToneMapping() const { return m_tonemap; }
    void setGamma(float g) { m_gamma = g; }
    float getGamma() const { return m_gamma; }
    
    // Random seed for deterministic rendering
    void setSeed(uint32_t seed) { m_seed = seed; }
    uint32_t getSeed() const { return m_seed; }

    // Debug/utility rendering
    void setShowGrid(bool show) { m_showGrid = show; }
    void setShowAxes(bool show) { m_showAxes = show; }
    void setShowSkybox(bool show) { m_showSkybox = show; }
    bool isShowGrid() const { return m_showGrid; }
    bool isShowAxes() const { return m_showAxes; }
    bool isShowSkybox() const { return m_showSkybox; }

    // Statistics
    const RenderStats& getLastFrameStats() const { return m_stats; }

    // MSAA sample control
    void setSampleCount(int samples) { m_samples = samples < 1 ? 1 : samples; m_recreateTargets = true; }
    int  getSampleCount() const { return m_samples; }

    // Raytracing
    void setDenoiseEnabled(bool enabled) { m_denoiseEnabled = enabled; }
    bool isDenoiseEnabled() const { return m_denoiseEnabled; }
    bool denoise(std::vector<glm::vec3>& color,
                const std::vector<glm::vec3>* normal = nullptr,
                const std::vector<glm::vec3>* albedo = nullptr);
    bool denoise(std::vector<glm::vec3>& color, int width, int height,
                const std::vector<glm::vec3>* normal = nullptr,
                const std::vector<glm::vec3>* albedo = nullptr);

    // Gizmo support - forward declared, implemented in cpp
    class Gizmo* getGizmo() { return m_gizmo.get(); }
    const class Gizmo* getGizmo() const { return m_gizmo.get(); }
    
    // Skybox access
    class Skybox* getSkybox() { return m_skybox.get(); }
    const class Skybox* getSkybox() const { return m_skybox.get(); }
    
    // Gizmo/selection configuration
    void setGizmoMode(GizmoMode mode) { m_gizmoMode = mode; }
    void setGizmoAxis(GizmoAxis axis) { m_gizmoAxis = axis; }
    void setGizmoLocalSpace(bool local) { m_gizmoLocal = local; }
    void setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }
    void setSelectedLightIndex(int idx) { m_selectedLightIndex = idx; }
    GizmoMode gizmoMode() const { return m_gizmoMode; }
    GizmoAxis gizmoAxis() const { return m_gizmoAxis; }
    bool gizmoLocalSpace() const { return m_gizmoLocal; }
    bool snapEnabled() const { return m_snapEnabled; }
    int selectedLightIndex() const { return m_selectedLightIndex; }

private:
    // Core rendering state
    CameraState m_camera;
    glm::mat4 m_viewMatrix{1.0f};
    glm::mat4 m_projectionMatrix{1.0f};
    
    RenderMode m_renderMode = RenderMode::Solid;
    ShadingMode m_shadingMode = ShadingMode::Gouraud;
    bool m_framebufferSRGBEnabled = true;
    glm::vec3 m_backgroundColor{0.10f, 0.11f, 0.12f};
    float m_exposure = 0.0f;
    float m_gamma = 2.2f;
    RenderToneMapMode m_tonemap = RenderToneMapMode::Linear;
    uint32_t m_seed = 0;
    
    // Debug rendering
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_showSkybox = false;
    
    // Utility renderers
    std::unique_ptr<AxisRenderer> m_axisRenderer;
    std::unique_ptr<Grid> m_grid;
    std::unique_ptr<Gizmo> m_gizmo;
    std::unique_ptr<Skybox> m_skybox;
    
    // Raytracer
    std::unique_ptr<Raytracer> m_raytracer;
    bool m_denoiseEnabled = false;
    
    // Raytracing screen quad resources
    GLuint m_screenQuadVAO = 0;
    GLuint m_screenQuadVBO = 0;
    GLuint m_raytraceTexture = 0;
    std::unique_ptr<Shader> m_screenQuadShader;
    int m_raytraceWidth = 512;
    int m_raytraceHeight = 512;
    
    // Shaders
    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<Shader> m_pbrShader;
    std::unique_ptr<Shader> m_gridShader;
    
    // Fallback shadow map to satisfy shaders that sample shadowMap
    GLuint m_dummyShadowTex = 0;
    
    // Statistics
    RenderStats m_stats;
    
    // Private methods
    void renderRasterized(const SceneManager& scene, const Light& lights);
    void renderRaytraced(const SceneManager& scene, const Light& lights);
    void renderObject(const SceneObject& obj, const Light& lights);
    void updateRenderStats(const SceneManager& scene);
    
    // Optimized rendering methods
    void renderDebugElements(const SceneManager& scene, const Light& lights);
    void renderSelectionOutline(const SceneManager& scene);
    void renderGizmo(const SceneManager& scene, const Light& lights);
    void renderObjectsBatched(const SceneManager& scene, const Light& lights);
    void setupCommonUniforms(Shader* shader);
    void renderObjectFast(const SceneObject& obj, const Light& lights, Shader* shader);
    
    // Raytracing support methods
    void initScreenQuad();
    void initRaytraceTexture();
    void cleanupRaytracing();

    // Gizmo state
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    GizmoAxis m_gizmoAxis = GizmoAxis::None;
    bool m_gizmoLocal = true;
    bool m_snapEnabled = false;
    int  m_selectedLightIndex = -1;

    // MSAA offscreen pipeline for onscreen rendering
    int   m_samples = 1;
    bool  m_recreateTargets = false;
    int   m_fbWidth = 0;
    int   m_fbHeight = 0;
    GLuint m_msaaFBO = 0;
    GLuint m_msaaColorRBO = 0;
    GLuint m_msaaDepthRBO = 0;

    // Internal helpers
    void createOrResizeTargets(int width, int height);
    void destroyTargets();
};
