// Machine Summary Block
// {"file":"engine/platform/desktop/ui_bridge.cpp","purpose":"Implements ImGui-driven UI bridge and command console for Glint3D.","exports":["UIBridge"],"depends_on":["scene_manager.h","render_utils.h","imgui"],"notes":["console_defaults_output_renders","desktop_platform_layer"]}
// Human Summary
// UI bridge wiring ImGui panels, command parsing, and render/export helpers targeting output/renders by default.

#include "ui_bridge.h"
#include "scene_manager.h"
#include "render_system.h"
#include "camera_controller.h"
#include "light.h"
#include "skybox.h"
#include "ibl_system.h"
#include "json_ops.h"
#include "config_defaults.h"
#include "render_utils.h"
#include "help_text.h"
#include "file_dialog.h"
#include "resource_paths.h"
#include "user_paths.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>

namespace {

glm::vec3 extractTranslation(const glm::mat4& matrix)
{
    return glm::vec3(matrix[3]);
}

glm::vec3 extractScale(const glm::mat4& matrix)
{
    return {
        glm::length(glm::vec3(matrix[0])),
        glm::length(glm::vec3(matrix[1])),
        glm::length(glm::vec3(matrix[2]))
    };
}

glm::vec3 extractRotationDegrees(const glm::mat4& matrix)
{
    glm::vec3 scale = extractScale(matrix);
    glm::mat3 rotationMatrix(
        glm::vec3(matrix[0]) / (scale.x == 0.0f ? 1.0f : scale.x),
        glm::vec3(matrix[1]) / (scale.y == 0.0f ? 1.0f : scale.y),
        glm::vec3(matrix[2]) / (scale.z == 0.0f ? 1.0f : scale.z)
    );

    float rotY = asinf(glm::clamp(rotationMatrix[0][2], -1.0f, 1.0f));
    float rotX;
    float rotZ;
    if (std::abs(cosf(rotY)) > 0.0001f) {
        rotX = atan2f(-rotationMatrix[1][2], rotationMatrix[2][2]);
        rotZ = atan2f(-rotationMatrix[0][1], rotationMatrix[0][0]);
    } else {
        rotX = atan2f(rotationMatrix[2][1], rotationMatrix[1][1]);
        rotZ = 0.0f;
    }
    return glm::degrees(glm::vec3(rotX, rotY, rotZ));
}

glm::mat4 composeTransform(const glm::vec3& translation,
                           const glm::vec3& rotationDeg,
                           const glm::vec3& scale)
{
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, translation);
    transform = glm::rotate(transform, glm::radians(rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, scale);
    return transform;
}

bool parseVec3(const rapidjson::Value& value, glm::vec3& out)
{
    if (!value.IsArray() || value.Size() != 3) {
        return false;
    }
    out = glm::vec3(
        static_cast<float>(value[0].GetDouble()),
        static_cast<float>(value[1].GetDouble()),
        static_cast<float>(value[2].GetDouble()));
    return true;
}

} // namespace

UIBridge::UIBridge(SceneManager& scene, RenderSystem& renderer,
                   CameraController& camera, Light& lights)
    : m_scene(scene)
    , m_renderer(renderer)
    , m_camera(camera)
    , m_lights(lights)
{
    // Initialize modular JSON ops executor for shared implementation
    m_ops = std::make_unique<JsonOpsExecutor>(m_scene, m_renderer, m_camera, m_lights);
}

void UIBridge::setWorkspaceRoot(const std::filesystem::path& workspaceRoot)
{
    if (workspaceRoot.empty()) {
        m_workspaceRoot.clear();
        return;
    }

    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(workspaceRoot, ec);
    m_workspaceRoot = ec ? workspaceRoot : canonical;
}

bool UIBridge::bootstrapWorkspace()
{
    if (m_workspaceRoot.empty()) {
        return false;
    }

    std::string error;
    bool loaded = loadWorkspaceState(&error);
    if (loaded) {
        addConsoleMessage("Loaded workspace: " + m_workspaceRoot.generic_string());
    } else {
        if (!error.empty() && error.find("not found") == std::string::npos) {
            addConsoleMessage(error);
        }
        clearSceneForWorkspace();
        addConsoleMessage("Workspace ready: " + m_workspaceRoot.generic_string());
    }
    return true;
}

bool UIBridge::initUI(int windowWidth, int windowHeight)
{
    if (!m_ui) {
        return true; // No UI layer to initialize
    }
    
    // Set up command callback
    m_ui->onCommand = [this](const UICommandData& cmd) {
        handleUICommand(cmd);
    };
    
    bool ok = m_ui->init(windowWidth, windowHeight);
    if (ok) {
        // Load MRU recent files
        loadRecentFiles();
        // Show GLINT3D ASCII banner, version, then welcome lines
        for_each_glint_ascii([this](const std::string& line){ this->addConsoleMessage(line); });
        emit_welcome_lines([this](const std::string& line){ this->addConsoleMessage(line); });
    }
    return ok;
}

void UIBridge::shutdownUI()
{
    if (m_ui) {
        m_ui->shutdown();
    }
}

void UIBridge::renderUI()
{
    if (!m_ui) {
        return;
    }
    
    UIState state = buildUIState();
    m_ui->render(state);
}

void UIBridge::handleResize(int width, int height)
{
    if (m_ui) {
        m_ui->handleResize(width, height);
    }
}

UIState UIBridge::buildUIState() const
{
    UIState state;
    state.workspaceRoot = m_workspaceRoot.empty() ? std::string()
                                                  : m_workspaceRoot.generic_string();
    
    // Camera state
    state.camera = m_camera.getCameraState();
    state.cameraSpeed = m_camera.getSpeed();
    state.sensitivity = m_camera.getSensitivity();
    
    // Render state
    state.renderMode = m_renderer.getRenderMode();
    state.shadingMode = m_renderer.getShadingMode();
    state.framebufferSRGBEnabled = m_renderer.isFramebufferSRGBEnabled();
    state.denoiseEnabled = m_renderer.isDenoiseEnabled();
    state.msaaSamples = m_renderer.getSampleCount();
    state.showGrid = m_renderer.isShowGrid();
    state.showAxes = m_renderer.isShowAxes();
    state.showSkybox = m_renderer.isShowSkybox();
    state.requireRMBToMove = m_requireRMBToMove;
    state.backgroundMode = m_renderer.getBackgroundMode();
    state.backgroundSolid = m_renderer.getBackgroundColor();
    state.backgroundTop = m_renderer.getBackgroundTopColor();
    state.backgroundBottom = m_renderer.getBackgroundBottomColor();
    state.backgroundHDRPath = m_renderer.getBackgroundHDRPath();
    
    // Environment/IBL state
    if (m_renderer.getSkybox()) {
        state.skyboxIntensity = m_renderer.getSkybox()->getIntensity();
    }
    if (m_renderer.getIBLSystem()) {
        state.iblIntensity = m_renderer.getIBLSystem()->getIntensity();
    }
    
    // Scene state
    state.selectedObjectIndex = m_scene.getSelectedObjectIndex();
    state.selectedObjectName = m_scene.getSelectedObjectName();
    state.objectCount = (int)m_scene.getObjects().size();
    // Populate object names for hierarchy panel
    state.objectNames.clear();
    state.objectNames.reserve(m_scene.getObjects().size());
    for (const auto& obj : m_scene.getObjects()) {
        state.objectNames.push_back(obj.name);
    }
    // Get hierarchy information from SceneManager
    state.objectParentIndex = m_scene.getParentIndices();
    
    // Light state  
    state.lightCount = (int)m_lights.getLightCount();
    state.selectedLightIndex = m_selectedLightIndex;
    state.lights.clear();
    state.lights.reserve(m_lights.m_lights.size());
    for (const auto& l : m_lights.m_lights) {
        UIState::LightUI lu;
        lu.type = (int)l.type;
        lu.position = l.position;
        lu.direction = l.direction;
        lu.color = l.color;
        lu.intensity = l.intensity;
        lu.enabled = l.enabled;
        lu.innerConeDeg = l.innerConeDeg;
        lu.outerConeDeg = l.outerConeDeg;
        state.lights.push_back(lu);
    }
    
    // Statistics
    state.renderStats = m_renderer.getLastFrameStats();
    
    // Console log
    state.consoleLog = m_consoleLog;

    // Recent files MRU
    state.recentFiles = m_recentFiles;

    return state;
}

