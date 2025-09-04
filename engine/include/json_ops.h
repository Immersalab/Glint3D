#pragma once

#include <string>
#include <memory>

// Forward declarations to avoid heavy includes in header
class SceneManager;
class RenderSystem;
class CameraController;
class Light;

// JsonOpsExecutor: parses and applies JSON operations to the engine state.
//
// Goals:
// - Keep JSON ops logic separate from UI concerns
// - Provide a clean API for headless CLI and any UI bridge
// - Make it easy to extend with more ops and versions
class JsonOpsExecutor {
public:
    JsonOpsExecutor(SceneManager& scene,
                    RenderSystem& renderer,
                    CameraController& camera,
                    Light& lights);
    ~JsonOpsExecutor() = default;

    // Apply a JSON string containing one or more operations.
    // Returns true on success; false populates `error` with a human-readable message.
    bool apply(const std::string& json, std::string& error);

private:
    SceneManager& m_scene;
    RenderSystem& m_renderer;
    CameraController& m_camera;
    Light& m_lights;
};

