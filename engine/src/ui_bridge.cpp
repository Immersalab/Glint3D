#include "ui_bridge.h"
#include "scene_manager.h"
#include "render_system.h"
#include "camera_controller.h"
#include "light.h"
#include "json_ops.h"
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
        addConsoleMessage("Welcome to Glint3D!");
        addConsoleMessage("Type 'help' for a list of console commands.");
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
    state.requireRMBToMove = m_requireRMBToMove;
    
    // Scene state
    state.selectedObjectIndex = m_scene.getSelectedObjectIndex();
    state.selectedObjectName = m_scene.getSelectedObjectName();
    state.objectCount = (int)m_scene.getObjects().size();
    
    // Light state  
    state.lightCount = (int)m_lights.getLightCount();
    state.selectedLightIndex = m_selectedLightIndex;
    
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
            // TODO: Implement grid toggle in renderer
            addConsoleMessage("Grid toggle requested (not yet implemented)");
            break;
        case UICommand::ToggleAxes:
            // TODO: Implement axes toggle in renderer
            addConsoleMessage("Axes toggle requested (not yet implemented)");
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
        addConsoleMessage("Available commands:");
        addConsoleMessage("  help             - Show this help");
        addConsoleMessage("  clear            - Clear the console");
        addConsoleMessage("  load <path>      - Load a model (e.g., assets/models/cube.obj)");
        addConsoleMessage("  render <out.png> [W H] - Render PNG to path, optional size");
        addConsoleMessage("Tips: Use Up/Down arrows to navigate command history.");
    }
    else if (command == "clear") {
        clearConsoleLog();
    }
    else if (command.rfind("render ", 0) == 0) {
        // Syntax: render <out.png> [W H]
        std::istringstream ss(command.substr(7));
        std::string out; int w = 1024; int h = 1024;
        ss >> out;
        if (!out.empty()) {
            if (!(ss >> w)) w = 1024;
            if (!(ss >> h)) h = 1024;
            UICommandData r;
            r.command = UICommand::RenderToPNG;
            r.stringParam = out;
            r.intParam = w;
            r.floatParam = (float)h;
            handleUICommand(r);
        } else {
            addConsoleMessage("Usage: render <out.png> [W H]");
        }
    }
    else if (command.substr(0, 5) == "load ") {
        std::string path = command.substr(5);
        UICommandData loadCmd;
        loadCmd.command = UICommand::LoadObject;
        loadCmd.stringParam = path;
        loadCmd.vec3Param = glm::vec3(0.0f, 0.0f, -2.0f);
        handleLoadObject(loadCmd);
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
        
        Value lightPos(kArrayType);
        lightPos.PushBack(light.position.x, allocator);
        lightPos.PushBack(light.position.y, allocator);
        lightPos.PushBack(light.position.z, allocator);
        addLightOp.AddMember("position", lightPos, allocator);
        
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