void UIBridge::handleUICommand(const UICommandData& command)
{
    switch (command.command) {
        case UICommand::LoadObject:
            handleLoadObject(command);
            break;
        case UICommand::OpenFile:
            {
                if (!command.stringParam.empty()) {
                    bool ok = openFilePath(command.stringParam);
                    if (!ok) addConsoleMessage("Open failed: " + command.stringParam);
                }
            }
            break;
        case UICommand::ReparentObject:
            {
                int child = command.intParam;
                int newParent = command.intParam2;
                
                if (m_scene.reparentObject(child, newParent)) {
                    std::string childName = (child >= 0 && child < static_cast<int>(m_scene.getObjects().size())) 
                        ? m_scene.getObjects()[child].name : "unknown";
                    std::string parentName = (newParent >= 0 && newParent < static_cast<int>(m_scene.getObjects().size()))
                        ? m_scene.getObjects()[newParent].name : "root";
                    addConsoleMessage("Reparented '" + childName + "' to '" + parentName + "'");
                } else {
                    addConsoleMessage("Reparenting failed - check console for details");
                }
            }
            break;
        case UICommand::RemoveObject:
            {
                std::string name = command.stringParam;
                if (name.empty() && command.intParam >= 0 && command.intParam < (int)m_scene.getObjects().size()) {
                    name = m_scene.getObjects()[(size_t)command.intParam].name;
                }
                if (!name.empty()) {
                    if (m_scene.deleteObject(name)) {
                        addConsoleMessage("Deleted object: " + name);
                    } else {
                        addConsoleMessage("Delete failed: " + name);
                    }
                }
            }
            break;
        case UICommand::DuplicateObject:
            {
                std::string src = command.stringParam;
                if (src.empty() && command.intParam >= 0 && command.intParam < (int)m_scene.getObjects().size()) {
                    src = m_scene.getObjects()[(size_t)command.intParam].name;
                }
                if (!src.empty()) {
                    // Propose a unique name: src + "_copy", with numeric suffix if needed
                    std::string base = src + "_copy";
                    std::string newName = base;
                    int n = 1;
                    while (m_scene.findObjectByName(newName) != nullptr) {
                        newName = base + std::string("_") + std::to_string(n++);
                    }
                    // New position slightly offset on Z
                    glm::vec3 pos = m_scene.getSelectedObjectCenterWorld();
                    pos.z -= 0.2f;
                    if (m_scene.duplicateObject(src, newName, pos)) {
                        addConsoleMessage("Duplicated '" + src + "' as '" + newName + "'");
                    } else {
                        addConsoleMessage("Duplicate failed: " + src);
                    }
                }
            }
            break;
        case UICommand::RenameObject:
            {
                // stringParam: new name, intParam: index to rename
                int idx = command.intParam;
                if (idx >= 0 && idx < (int)m_scene.getObjects().size()) {
                    std::string oldName = m_scene.getObjects()[(size_t)idx].name;
                    const std::string& newName = command.stringParam;
                    if (!newName.empty() && oldName != newName) {
                        // Implemented in SceneManager via find + set
                        // Fallback here if no API: check duplicates and set
                        if (m_scene.findObjectByName(newName) == nullptr) {
                            auto& obj = m_scene.getObjects()[(size_t)idx];
                            obj.name = newName;
                            addConsoleMessage("Renamed '" + oldName + "' to '" + newName + "'");
                        } else {
                            addConsoleMessage("Rename failed: name exists: " + newName);
                        }
                    }
                }
            }
            break;
        case UICommand::SelectObject:
            {
                // Prefer intParam index; fallback to name lookup
                if (command.intParam >= 0 && command.intParam < (int)m_scene.getObjects().size()) {
                    m_scene.setSelectedObjectIndex(command.intParam);
                } else if (!command.stringParam.empty()) {
                    int idx = m_scene.findObjectIndex(command.stringParam);
                    if (idx >= 0) m_scene.setSelectedObjectIndex(idx);
                }
            }
            break;
            
        case UICommand::SetRenderMode:
            handleRenderMode(command);
            break;
            
        case UICommand::SetCameraSpeed:
        case UICommand::SetMouseSensitivity:
            handleCameraSettings(command);
            break;
            
        case UICommand::SetGizmoMode:
        case UICommand::ToggleGizmoSpace:
        case UICommand::ToggleSnap:
            handleGizmoSettings(command);
            break;
            
        case UICommand::ExecuteConsoleCommand:
            handleConsoleCommand(command);
            break;
            
        case UICommand::ApplyJsonOps:
            {
                std::string error;
                applyJsonOps(command.stringParam, error);
                if (!error.empty()) {
                    addConsoleMessage("JSON Ops error: " + error);
                }
            }
            break;
            
        case UICommand::RenderToPNG:
            {
                bool success = m_renderer.renderToPNG(m_scene, m_lights, command.stringParam, 
                                                     command.intParam, (int)command.floatParam);
                if (success) {
                    addConsoleMessage("Rendered to: " + command.stringParam);
                } else {
                    addConsoleMessage("Render to PNG failed");
                }
            }
            break;
        case UICommand::SetMSAASamples:
            {
                int s = std::max(1, command.intParam);
                m_renderer.setSampleCount(s);
                addConsoleMessage("MSAA samples set to " + std::to_string(s));
            }
            break;
        case UICommand::AddLight:
            {
                // Add a light at the specified position
                glm::vec3 pos = command.vec3Param;
                glm::vec3 color(1.0f, 1.0f, 1.0f);  // Default white
                float intensity = 1.0f;
                m_lights.addLight(pos, color, intensity);
                addConsoleMessage("Light added at (" + std::to_string(pos.x) + ", " + 
                                std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")");
            }
            break;
        case UICommand::AddPointLight:
            {
                // Add a point light in front of the camera
                const CameraState& camState = m_camera.getCameraState();
                glm::vec3 pos = camState.position + camState.front * 2.0f;  // In front of camera
                glm::vec3 color(1.0f, 1.0f, 1.0f);  // Default white
                float intensity = 1.0f;
                m_lights.addLight(pos, color, intensity);
                addConsoleMessage("Point light added at (" + std::to_string(pos.x) + ", " + 
                                std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")");
            }
            break;
        case UICommand::AddDirectionalLight:
            {
                // Add a directional light pointing downward
                glm::vec3 direction(0.0f, -1.0f, 0.0f);  // Point downward
                glm::vec3 color(1.0f, 1.0f, 0.8f);  // Slightly warm white for sunlight
                float intensity = 3.0f;  // Stronger for directional light
                m_lights.addDirectionalLight(direction, color, intensity);
                addConsoleMessage("Directional light added (direction: " + std::to_string(direction.x) + ", " + 
                                std::to_string(direction.y) + ", " + std::to_string(direction.z) + ")");
            }
            break;
        case UICommand::AddSpotLight:
            {
                // Add a spot light in front of the camera pointing forward
                const CameraState& cam = m_camera.getCameraState();
                glm::vec3 pos = cam.position + cam.front * 2.0f;
                glm::vec3 dir = cam.front;
                glm::vec3 color(1.0f, 1.0f, 1.0f);
                float intensity = 3.0f;
                float innerDeg = 15.0f, outerDeg = 25.0f;
                m_lights.addSpotLight(pos, dir, color, intensity, innerDeg, outerDeg);
                addConsoleMessage("Spot light added");
            }
            break;
        case UICommand::SelectLight:
            {
                int lightIndex = command.intParam;
                if (lightIndex >= 0 && lightIndex < (int)m_lights.getLightCount()) {
                    m_selectedLightIndex = lightIndex;
                    addConsoleMessage("Selected light " + std::to_string(lightIndex + 1));
                } else {
                    addConsoleMessage("Invalid light index: " + std::to_string(lightIndex));
                }
            }
            break;
        case UICommand::DeleteLight:
            {
                int lightIndex = command.intParam;
                if (m_lights.removeLightAt(lightIndex)) {
                    addConsoleMessage("Deleted light " + std::to_string(lightIndex + 1));
                    if (m_selectedLightIndex == lightIndex) {
                        m_selectedLightIndex = -1;
                    } else if (m_selectedLightIndex > lightIndex) {
                        m_selectedLightIndex--;  // Adjust selection index
                    }
                } else {
                    addConsoleMessage("Failed to delete light " + std::to_string(lightIndex + 1));
                }
            }
            break;
        case UICommand::SetLightEnabled:
            {
                int idx = command.intParam;
                if (idx >= 0 && idx < (int)m_lights.m_lights.size()) {
                    m_lights.m_lights[(size_t)idx].enabled = command.boolParam;
                }
            }
            break;
        case UICommand::SetLightIntensity:
            {
                int idx = command.intParam;
                if (idx >= 0 && idx < (int)m_lights.m_lights.size()) {
                    m_lights.m_lights[(size_t)idx].intensity = command.floatParam;
                }
            }
            break;
        case UICommand::SetLightDirection:
            {
                int idx = command.intParam;
                if (idx >= 0 && idx < (int)m_lights.m_lights.size()) {
                    auto& l = m_lights.m_lights[(size_t)idx];
                    if (l.type == LightType::DIRECTIONAL || l.type == LightType::SPOT) {
                        glm::vec3 d = command.vec3Param;
                        if (glm::length(d) > 1e-4f) d = glm::normalize(d);
                        l.direction = d;
                    }
                }
            }
            break;
        case UICommand::SetLightPosition:
            {
                int idx = command.intParam;
                if (idx >= 0 && idx < (int)m_lights.m_lights.size()) {
                    auto& l = m_lights.m_lights[(size_t)idx];
                    if (l.type == LightType::POINT || l.type == LightType::SPOT) {
                        l.position = command.vec3Param;
                    }
                }
            }
            break;
        case UICommand::SetLightInnerCone:
            {
                int idx = command.intParam;
                if (idx >= 0 && idx < (int)m_lights.m_lights.size()) {
                    auto& l = m_lights.m_lights[(size_t)idx];
                    l.innerConeDeg = command.floatParam;
                    if (l.outerConeDeg < l.innerConeDeg) l.outerConeDeg = l.innerConeDeg;
                }
            }
            break;
        case UICommand::SetLightOuterCone:
            {
                int idx = command.intParam;
                if (idx >= 0 && idx < (int)m_lights.m_lights.size()) {
                    auto& l = m_lights.m_lights[(size_t)idx];
                    l.outerConeDeg = command.floatParam;
                    if (l.outerConeDeg < l.innerConeDeg) l.innerConeDeg = l.outerConeDeg;
                }
            }
            break;
        case UICommand::SetRequireRMBToMove:
            m_requireRMBToMove = command.boolParam;
            break;
            
        // UI visibility toggles
        case UICommand::ToggleSettingsPanel:
            if (m_ui) m_ui->handleCommand(command);
            addConsoleMessage("Settings panel toggled");
            break;
        case UICommand::TogglePerfHUD:
            if (m_ui) m_ui->handleCommand(command);
            addConsoleMessage("Performance HUD toggled");
            break;
        case UICommand::ToggleGrid:
            m_renderer.setShowGrid(!m_renderer.isShowGrid());
            addConsoleMessage(m_renderer.isShowGrid() ? "Grid enabled" : "Grid disabled");
            break;
        case UICommand::ToggleAxes:
            m_renderer.setShowAxes(!m_renderer.isShowAxes());
            addConsoleMessage(m_renderer.isShowAxes() ? "Axes enabled" : "Axes disabled");
            break;
        case UICommand::ToggleSkybox:
            m_renderer.setShowSkybox(!m_renderer.isShowSkybox());
            addConsoleMessage(m_renderer.isShowSkybox() ? "Skybox enabled" : "Skybox disabled");
            break;
            
        // IBL/Environment controls
        case UICommand::LoadHDREnvironment:
            {
                std::string hdrPath = command.stringParam;
                if (m_renderer.loadHDREnvironment(hdrPath)) {
                    addConsoleMessage("HDR environment loaded: " + hdrPath);
                } else {
                    addConsoleMessage("Failed to load HDR environment: " + hdrPath);
                }
            }
            break;
        case UICommand::SetSkyboxIntensity:
            {
                float intensity = command.floatParam;
                if (m_renderer.getSkybox()) {
                    m_renderer.getSkybox()->setIntensity(intensity);
                    addConsoleMessage("Skybox intensity set to " + std::to_string(intensity));
                }
            }
            break;
        case UICommand::SetIBLIntensity:
            {
                float intensity = command.floatParam;
                m_renderer.setIBLIntensity(intensity);
                addConsoleMessage("IBL intensity set to " + std::to_string(intensity));
            }
            break;
            
        // Scene operations
        case UICommand::CenterCamera:
            {
                // Reset camera to default position looking at origin
                CameraState defaultCam;
                defaultCam.position = glm::vec3(0.0f, 2.0f, 5.0f);
                defaultCam.front = glm::vec3(0.0f, 0.0f, -1.0f);
                defaultCam.up = glm::vec3(0.0f, 1.0f, 0.0f);
                m_camera.setCameraState(defaultCam);
                addConsoleMessage("Camera centered");
            }
            break;
            
        case UICommand::SetCameraPreset:
            {
                CameraPreset preset = static_cast<CameraPreset>(command.intParam);
                glm::vec3 customTarget = command.vec3Param;
                float fov = (command.floatParam > 0.0f) ? command.floatParam : Defaults::CameraPresetFovDeg;
                float margin = Defaults::CameraPresetMargin; // unified default
                
                m_camera.setCameraPreset(preset, m_scene, customTarget, fov, margin);
                
                std::string presetName = CameraController::presetName(preset);
                addConsoleMessage("Camera set to " + presetName + " preset");
            }
            break;
        case UICommand::ResetScene:
            {
                // TODO: Implement scene clearing methods in SceneManager and Light classes
                addConsoleMessage("Scene reset requested (clear methods not yet implemented)");
            }
            break;
            
        // Application control
        case UICommand::CopyShareLink:
            {
                std::string shareLink = buildShareLink();
                // TODO: Copy to clipboard - for now just log it
                addConsoleMessage("Share link: " + shareLink);
            }
            break;
        case UICommand::ExitApplication:
            // TODO: Signal application to exit
            addConsoleMessage("Exit requested");
            break;
            
        // File operations
        case UICommand::ImportAsset:
            {
                std::string filepath = FileDialog::openFile("Import Asset", FileDialog::getAssetFilters());
                if (!filepath.empty()) {
                    if (FileDialog::isSceneFile(filepath)) {
                        // Handle JSON scene file
                        std::ifstream file(filepath);
                        if (file.is_open()) {
                            std::string jsonContent((std::istreambuf_iterator<char>(file)),
                                                  std::istreambuf_iterator<char>());
                            file.close();
                            
                            std::string error;
                            bool success = applyJsonOps(jsonContent, error);
                            if (success) {
                                addConsoleMessage("Scene loaded from: " + filepath);
                                addRecentFile(filepath);
                            } else {
                                addConsoleMessage("Failed to load scene: " + error);
                            }
                        } else {
                            addConsoleMessage("Failed to open file: " + filepath);
                        }
                    } else if (FileDialog::isModelFile(filepath)) {
                        // Handle 3D model file
                        UICommandData loadCmd;
                        loadCmd.command = UICommand::LoadObject;
                        loadCmd.stringParam = filepath;
                        loadCmd.vec3Param = glm::vec3(0.0f, 0.0f, -2.0f); // Default position
                        handleLoadObject(loadCmd);
                        addRecentFile(filepath);
                    } else {
                        addConsoleMessage("Unsupported file type: " + filepath);
                    }
                } else {
                    addConsoleMessage("Import cancelled.");
                }
            }
            break;
        case UICommand::ExportScene:
            {
                std::string sceneJson = sceneToJson();
                std::string filepath = FileDialog::saveFile("Export Scene", FileDialog::getJSONFilters(), "", "scene.json");
                if (!filepath.empty()) {
                    std::ofstream file(filepath);
                    if (file.is_open()) {
                        file << sceneJson;
                        file.close();
                        addConsoleMessage("Scene exported to: " + filepath);
                    } else {
                        addConsoleMessage("Failed to save scene to: " + filepath);
                    }
                } else {
                    addConsoleMessage("Export cancelled.");
                }
            }
            break;
        case UICommand::OpenWorkspace:
            {
                std::string defaultDir = m_workspaceRoot.empty()
                    ? std::filesystem::current_path().generic_string()
                    : m_workspaceRoot.generic_string();
                std::string selected = FileDialog::selectDirectory("Open Workspace", defaultDir);
                if (selected.empty()) {
                    addConsoleMessage("Workspace selection cancelled.");
                    break;
                }
                switchWorkspace(std::filesystem::u8path(selected));
            }
            break;
        case UICommand::SaveWorkspace:
            {
                if (m_workspaceRoot.empty()) {
                    addConsoleMessage("No active workspace configured; use Open Workspace first.");
                    break;
                }
                std::string error;
                if (saveWorkspaceState(error)) {
                    addConsoleMessage("Workspace saved: " + workspaceStatePath().generic_string());
                } else {
                    addConsoleMessage("Failed to save workspace: " + error);
                }
            }
            break;

        default:
            addConsoleMessage("Unknown UI command");
            break;
    }
}

