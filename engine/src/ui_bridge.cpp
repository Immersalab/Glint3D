#include "ui_bridge.h"
#include "scene_manager.h"
#include "render_system.h"
#include "camera_controller.h"
#include "light.h"
#include "json_ops.h"
#include "config_defaults.h"
#include "render_utils.h"
#include "help_text.h"
#include <iostream>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

UIBridge::UIBridge(SceneManager& scene, RenderSystem& renderer, 
                   CameraController& camera, Light& lights)
    : m_scene(scene)
    , m_renderer(renderer)
    , m_camera(camera)
    , m_lights(lights)
{
    // Default AI endpoint for local Ollama
    m_aiEndpoint = "http://127.0.0.1:11434";
    // Initialize modular JSON ops executor for shared implementation
    m_ops = std::make_unique<JsonOpsExecutor>(m_scene, m_renderer, m_camera, m_lights);
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
    
    // Camera state
    state.camera = m_camera.getCameraState();
    state.cameraSpeed = m_camera.getSpeed();
    state.sensitivity = m_camera.getSensitivity();
    
    // Render state
    state.renderMode = m_renderer.getRenderMode();
    state.shadingMode = m_renderer.getShadingMode();
    state.framebufferSRGBEnabled = m_renderer.isFramebufferSRGBEnabled();
    state.denoiseEnabled = m_renderer.isDenoiseEnabled();
    state.showGrid = m_renderer.isShowGrid();
    state.showAxes = m_renderer.isShowAxes();
    state.showSkybox = m_renderer.isShowSkybox();
    state.requireRMBToMove = m_requireRMBToMove;
    
    // Scene state
    state.selectedObjectIndex = m_scene.getSelectedObjectIndex();
    state.selectedObjectName = m_scene.getSelectedObjectName();
    state.objectCount = (int)m_scene.getObjects().size();
    
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
    
    // AI
    state.useAI = m_useAI;
    state.aiEndpoint = m_aiEndpoint;
    
    return state;
}

void UIBridge::handleUICommand(const UICommandData& command)
{
    switch (command.command) {
        case UICommand::LoadObject:
            handleLoadObject(command);
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
        case UICommand::SetUseAI:
            m_useAI = command.boolParam;
            break;
        case UICommand::SetAIEndpoint:
            m_aiEndpoint = command.stringParam;
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
        case UICommand::ImportModel:
            addConsoleMessage("Import Model: File dialog not yet implemented on desktop. Use console command: load <path>");
            break;
        case UICommand::ImportSceneJSON:
            addConsoleMessage("Import Scene JSON: File dialog not yet implemented on desktop. Use console JSON ops.");
            break;
        case UICommand::ExportScene:
            {
                std::string sceneJson = sceneToJson();
                // TODO: Save to file with dialog - for now just show in console
                addConsoleMessage("Scene export (copy from console):");
                addConsoleMessage(sceneJson);
            }
            break;

        default:
            addConsoleMessage("Unknown UI command");
            break;
    }
}

void UIBridge::handleLoadObject(const UICommandData& cmd)
{
    bool success = m_scene.loadObject(cmd.stringParam, cmd.stringParam, cmd.vec3Param);
    if (success) {
        addConsoleMessage("Loaded object: " + cmd.stringParam);
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
        addConsoleMessage("  render [<out.png>] [W H] - Render PNG to path, optional size (defaults to renders/ folder)");
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
        addConsoleMessage("  set_background   - Set background color or skybox");
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
    
    // Helper function for base64 encoding
    auto base64Encode = [](const std::string& input) -> std::string {
        static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        int val = 0, valb = -6;
        for (unsigned char c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                encoded.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) encoded.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (encoded.size() % 4) encoded.push_back('=');
        return encoded;
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
        
        // Note: We don't have the original path stored, so we use name as placeholder
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
    }
    
    state.AddMember("ops", ops, allocator);
    
    // Convert to JSON string
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    state.Accept(writer);
    
    // Encode as base64
    std::string stateJson = buffer.GetString();
    std::string encoded = base64Encode(stateJson);
    
    // Build shareable URL (this could be configured)
    return "https://glint3d.com/viewer?state=" + encoded;
}

std::string UIBridge::sceneToJson() const
{
    return m_scene.toJson();
}
