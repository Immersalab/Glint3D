#include "ui_bridge.h"
#include "scene_manager.h"
#include "render_system.h"
#include "camera_controller.h"
#include "light.h"
#include <iostream>
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
    
    return m_ui->init(windowWidth, windowHeight);
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
        case UICommand::SetRequireRMBToMove:
            m_requireRMBToMove = command.boolParam;
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
        addConsoleMessage("Available commands: help, clear, render, load");
    }
    else if (command == "clear") {
        clearConsoleLog();
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
    using namespace rapidjson;
    error.clear();

    Document d;
    ParseResult ok = d.Parse(json.c_str());
    if (!ok) {
        error = std::string("JSON parse error: ") + GetParseError_En(ok.Code()) +
                " at offset " + std::to_string(ok.Offset());
        return false;
    }

    auto getVec3 = [](const Value& v, glm::vec3& out)->bool {
        if (!v.IsArray() || v.Size() != 3) return false;
        for (rapidjson::SizeType i = 0; i < 3; ++i) if (!v[i].IsNumber()) return false;
        out = glm::vec3((float)v[0].GetDouble(), (float)v[1].GetDouble(), (float)v[2].GetDouble());
        return true;
    };

    auto deriveNameFromPath = [](const std::string& path){
        std::string name = path;
        auto slash = name.find_last_of("/\\"); if (slash != std::string::npos) name = name.substr(slash+1);
        auto dot = name.find_last_of('.'); if (dot != std::string::npos) name = name.substr(0, dot);
        if (name.empty()) name = "Object";
        return name;
    };

    auto applyOp = [&](const Value& obj, int index)->bool {
        if (!obj.IsObject()) { error = "op at index " + std::to_string(index) + " is not an object"; return false; }
        if (!obj.HasMember("op") || !obj["op"].IsString()) { error = "missing 'op' at index " + std::to_string(index); return false; }
        std::string op = obj["op"].GetString();

        if (op == "load") {
            if (!obj.HasMember("path") || !obj["path"].IsString()) { error = "load: missing 'path'"; return false; }
            std::string path = obj["path"].GetString();
            std::string name;
            if (obj.HasMember("name") && obj["name"].IsString()) name = obj["name"].GetString();
            if (name.empty()) name = deriveNameFromPath(path);
            glm::vec3 pos(0.0f), scale(1.0f);
            if (obj.HasMember("position") && obj["position"].IsArray()) {
                if (!getVec3(obj["position"], pos)) { error = "load: bad 'position'"; return false; }
            }
            if (obj.HasMember("scale") && obj["scale"].IsArray()) {
                if (!getVec3(obj["scale"], scale)) { error = "load: bad 'scale'"; return false; }
            }
            if (obj.HasMember("transform") && obj["transform"].IsObject()) {
                const auto& t = obj["transform"];
                if (t.HasMember("position") && t["position"].IsArray()) {
                    if (!getVec3(t["position"], pos)) { error = "load: bad transform.position"; return false; }
                }
                if (t.HasMember("scale") && t["scale"].IsArray()) {
                    if (!getVec3(t["scale"], scale)) { error = "load: bad transform.scale"; return false; }
                }
            }
            bool okLoad = m_scene.loadObject(name, path, pos, scale);
            if (!okLoad) { error = std::string("load failed for '") + name + "'"; return false; }
            addConsoleMessage("Loaded object: " + name);
            return true;
        }
        else if (op == "set_camera") {
            glm::vec3 pos(0.0f), up(0,1,0), target(0.0f), front(0.0f,0.0f,-1.0f);
            bool hasPos=false, hasTarget=false, hasFront=false, hasUp=false;
            if (obj.HasMember("position") && obj["position"].IsArray()) { if (!getVec3(obj["position"], pos)) { error="set_camera: bad 'position'"; return false; } hasPos=true; }
            if (obj.HasMember("target") && obj["target"].IsArray()) { if (!getVec3(obj["target"], target)) { error="set_camera: bad 'target'"; return false; } hasTarget=true; }
            if (obj.HasMember("front") && obj["front"].IsArray()) { if (!getVec3(obj["front"], front)) { error="set_camera: bad 'front'"; return false; } hasFront=true; }
            if (obj.HasMember("up") && obj["up"].IsArray()) { if (!getVec3(obj["up"], up)) { error="set_camera: bad 'up'"; return false; } hasUp=true; }
            if (!hasPos) { error = "set_camera: missing 'position'"; return false; }

            if (hasTarget) {
                m_camera.setTarget(pos, target, hasUp ? up : glm::vec3(0,1,0));
            } else if (hasFront) {
                m_camera.setFrontUp(pos, front, hasUp ? up : glm::vec3(0,1,0));
            } else {
                CameraState cs = m_camera.getCameraState(); cs.position = pos; m_camera.setCameraState(cs);
            }
            // Lens
            float fov = m_camera.getCameraState().fov;
            float nz = m_camera.getCameraState().nearClip;
            float fz = m_camera.getCameraState().farClip;
            if (obj.HasMember("fov")) { if (!obj["fov"].IsNumber()) { error = "set_camera: bad 'fov'"; return false; } fov = (float)obj["fov"].GetDouble(); }
            if (obj.HasMember("fov_deg")) { if (!obj["fov_deg"].IsNumber()) { error = "set_camera: bad 'fov_deg'"; return false; } fov = (float)obj["fov_deg"].GetDouble(); }
            if (obj.HasMember("near")) { if (!obj["near"].IsNumber()) { error = "set_camera: bad 'near'"; return false; } nz = (float)obj["near"].GetDouble(); }
            if (obj.HasMember("far"))  { if (!obj["far"].IsNumber())  { error = "set_camera: bad 'far'";  return false; } fz = (float)obj["far"].GetDouble(); }
            m_camera.setLens(fov, nz, fz);

            // Sync renderer now
            m_renderer.setCamera(m_camera.getCameraState());
            m_renderer.updateViewMatrix();
            addConsoleMessage("Camera updated");
            return true;
        }
        else if (op == "add_light") {
            glm::vec3 pos(0.0f), color(1.0f);
            double intensity = 1.0;
            if (obj.HasMember("position") && obj["position"].IsArray()) { if (!getVec3(obj["position"], pos)) { error = "add_light: bad 'position'"; return false; } }
            if (obj.HasMember("color") && obj["color"].IsArray()) { if (!getVec3(obj["color"], color)) { error = "add_light: bad 'color'"; return false; } }
            if (obj.HasMember("intensity")) { if (!obj["intensity"].IsNumber()) { error = "add_light: bad 'intensity'"; return false; } intensity = obj["intensity"].GetDouble(); }
            m_lights.addLight(pos, color, (float)intensity);
            addConsoleMessage("Light added");
            return true;
        }
        else if (op == "duplicate") {
            if (!obj.HasMember("name") || !obj["name"].IsString()) { error = "duplicate: missing 'name'"; return false; }
            std::string srcName = obj["name"].GetString();
            
            const auto* srcObj = m_scene.findObjectByName(srcName);
            if (!srcObj) { error = "duplicate: object '" + srcName + "' not found"; return false; }
            
            std::string newName = srcName + "_copy";
            if (obj.HasMember("newName") && obj["newName"].IsString()) {
                newName = obj["newName"].GetString();
            }
            
            glm::vec3 offset(1.0f, 0.0f, 0.0f);
            if (obj.HasMember("offset") && obj["offset"].IsArray()) {
                if (!getVec3(obj["offset"], offset)) { error = "duplicate: bad 'offset'"; return false; }
            }
            
            glm::vec3 currentPos = glm::vec3(srcObj->modelMatrix[3]);
            glm::vec3 newPos = currentPos + offset;
            
            // Create new object with offset position
            bool success = m_scene.duplicateObject(srcName, newName, newPos);
            if (success) {
                addConsoleMessage("Duplicated object: " + srcName + " -> " + newName);
            } else {
                error = "duplicate: failed to duplicate object";
                return false;
            }
            return true;
        }
        else if (op == "delete") {
            if (!obj.HasMember("name") || !obj["name"].IsString()) { error = "delete: missing 'name'"; return false; }
            std::string name = obj["name"].GetString();
            
            bool success = m_scene.deleteObject(name);
            if (success) {
                addConsoleMessage("Deleted object: " + name);
            } else {
                error = "delete: object '" + name + "' not found";
                return false;
            }
            return true;
        }
        else if (op == "transform") {
            if (!obj.HasMember("name") || !obj["name"].IsString()) { error = "transform: missing 'name'"; return false; }
            std::string name = obj["name"].GetString();
            
            auto* targetObj = m_scene.findObjectByName(name);
            if (!targetObj) { error = "transform: object '" + name + "' not found"; return false; }
            
            glm::mat4 transform = targetObj->modelMatrix;
            
            if (obj.HasMember("translate") && obj["translate"].IsArray()) {
                glm::vec3 translate;
                if (!getVec3(obj["translate"], translate)) { error = "transform: bad 'translate'"; return false; }
                transform = glm::translate(transform, translate);
            }
            
            if (obj.HasMember("rotate") && obj["rotate"].IsArray()) {
                const auto& rotArray = obj["rotate"];
                if (!rotArray.IsArray() || rotArray.Size() != 3) { error = "transform: bad 'rotate'"; return false; }
                
                float rotX = (float)rotArray[0].GetDouble();
                float rotY = (float)rotArray[1].GetDouble();
                float rotZ = (float)rotArray[2].GetDouble();
                
                transform = glm::rotate(transform, glm::radians(rotX), glm::vec3(1, 0, 0));
                transform = glm::rotate(transform, glm::radians(rotY), glm::vec3(0, 1, 0));
                transform = glm::rotate(transform, glm::radians(rotZ), glm::vec3(0, 0, 1));
            }
            
            if (obj.HasMember("scale") && obj["scale"].IsArray()) {
                glm::vec3 scale;
                if (!getVec3(obj["scale"], scale)) { error = "transform: bad 'scale'"; return false; }
                transform = glm::scale(transform, scale);
            }
            
            if (obj.HasMember("setPosition") && obj["setPosition"].IsArray()) {
                glm::vec3 newPos;
                if (!getVec3(obj["setPosition"], newPos)) { error = "transform: bad 'setPosition'"; return false; }
                transform[3] = glm::vec4(newPos, 1.0f);
            }
            
            // Apply the transformation
            const_cast<SceneObject*>(targetObj)->modelMatrix = transform;
            addConsoleMessage("Transformed object: " + name);
            return true;
        }
        else if (op == "select") {
            if (obj.HasMember("name") && obj["name"].IsString()) {
                std::string name = obj["name"].GetString();
                int index = m_scene.findObjectIndex(name);
                if (index >= 0) {
                    m_scene.setSelectedObjectIndex(index);
                    addConsoleMessage("Selected object: " + name);
                } else {
                    error = "select: object '" + name + "' not found";
                    return false;
                }
            } else if (obj.HasMember("index") && obj["index"].IsInt()) {
                int index = obj["index"].GetInt();
                if (index >= 0 && index < (int)m_scene.getObjects().size()) {
                    m_scene.setSelectedObjectIndex(index);
                    addConsoleMessage("Selected object at index: " + std::to_string(index));
                } else {
                    error = "select: index out of range";
                    return false;
                }
            } else {
                error = "select: missing 'name' or 'index'";
                return false;
            }
            return true;
        }

        error = std::string("unknown op '") + op + "' at index " + std::to_string(index);
        return false;
    };

    // Accept: array of ops; single op object; or envelope { "ops": [...] }
    if (d.IsArray()) {
        for (rapidjson::SizeType i = 0; i < d.Size(); ++i) {
            if (!applyOp(d[i], (int)i)) return false;
        }
        return true;
    } else if (d.IsObject()) {
        if (d.HasMember("ops") && d["ops"].IsArray()) {
            const auto& arr = d["ops"];
            for (rapidjson::SizeType i = 0; i < arr.Size(); ++i) {
                if (!applyOp(arr[i], (int)i)) return false;
            }
            return true;
        } else {
            return applyOp(d, 0);
        }
    }

    error = "root must be array or object";
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