void UIBridge::handleLoadObject(const UICommandData& cmd)
{
    std::filesystem::path pathInput(cmd.stringParam);
    if (!pathInput.is_absolute()) {
        pathInput = ResourcePaths::resolve(pathInput.generic_string());
    }
    std::string pathStr = pathInput.generic_string();

    bool success = m_scene.loadObject(cmd.stringParam, pathStr, cmd.vec3Param);
    if (success) {
        addConsoleMessage("Loaded object: " + cmd.stringParam);
        // Select newly added object (assume appended)
        int newIndex = (int)m_scene.getObjects().size() - 1;
        if (newIndex >= 0) {
            m_scene.setSelectedObjectIndex(newIndex);
            // Frame the newly loaded object so it's visible
            const auto& objects = m_scene.getObjects();
            if (newIndex < (int)objects.size()) {
                const std::string& objName = objects[newIndex].name;
                // Escape the object name for JSON
                std::string escapedName = objName;
                // Replace backslashes with double backslashes for JSON
                size_t pos = 0;
                while ((pos = escapedName.find("\\", pos)) != std::string::npos) {
                    escapedName.replace(pos, 1, "\\\\");
                    pos += 2;
                }
                // Replace quotes with escaped quotes
                pos = 0;
                while ((pos = escapedName.find("\"", pos)) != std::string::npos) {
                    escapedName.replace(pos, 1, "\\\"");
                    pos += 2;
                }
                
                // Use JSON ops with properly escaped name
                std::string frameOps = "{\"ops\":[{\"op\":\"frame_object\",\"name\":\"" + escapedName + "\",\"margin\":0.25}]}";
                std::string error;
                if (m_ops && m_ops->apply(frameOps, error)) {
                    addConsoleMessage("Framed object: " + objName);
                } else {
                    addConsoleMessage("Failed to frame object: " + objName + " (" + error + ")");
                }
            }
        }
        addRecentFile(cmd.stringParam);
    } else {
        addConsoleMessage("Failed to load object: " + cmd.stringParam);
    }
}

void UIBridge::handleRenderMode(const UICommandData& cmd)
{
    m_renderer.setRenderMode(static_cast<RenderMode>(cmd.intParam));
    
    const char* modeNames[] = {"Points", "Wireframe", "Solid", "Raytrace"};
    if (cmd.intParam >= 0 && cmd.intParam < 4) {
        addConsoleMessage("Render mode: " + std::string(modeNames[cmd.intParam]));
    }
}

void UIBridge::handleCameraSettings(const UICommandData& cmd)
{
    switch (cmd.command) {
        case UICommand::SetCameraSpeed:
            m_camera.setSpeed(cmd.floatParam);
            break;
        case UICommand::SetMouseSensitivity:
            m_camera.setSensitivity(cmd.floatParam);
            break;
        default:
            break;
    }
}

void UIBridge::handleGizmoSettings(const UICommandData& cmd)
{
    switch (cmd.command) {
        case UICommand::SetGizmoMode:
            m_renderer.setGizmoMode(static_cast<GizmoMode>(cmd.intParam));
            addConsoleMessage("Gizmo mode changed");
            break;
        case UICommand::ToggleGizmoSpace:
            m_renderer.setGizmoLocalSpace(!m_renderer.gizmoLocalSpace());
            addConsoleMessage("Gizmo space toggled");
            break;
        case UICommand::ToggleSnap:
            m_renderer.setSnapEnabled(!m_renderer.snapEnabled());
            addConsoleMessage("Gizmo snap toggled");
            break;
        default:
            break;
    }
}

void UIBridge::handleConsoleCommand(const UICommandData& cmd)
{
    // Simple command parsing - could be expanded
    std::string command = cmd.stringParam;
    
    if (command == "help") {
        addConsoleMessage("=== GLINT3D HELP ===");
        addConsoleMessage("");
        addConsoleMessage("Available console commands:");
        addConsoleMessage("  help             - Show this help");
        addConsoleMessage("  clear            - Clear the console");
        addConsoleMessage("  load <path>      - Load a model (e.g., assets/models/cube.obj)");
        addConsoleMessage("  render [<out.png>] [W H] - Render PNG to path, optional size (defaults to output/renders/)");
        addConsoleMessage("  save_png [<out.png>] [W H] - Alias for render");
        addConsoleMessage("  list             - List scene objects with indices");
        addConsoleMessage("  select <name|index> - Select an object by name or index");
        addConsoleMessage("  json_ops         - Show detailed JSON Operations help");
        addConsoleMessage("");
        addConsoleMessage("JSON Operations v1.3 - Quick Reference:");
        addConsoleMessage("--- Object Management ---");
        addConsoleMessage("  load, duplicate, remove/delete, select, transform");
        addConsoleMessage("--- Camera Control ---");
        addConsoleMessage("  set_camera, set_camera_preset, orbit_camera, frame_object");
        addConsoleMessage("--- Lighting ---");
        addConsoleMessage("  add_light (point/directional/spot types)");
        addConsoleMessage("--- Materials & Appearance ---");
        addConsoleMessage("  set_material, set_background, exposure, tone_map");
        addConsoleMessage("--- Rendering ---");
        addConsoleMessage("  render_image");
        addConsoleMessage("");
        addConsoleMessage("Type 'json_ops' for detailed operation syntax and examples.");
        addConsoleMessage("See Help menu (top menu bar) for interactive guides and controls.");
        addConsoleMessage("Tips: Use Up/Down arrows to navigate command history.");
    }
    else if (command == "json_ops") {
        addConsoleMessage("JSON Operations v1.3 - Available Operations:");
        addConsoleMessage("--- Object Management ---");
        addConsoleMessage("  load             - Load models from file with transform");
        addConsoleMessage("  duplicate        - Create copies of objects with offsets");
        addConsoleMessage("  remove/delete    - Remove objects from scene (aliases)");
        addConsoleMessage("  select           - Select objects for editing");
        addConsoleMessage("  transform        - Apply transforms to objects");
        addConsoleMessage("--- Camera Control ---");
        addConsoleMessage("  set_camera       - Set camera position, target, lens");
        addConsoleMessage("  set_camera_preset - Apply presets (front,back,left,right,top,bottom,iso_fl,iso_br)");
        addConsoleMessage("  orbit_camera     - Orbit camera around center by yaw/pitch");
        addConsoleMessage("  frame_object     - Frame specific object in viewport");
        addConsoleMessage("--- Lighting ---");
        addConsoleMessage("  add_light        - Add point/directional/spot lights");
        addConsoleMessage("--- Materials & Appearance ---");
        addConsoleMessage("  set_material     - Modify object material properties");
        addConsoleMessage("  set_background   - Set background (solid/gradient/HDR stub/skybox)");
        addConsoleMessage("  exposure         - Adjust scene exposure");
        addConsoleMessage("  tone_map         - Configure tone mapping (linear/reinhard/filmic/aces)");
        addConsoleMessage("--- Rendering ---");
        addConsoleMessage("  render_image     - Render scene to PNG file");
        addConsoleMessage("");
        addConsoleMessage("See examples/json-ops/ for detailed examples and schemas/json_ops_v1.json for validation.");
        addConsoleMessage("Check Help > JSON Operations (menu bar) for interactive reference with examples.");
    }
    else if (command == "clear") {
        clearConsoleLog();
    }
    else if (command.rfind("render ", 0) == 0 || command == "render") {
        // Syntax: render [<out.png>] [W H]
        std::string args = (command == "render") ? "" : command.substr(7);
        std::istringstream ss(args);
        std::string out; int w = 1024; int h = 1024;
        ss >> out;
        
        // If no output filename provided, use default
        if (out.empty()) {
            out = RenderUtils::getDefaultOutputPath();
        } else {
            // Process the provided path
            out = RenderUtils::processOutputPath(out);
        }
        
        if (!(ss >> w)) w = 1024;
        if (!(ss >> h)) h = 1024;
        
        UICommandData r;
        r.command = UICommand::RenderToPNG;
        r.stringParam = out;
        r.intParam = w;
        r.floatParam = (float)h;
        handleUICommand(r);
    }
    else if (command.substr(0, 5) == "load ") {
        std::string path = command.substr(5);
        UICommandData loadCmd;
        loadCmd.command = UICommand::LoadObject;
        loadCmd.stringParam = path;
        loadCmd.vec3Param = glm::vec3(0.0f, 0.0f, -2.0f);
        handleLoadObject(loadCmd);
    }
    else if (command.rfind("save_png ", 0) == 0 || command == "save_png") {
        // Alias to render - Syntax: save_png [<out.png>] [W H]
        std::string args = (command == "save_png") ? "" : command.substr(9);
        std::istringstream ss(args);
        std::string out; int w = 1024; int h = 1024;
        ss >> out;
        
        // If no output filename provided, use default
        if (out.empty()) {
            out = RenderUtils::getDefaultOutputPath();
        } else {
            // Process the provided path
            out = RenderUtils::processOutputPath(out);
        }
        
        if (!(ss >> w)) w = 1024;
        if (!(ss >> h)) h = 1024;
        
        UICommandData r;
        r.command = UICommand::RenderToPNG;
        r.stringParam = out;
        r.intParam = w;
        r.floatParam = (float)h;
        handleUICommand(r);
    }
    else if (command == "list") {
        const auto& objs = m_scene.getObjects();
        addConsoleMessage("Objects:");
        for (size_t i = 0; i < objs.size(); ++i) {
            addConsoleMessage(std::to_string(i) + ": " + objs[i].name);
        }
    }
    else if (command.rfind("select ", 0) == 0) {
        std::string arg = command.substr(7);
        if (arg.empty()) { addConsoleMessage("Usage: select <name|index>"); return; }
        // Try index first
        try {
            int idx = std::stoi(arg);
            if (idx >= 0 && idx < (int)m_scene.getObjects().size()) {
                m_scene.setSelectedObjectIndex(idx);
                addConsoleMessage("Selected object at index: " + std::to_string(idx));
                return;
            }
        } catch(...) {}
        // Fallback to name
        int index = m_scene.findObjectIndex(arg);
        if (index >= 0) {
            m_scene.setSelectedObjectIndex(index);
            addConsoleMessage("Selected object: " + arg);
        } else {
            addConsoleMessage("select: object not found: " + arg);
        }
    }
    else {
        addConsoleMessage("Unknown command: " + command + " (type 'help' for commands)");
    }
}

void UIBridge::addConsoleMessage(const std::string& message)
{
    m_consoleLog.push_back(message);
    
    // Limit console log size
    const size_t maxLogSize = 1000;
    if (m_consoleLog.size() > maxLogSize) {
        m_consoleLog.erase(m_consoleLog.begin(), m_consoleLog.begin() + (m_consoleLog.size() - maxLogSize));
    }
}

void UIBridge::clearConsoleLog()
{
    m_consoleLog.clear();
    addConsoleMessage("Console cleared");
}

void UIBridge::loadRecentFiles()
{
    m_recentFiles.clear();
    std::filesystem::path recentPath = glint::getDataPath("recent.txt");
    std::ifstream in(recentPath, std::ios::in);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) m_recentFiles.push_back(line);
        if (m_recentFiles.size() >= m_recentMax) break;
    }
}

void UIBridge::saveRecentFiles() const
{
    std::filesystem::path recentPath = glint::getDataPath("recent.txt");
    std::ofstream out(recentPath, std::ios::out | std::ios::trunc);
    if (!out) return;
    size_t count = 0;
    for (const auto& p : m_recentFiles) {
        out << p << "\n";
        if (++count >= m_recentMax) break;
    }
}

void UIBridge::addRecentFile(const std::string& path)
{
    if (path.empty()) return;
    // Remove if exists
    m_recentFiles.erase(std::remove(m_recentFiles.begin(), m_recentFiles.end(), path), m_recentFiles.end());
    // Add to front
    m_recentFiles.insert(m_recentFiles.begin(), path);
    // Trim
    if (m_recentFiles.size() > m_recentMax) m_recentFiles.resize(m_recentMax);
    saveRecentFiles();
}

static std::string toLowerStr(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
static std::string basenameOnly(const std::string& path)
{
    size_t pos = path.find_last_of("/\\");
    std::string file = (pos == std::string::npos) ? path : path.substr(pos + 1);
    return file;
}

bool UIBridge::openFilePath(const std::string& path)
{
    if (path.empty()) return false;
    std::string low = toLowerStr(path);
    bool ok = false;
    auto extPos = low.find_last_of('.');
    std::string ext = (extPos == std::string::npos) ? std::string() : low.substr(extPos);
    if (ext == ".json") {
        // Load JSON ops or scene
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in) { addConsoleMessage("Cannot open: " + path); return false; }
        std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::string err;
        ok = applyJsonOps(data, err);
        if (!ok && !err.empty()) addConsoleMessage("JSON open error: " + err);
        if (ok) addConsoleMessage("Applied JSON ops from: " + path);
    }
    else if (ext == ".obj") {
        // Treat as model
        UICommandData loadCmd;
        // Name as basename
        loadCmd.command = UICommand::LoadObject;
        loadCmd.stringParam = path;
        loadCmd.vec3Param = glm::vec3(0.0f, 0.0f, -2.0f);
        handleLoadObject(loadCmd);
        ok = true; // handleLoadObject logs failure too
    } else {
        addConsoleMessage("Unsupported file type: " + ext + ". Supported: .obj, .json");
        ok = false;
    }
    if (ok) addRecentFile(path);
    return ok;
}

bool UIBridge::ensureWorkspaceFolder(std::string* errorMessage) const
{
    if (m_workspaceRoot.empty()) {
        if (errorMessage) {
            *errorMessage = "No active workspace configured.";
        }
        return false;
    }

    std::filesystem::path metadataDir = m_workspaceRoot / ".glint";
    std::error_code ec;
    std::filesystem::create_directories(metadataDir, ec);
    if (ec) {
        if (errorMessage) {
            *errorMessage = "Failed to prepare workspace metadata: " + ec.message();
        }
        return false;
    }
    return true;
}

std::filesystem::path UIBridge::workspaceStatePath() const
{
    if (m_workspaceRoot.empty()) {
        return {};
    }
    return m_workspaceRoot / ".glint" / "workspace_state.json";
}

std::string UIBridge::serializeWorkspacePath(const std::filesystem::path& path) const
{
    if (m_workspaceRoot.empty()) {
        return path.generic_string();
    }
    std::error_code ec;
    auto relative = std::filesystem::relative(path, m_workspaceRoot, ec);
    if (!ec) {
        return relative.generic_string();
    }
    return path.generic_string();
}

std::filesystem::path UIBridge::resolveWorkspacePath(const std::string& path) const
{
    std::filesystem::path candidate = std::filesystem::u8path(path);
    std::error_code ec;
    if (!candidate.is_absolute() && !m_workspaceRoot.empty()) {
        auto combined = m_workspaceRoot / candidate;
        auto canonical = std::filesystem::weakly_canonical(combined, ec);
        if (!ec) {
            return canonical;
        }
        return combined;
    }

    auto canonical = std::filesystem::weakly_canonical(candidate, ec);
    if (!ec) {
        return canonical;
    }
    return candidate;
}

void UIBridge::clearSceneForWorkspace()
{
    m_scene.clear();
    m_scene.setSelectedObjectIndex(-1);
    m_selectedLightIndex = -1;
}

bool UIBridge::saveWorkspaceState(std::string& errorMessage) const
{
    if (!ensureWorkspaceFolder(&errorMessage)) {
        return false;
    }

    using namespace rapidjson;
    Document doc;
    doc.SetObject();
    Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("version", 1, allocator);
    if (!m_workspaceRoot.empty()) {
        const std::string rootStr = m_workspaceRoot.generic_string();
        Value workspaceRootValue;
        workspaceRootValue.SetString(rootStr.c_str(), static_cast<SizeType>(rootStr.size()), allocator);
        doc.AddMember("workspace_root", workspaceRootValue, allocator);
    }

    Value objects(kArrayType);
    auto appendVec = [&](Value& parent, const char* key, const glm::vec3& vec) {
        Value arr(kArrayType);
        arr.PushBack(vec.x, allocator);
        arr.PushBack(vec.y, allocator);
        arr.PushBack(vec.z, allocator);
        parent.AddMember(Value(key, allocator), arr, allocator);
    };

    for (const auto& obj : m_scene.getObjects()) {
        if (obj.sourcePath.empty()) {
            continue;
        }

        Value entry(kObjectType);
        Value nameValue;
        nameValue.SetString(obj.name.c_str(), static_cast<SizeType>(obj.name.size()), allocator);
        entry.AddMember("name", nameValue, allocator);

        std::filesystem::path assetPath = std::filesystem::u8path(obj.sourcePath);
        std::string serializedPath = serializeWorkspacePath(assetPath);
        Value pathValue;
        pathValue.SetString(serializedPath.c_str(), static_cast<SizeType>(serializedPath.size()), allocator);
        entry.AddMember("path", pathValue, allocator);

        appendVec(entry, "translation", extractTranslation(obj.modelMatrix));
        appendVec(entry, "rotation", extractRotationDegrees(obj.modelMatrix));
        appendVec(entry, "scale", extractScale(obj.modelMatrix));

        objects.PushBack(entry, allocator);
    }
    doc.AddMember("objects", objects, allocator);

    const CameraState& camState = m_camera.getCameraState();
    Value camera(kObjectType);
    appendVec(camera, "position", camState.position);
    appendVec(camera, "front", camState.front);
    appendVec(camera, "up", camState.up);
    camera.AddMember("fov", camState.fov, allocator);
    camera.AddMember("yaw", camState.yaw, allocator);
    camera.AddMember("pitch", camState.pitch, allocator);
    doc.AddMember("camera", camera, allocator);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream stream(workspaceStatePath(), std::ios::binary | std::ios::trunc);
    if (!stream) {
        errorMessage = "Unable to open workspace state for writing.";
        return false;
    }
    stream << buffer.GetString() << '\n';
    if (!stream) {
        errorMessage = "Failed to write workspace state file.";
        return false;
    }
    return true;
}

bool UIBridge::loadWorkspaceState(std::string* errorMessage)
{
    if (!ensureWorkspaceFolder(errorMessage)) {
        return false;
    }

    const auto statePath = workspaceStatePath();
    std::error_code existsError;
    if (!std::filesystem::exists(statePath, existsError)) {
        if (errorMessage) {
            *errorMessage = "Workspace state not found at " + statePath.generic_string();
        }
        return false;
    }

    std::ifstream stream(statePath);
    if (!stream) {
        if (errorMessage) {
            *errorMessage = "Unable to open workspace state.";
        }
        return false;
    }

    rapidjson::IStreamWrapper wrapper(stream);
    rapidjson::Document doc;
    doc.ParseStream(wrapper);
    if (doc.HasParseError() || !doc.IsObject()) {
        if (errorMessage) {
            *errorMessage = "Workspace state is invalid.";
        }
        return false;
    }

    clearSceneForWorkspace();

    if (doc.HasMember("objects") && doc["objects"].IsArray()) {
        for (const auto& entry : doc["objects"].GetArray()) {
            if (!entry.IsObject() || !entry.HasMember("name") || !entry["name"].IsString()
                || !entry.HasMember("path") || !entry["path"].IsString()) {
                continue;
            }

            const char* name = entry["name"].GetString();
            const char* pathStr = entry["path"].GetString();

            glm::vec3 translation(0.0f);
            glm::vec3 rotation(0.0f);
            glm::vec3 scale(1.0f);
            if (entry.HasMember("translation") && entry["translation"].IsArray()) {
                parseVec3(entry["translation"], translation);
            }
            if (entry.HasMember("rotation") && entry["rotation"].IsArray()) {
                parseVec3(entry["rotation"], rotation);
            }
            if (entry.HasMember("scale") && entry["scale"].IsArray()) {
                parseVec3(entry["scale"], scale);
            }

            auto resolved = resolveWorkspacePath(pathStr);
            std::error_code loadError;
            if (!std::filesystem::exists(resolved, loadError)) {
                addConsoleMessage("Missing workspace asset: " + resolved.generic_string());
                continue;
            }

            if (!m_scene.loadObject(name, resolved.generic_string(), translation, scale)) {
                addConsoleMessage(std::string("Failed to load workspace object: ") + name);
                continue;
            }

            glm::mat4 transform = composeTransform(translation, rotation, scale);
            m_scene.setLocalMatrix(name, transform);
        }
    }

    if (doc.HasMember("camera") && doc["camera"].IsObject()) {
        const auto& cameraNode = doc["camera"];
        glm::vec3 position = m_camera.getCameraState().position;
        glm::vec3 front = m_camera.getCameraState().front;
        glm::vec3 up = m_camera.getCameraState().up;
        float fov = m_camera.getCameraState().fov;
        float yaw = m_camera.getCameraState().yaw;
        float pitch = m_camera.getCameraState().pitch;

        if (cameraNode.HasMember("position") && cameraNode["position"].IsArray()) {
            parseVec3(cameraNode["position"], position);
        }
        if (cameraNode.HasMember("front") && cameraNode["front"].IsArray()) {
            parseVec3(cameraNode["front"], front);
        }
        if (cameraNode.HasMember("up") && cameraNode["up"].IsArray()) {
            parseVec3(cameraNode["up"], up);
        }
        if (cameraNode.HasMember("fov") && cameraNode["fov"].IsNumber()) {
            fov = static_cast<float>(cameraNode["fov"].GetDouble());
        }
        if (cameraNode.HasMember("yaw") && cameraNode["yaw"].IsNumber()) {
            yaw = static_cast<float>(cameraNode["yaw"].GetDouble());
        }
        if (cameraNode.HasMember("pitch") && cameraNode["pitch"].IsNumber()) {
            pitch = static_cast<float>(cameraNode["pitch"].GetDouble());
        }

        CameraState camState = m_camera.getCameraState();
        camState.position = position;
        camState.front = front;
        camState.up = up;
        camState.fov = fov;
        camState.yaw = yaw;
        camState.pitch = pitch;
        m_camera.setCameraState(camState);
    }

    return true;
}

bool UIBridge::switchWorkspace(const std::filesystem::path& newRoot)
{
    std::error_code existsError;
    if (newRoot.empty() || !std::filesystem::exists(newRoot, existsError)) {
        addConsoleMessage("Workspace path does not exist: " + newRoot.generic_string());
        return false;
    }
    if (!std::filesystem::exists(newRoot / "glint.project.json", existsError)) {
        addConsoleMessage("glint.project.json not found in workspace: " + newRoot.generic_string());
        return false;
    }

    setWorkspaceRoot(newRoot);

    std::error_code cwdError;
    std::filesystem::current_path(m_workspaceRoot, cwdError);

    std::string error;
    bool loaded = loadWorkspaceState(&error);
    if (loaded) {
        addConsoleMessage("Loaded workspace: " + m_workspaceRoot.generic_string());
    } else {
        if (!error.empty() && error.find("not found") == std::string::npos) {
            addConsoleMessage(error);
        }
        clearSceneForWorkspace();
        addConsoleMessage("Workspace ready: " + m_workspaceRoot.generic_string());
    }
    return true;
}

bool UIBridge::applyJsonOps(const std::string& json, std::string& error)
{
    // Delegate to the shared JsonOpsExecutor. Keep legacy code below for now.
    if (m_ops) {
        bool ok = m_ops->apply(json, error);
        if (!ok && !error.empty()) {
            addConsoleMessage("JSON Ops error: " + error);
        }
        return ok;
    }
    error = "JSON ops executor unavailable";
    return false;
}

std::string UIBridge::buildShareLink() const
{
    using namespace rapidjson;
    
    // Helper function for URL-safe base64 encoding (RFC 4648 §5)
    auto base64UrlEncode = [](const std::string& input) -> std::string {
        static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string b64;
        int val = 0, valb = -6;
        for (unsigned char c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                b64.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) b64.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        // Transform to URL-safe and strip padding
        for (auto& ch : b64) { if (ch=='+') ch='-'; else if (ch=='/') ch='_'; }
        // Optional: remove '=' padding for cleaner URLs
        while (!b64.empty() && b64.back()=='=') b64.pop_back();
        return b64;
    };
    
    // Build state JSON containing scene data and camera
    Document state;
    state.SetObject();
    Document::AllocatorType& allocator = state.GetAllocator();
    
    // Add camera state
    Value camera(kObjectType);
    CameraState camState = m_camera.getCameraState();
    
    Value position(kArrayType);
    position.PushBack(camState.position.x, allocator);
    position.PushBack(camState.position.y, allocator);
    position.PushBack(camState.position.z, allocator);
    camera.AddMember("position", position, allocator);
    
    Value front(kArrayType);
    front.PushBack(camState.front.x, allocator);
    front.PushBack(camState.front.y, allocator);
    front.PushBack(camState.front.z, allocator);
    camera.AddMember("front", front, allocator);
    
    Value up(kArrayType);
    up.PushBack(camState.up.x, allocator);
    up.PushBack(camState.up.y, allocator);
    up.PushBack(camState.up.z, allocator);
    camera.AddMember("up", up, allocator);
    
    camera.AddMember("fov", camState.fov, allocator);
    camera.AddMember("near", camState.nearClip, allocator);
    camera.AddMember("far", camState.farClip, allocator);
    
    state.AddMember("camera", camera, allocator);
    
    // Add lights
    Value lights(kArrayType);
    for (size_t i = 0; i < m_lights.getLightCount(); ++i) {
        const auto& light = m_lights.m_lights[i];
        Value lightObj(kObjectType);
        
        Value lightPos(kArrayType);
        lightPos.PushBack(light.position.x, allocator);
        lightPos.PushBack(light.position.y, allocator);
        lightPos.PushBack(light.position.z, allocator);
        lightObj.AddMember("position", lightPos, allocator);
        
        Value lightColor(kArrayType);
        lightColor.PushBack(light.color.x, allocator);
        lightColor.PushBack(light.color.y, allocator);
        lightColor.PushBack(light.color.z, allocator);
        lightObj.AddMember("color", lightColor, allocator);
        
        lightObj.AddMember("intensity", light.intensity, allocator);
        
        lights.PushBack(lightObj, allocator);
    }
    state.AddMember("lights", lights, allocator);
    
    // Create ops array for reconstruction
    Value ops(kArrayType);
    
    // Add camera set operation
    Value setCameraOp(kObjectType);
    setCameraOp.AddMember("op", "set_camera", allocator);
    setCameraOp.AddMember("position", Value(position, allocator), allocator);
    setCameraOp.AddMember("front", Value(front, allocator), allocator);
    setCameraOp.AddMember("up", Value(up, allocator), allocator);
    setCameraOp.AddMember("fov", camState.fov, allocator);
    setCameraOp.AddMember("near", camState.nearClip, allocator);
    setCameraOp.AddMember("far", camState.farClip, allocator);
    ops.PushBack(setCameraOp, allocator);
    
    // Add light operations
    for (size_t i = 0; i < m_lights.getLightCount(); ++i) {
        const auto& light = m_lights.m_lights[i];
        Value addLightOp(kObjectType);
        addLightOp.AddMember("op", "add_light", allocator);
        
        // Add light type
        if (light.type == LightType::POINT) {
            addLightOp.AddMember("type", "point", allocator);
            
            Value lightPos(kArrayType);
            lightPos.PushBack(light.position.x, allocator);
            lightPos.PushBack(light.position.y, allocator);
            lightPos.PushBack(light.position.z, allocator);
            addLightOp.AddMember("position", lightPos, allocator);
        } else if (light.type == LightType::DIRECTIONAL) {
            addLightOp.AddMember("type", "directional", allocator);
            
            Value lightDir(kArrayType);
            lightDir.PushBack(light.direction.x, allocator);
            lightDir.PushBack(light.direction.y, allocator);
            lightDir.PushBack(light.direction.z, allocator);
            addLightOp.AddMember("direction", lightDir, allocator);
        } else if (light.type == LightType::SPOT) {
            addLightOp.AddMember("type", "spot", allocator);

            Value lightPos(kArrayType);
            lightPos.PushBack(light.position.x, allocator);
            lightPos.PushBack(light.position.y, allocator);
            lightPos.PushBack(light.position.z, allocator);
            addLightOp.AddMember("position", lightPos, allocator);

            Value lightDir(kArrayType);
            lightDir.PushBack(light.direction.x, allocator);
            lightDir.PushBack(light.direction.y, allocator);
            lightDir.PushBack(light.direction.z, allocator);
            addLightOp.AddMember("direction", lightDir, allocator);
            addLightOp.AddMember("inner_deg", light.innerConeDeg, allocator);
            addLightOp.AddMember("outer_deg", light.outerConeDeg, allocator);
        }
        
        Value lightColor(kArrayType);
        lightColor.PushBack(light.color.x, allocator);
        lightColor.PushBack(light.color.y, allocator);
        lightColor.PushBack(light.color.z, allocator);
        addLightOp.AddMember("color", lightColor, allocator);
        
        addLightOp.AddMember("intensity", light.intensity, allocator);
        
        ops.PushBack(addLightOp, allocator);
    }
    
    // Add object load operations
    const auto& objects = m_scene.getObjects();
    for (const auto& obj : objects) {
        Value loadOp(kObjectType);
        loadOp.AddMember("op", "load", allocator);
        
        Value name(obj.name.c_str(), allocator);
        loadOp.AddMember("name", name, allocator);
        
        // Note: Original path is not persisted; using name as placeholder for now
        Value path(obj.name.c_str(), allocator);
        loadOp.AddMember("path", path, allocator);
        
        // Extract transform
        Value transform(kObjectType);
        glm::vec3 pos = glm::vec3(obj.modelMatrix[3]);
        Value objPos(kArrayType);
        objPos.PushBack(pos.x, allocator);
        objPos.PushBack(pos.y, allocator);
        objPos.PushBack(pos.z, allocator);
        transform.AddMember("position", objPos, allocator);
        
        glm::vec3 scale(
            glm::length(glm::vec3(obj.modelMatrix[0])),
            glm::length(glm::vec3(obj.modelMatrix[1])),
            glm::length(glm::vec3(obj.modelMatrix[2]))
        );
        Value objScale(kArrayType);
        objScale.PushBack(scale.x, allocator);
        objScale.PushBack(scale.y, allocator);
        objScale.PushBack(scale.z, allocator);
        transform.AddMember("scale", objScale, allocator);
        
        loadOp.AddMember("transform", transform, allocator);
        
        ops.PushBack(loadOp, allocator);

        // Also export material properties so share-links restore appearance
        Value matOp(kObjectType);
        matOp.AddMember("op", "set_material", allocator);
        matOp.AddMember("target", Value(obj.name.c_str(), allocator), allocator);
        Value mat(kObjectType);
        // Color from baseColorFactor if set, else diffuse
        Value colorArr(kArrayType);
        glm::vec3 color = glm::vec3(obj.baseColorFactor);
        if (color == glm::vec3(1.0f) && obj.material.diffuse != glm::vec3(1.0f)) {
            color = obj.material.diffuse;
        }
        colorArr.PushBack(color.x, allocator);
        colorArr.PushBack(color.y, allocator);
        colorArr.PushBack(color.z, allocator);
        mat.AddMember("color", colorArr, allocator);
        // Roughness / Metallic if available
        mat.AddMember("roughness", obj.material.roughness, allocator);
        mat.AddMember("metallic", obj.material.metallic, allocator);
        // Specular / Ambient (legacy phong)
        Value specArr(kArrayType); specArr.PushBack(obj.material.specular.x, allocator); specArr.PushBack(obj.material.specular.y, allocator); specArr.PushBack(obj.material.specular.z, allocator);
        mat.AddMember("specular", specArr, allocator);
        Value ambArr(kArrayType); ambArr.PushBack(obj.material.ambient.x, allocator); ambArr.PushBack(obj.material.ambient.y, allocator); ambArr.PushBack(obj.material.ambient.z, allocator);
        mat.AddMember("ambient", ambArr, allocator);
        matOp.AddMember("material", mat, allocator);
        ops.PushBack(matOp, allocator);
    }
    
    state.AddMember("ops", ops, allocator);
    
    // Convert to JSON string
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    state.Accept(writer);
    
    // Encode as base64
    std::string stateJson = buffer.GetString();
    std::string encoded = base64UrlEncode(stateJson);
    
    // Build shareable URL (this could be configured)
    return "https://glint3d.com/viewer?state=" + encoded;
}

std::string UIBridge::sceneToJson() const
{
    return m_scene.toJson();
}
