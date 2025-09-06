#include "json_ops.h"

#include "scene_manager.h"
#include "render_system.h"
#include "camera_controller.h"
#include "config_defaults.h"
#include "light.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <algorithm>
#include <cctype>
#include <limits>

namespace {
    static bool getVec3(const rapidjson::Value& v, glm::vec3& out) {
        if (!v.IsArray() || v.Size() != 3) return false;
        for (rapidjson::SizeType i = 0; i < 3; ++i) if (!v[i].IsNumber()) return false;
        out = glm::vec3((float)v[0].GetDouble(), (float)v[1].GetDouble(), (float)v[2].GetDouble());
        return true;
    }

    static std::string deriveNameFromPath(const std::string& path) {
        std::string name = path;
        auto slash = name.find_last_of("/\\"); if (slash != std::string::npos) name = name.substr(slash+1);
        auto dot = name.find_last_of('.'); if (dot != std::string::npos) name = name.substr(0, dot);
        if (name.empty()) name = "Object";
        return name;
    }
}

JsonOpsExecutor::JsonOpsExecutor(SceneManager& scene,
                                 RenderSystem& renderer,
                                 CameraController& camera,
                                 Light& lights)
    : m_scene(scene)
    , m_renderer(renderer)
    , m_camera(camera)
    , m_lights(lights)
{}

bool JsonOpsExecutor::apply(const std::string& json, std::string& error)
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

    // Inner lambda that applies a single op object
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
            return true;
        }
        else if (op == "set_camera_preset") {
            // expected fields: preset (string), optional target (vec3), optional fov (number), optional margin (number)
            if (!obj.HasMember("preset") || !obj["preset"].IsString()) { error = "set_camera_preset: missing 'preset'"; return false; }
            std::string presetStr = obj["preset"].GetString();
            std::string lower;
            lower.resize(presetStr.size());
            std::transform(presetStr.begin(), presetStr.end(), lower.begin(), [](unsigned char c){ return (char)std::tolower(c); });

            CameraPreset preset = CameraPreset::Front;
            if (lower == "front") preset = CameraPreset::Front;
            else if (lower == "back") preset = CameraPreset::Back;
            else if (lower == "left") preset = CameraPreset::Left;
            else if (lower == "right") preset = CameraPreset::Right;
            else if (lower == "top") preset = CameraPreset::Top;
            else if (lower == "bottom") preset = CameraPreset::Bottom;
            else if (lower == "iso_fl" || lower == "isofl" || lower == "iso-front-left" || lower == "iso-fl") preset = CameraPreset::IsoFL;
            else if (lower == "iso_br" || lower == "isobr" || lower == "iso-back-right" || lower == "iso-br") preset = CameraPreset::IsoBR;
            else { error = "set_camera_preset: unknown preset '" + presetStr + "'"; return false; }

            glm::vec3 target(0.0f);
            if (obj.HasMember("target") && obj["target"].IsArray()) {
                if (!getVec3(obj["target"], target)) { error = "set_camera_preset: bad 'target'"; return false; }
            }

            float fov = Defaults::CameraPresetFovDeg;
            if (obj.HasMember("fov") && obj["fov"].IsNumber()) fov = (float)obj["fov"].GetDouble();
            float margin = Defaults::CameraPresetMargin;
            if (obj.HasMember("margin") && obj["margin"].IsNumber()) margin = (float)obj["margin"].GetDouble();

            m_camera.setCameraPreset(preset, m_scene, target, fov, margin);
            return true;
        }
        else if (op == "add_light") {
            // Default light type is point light for backward compatibility
            std::string lightType = "point";
            if (obj.HasMember("type") && obj["type"].IsString()) {
                lightType = obj["type"].GetString();
                if (lightType != "point" && lightType != "directional" && lightType != "spot") {
                    error = "add_light: invalid type '" + lightType + "' (must be 'point', 'directional', or 'spot')";
                    return false;
                }
            }
            
            glm::vec3 pos(0.0f), dir(0.0f, -1.0f, 0.0f), color(1.0f);
            double intensity = 1.0;
            
            if (obj.HasMember("color") && obj["color"].IsArray()) { 
                if (!getVec3(obj["color"], color)) { error = "add_light: bad 'color'"; return false; } 
            }
            if (obj.HasMember("intensity")) { 
                if (!obj["intensity"].IsNumber()) { error = "add_light: bad 'intensity'"; return false; } 
                intensity = obj["intensity"].GetDouble(); 
            }
            
            if (lightType == "point") {
                if (obj.HasMember("position") && obj["position"].IsArray()) { 
                    if (!getVec3(obj["position"], pos)) { error = "add_light: bad 'position'"; return false; } 
                }
                m_lights.addLight(pos, color, (float)intensity);
            } else if (lightType == "directional") {
                if (obj.HasMember("direction") && obj["direction"].IsArray()) { 
                    if (!getVec3(obj["direction"], dir)) { error = "add_light: bad 'direction'"; return false; } 
                }
                m_lights.addDirectionalLight(dir, color, (float)intensity);
            } else if (lightType == "spot") {
                if (obj.HasMember("position") && obj["position"].IsArray()) { 
                    if (!getVec3(obj["position"], pos)) { error = "add_light: bad 'position'"; return false; } 
                } else { error = "add_light: missing 'position' for spot"; return false; }
                if (obj.HasMember("direction") && obj["direction"].IsArray()) { 
                    if (!getVec3(obj["direction"], dir)) { error = "add_light: bad 'direction'"; return false; } 
                } else { error = "add_light: missing 'direction' for spot"; return false; }
                double innerDeg = 15.0, outerDeg = 25.0;
                if (obj.HasMember("inner_deg")) { 
                    if (!obj["inner_deg"].IsNumber()) { error = "add_light: bad 'inner_deg'"; return false; } 
                    innerDeg = obj["inner_deg"].GetDouble(); 
                }
                if (obj.HasMember("outer_deg")) { 
                    if (!obj["outer_deg"].IsNumber()) { error = "add_light: bad 'outer_deg'"; return false; } 
                    outerDeg = obj["outer_deg"].GetDouble(); 
                }
                if (outerDeg < innerDeg) std::swap(outerDeg, innerDeg);
                m_lights.addSpotLight(pos, dir, color, (float)intensity, (float)innerDeg, (float)outerDeg);
            }
            return true;
        }
        else if (op == "set_material") {
            if (!obj.HasMember("target") || !obj["target"].IsString()) { error = "set_material: missing 'target'"; return false; }
            std::string target = obj["target"].GetString();

            auto* targetObj = m_scene.findObjectByName(target);
            if (!targetObj) { error = "set_material: object '" + target + "' not found"; return false; }

            if (!obj.HasMember("material") || !obj["material"].IsObject()) { error = "set_material: missing 'material'"; return false; }
            const auto& matObj = obj["material"];

            // Update material properties
            if (matObj.HasMember("color") && matObj["color"].IsArray()) {
                glm::vec3 color;
                if (!getVec3(matObj["color"], color)) { error = "set_material: bad 'color'"; return false; }
                const_cast<SceneObject*>(targetObj)->color = color;
                const_cast<SceneObject*>(targetObj)->material.diffuse = color;
                const_cast<SceneObject*>(targetObj)->baseColorFactor = glm::vec4(color, 1.0f);
            }

            if (matObj.HasMember("roughness") && matObj["roughness"].IsNumber()) {
                float roughness = (float)matObj["roughness"].GetDouble();
                const_cast<SceneObject*>(targetObj)->material.roughness = roughness;
                const_cast<SceneObject*>(targetObj)->roughnessFactor = roughness;
            }

            if (matObj.HasMember("metallic") && matObj["metallic"].IsNumber()) {
                float metallic = (float)matObj["metallic"].GetDouble();
                const_cast<SceneObject*>(targetObj)->material.metallic = metallic;
                const_cast<SceneObject*>(targetObj)->metallicFactor = metallic;
            }

            if (matObj.HasMember("specular") && matObj["specular"].IsArray()) {
                glm::vec3 specular;
                if (!getVec3(matObj["specular"], specular)) { error = "set_material: bad 'specular'"; return false; }
                const_cast<SceneObject*>(targetObj)->material.specular = specular;
            }

            if (matObj.HasMember("ambient") && matObj["ambient"].IsArray()) {
                glm::vec3 ambient;
                if (!getVec3(matObj["ambient"], ambient)) { error = "set_material: bad 'ambient'"; return false; }
                const_cast<SceneObject*>(targetObj)->material.ambient = ambient;
            }

            return true;
        }
        else if (op == "delete") {
            if (!obj.HasMember("name") || !obj["name"].IsString()) { error = "delete: missing 'name'"; return false; }
            std::string name = obj["name"].GetString();
            bool success = m_scene.deleteObject(name);
            if (!success) { error = "delete: object '" + name + "' not found"; return false; }
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
            return true;
        }
        else if (op == "render_image") {
            if (!obj.HasMember("path") || !obj["path"].IsString()) { error = "render_image: missing 'path'"; return false; }
            std::string path = obj["path"].GetString();
            int width = 800, height = 600;
            if (obj.HasMember("width") && obj["width"].IsInt()) width = obj["width"].GetInt();
            if (obj.HasMember("height") && obj["height"].IsInt()) height = obj["height"].GetInt();
            
            bool ok = m_renderer.renderToPNG(m_scene, m_lights, path, width, height);
            if (!ok) { error = std::string("render_image: failed to render to '") + path + "'"; return false; }
            return true;
        }
        else if (op == "duplicate") {
            if (!obj.HasMember("source") || !obj["source"].IsString()) { error = "duplicate: missing 'source'"; return false; }
            if (!obj.HasMember("name") || !obj["name"].IsString()) { error = "duplicate: missing 'name'"; return false; }
            
            std::string sourceName = obj["source"].GetString();
            std::string newName = obj["name"].GetString();
            
            // Check if source object exists
            if (!m_scene.findObjectByName(sourceName)) {
                error = "duplicate: source object '" + sourceName + "' not found";
                return false;
            }
            
            // Optional position offset
            glm::vec3 deltaPos(0.0f);
            if (obj.HasMember("position") && obj["position"].IsArray()) {
                if (!getVec3(obj["position"], deltaPos)) { error = "duplicate: bad 'position'"; return false; }
            }
            
            // Optional scale offset
            glm::vec3* deltaScale = nullptr;
            glm::vec3 scaleVal(0.0f);
            if (obj.HasMember("scale") && obj["scale"].IsArray()) {
                if (!getVec3(obj["scale"], scaleVal)) { error = "duplicate: bad 'scale'"; return false; }
                deltaScale = &scaleVal;
            }
            
            // Optional rotation offset (in degrees)
            glm::vec3* deltaRot = nullptr;
            glm::vec3 rotVal(0.0f);
            if (obj.HasMember("rotation") && obj["rotation"].IsArray()) {
                if (!getVec3(obj["rotation"], rotVal)) { error = "duplicate: bad 'rotation'"; return false; }
                deltaRot = &rotVal;
            }
            
            bool success = m_scene.duplicateObject(sourceName, newName, &deltaPos, deltaScale, deltaRot);
            if (!success) { error = "duplicate: failed to duplicate '" + sourceName + "'"; return false; }
            return true;
        }
        else if (op == "remove") {
            if (!obj.HasMember("name") || !obj["name"].IsString()) { error = "remove: missing 'name'"; return false; }
            std::string name = obj["name"].GetString();
            bool success = m_scene.deleteObject(name);
            if (!success) { error = "remove: object '" + name + "' not found"; return false; }
            return true;
        }
        else if (op == "orbit_camera") {
            float deltaYaw = 0.0f, deltaPitch = 0.0f;
            if (obj.HasMember("yaw") && obj["yaw"].IsNumber()) deltaYaw = (float)obj["yaw"].GetDouble();
            if (obj.HasMember("pitch") && obj["pitch"].IsNumber()) deltaPitch = (float)obj["pitch"].GetDouble();
            
            // Optional target center (defaults to current target/scene center)
            glm::vec3 center(0.0f);
            if (obj.HasMember("center") && obj["center"].IsArray()) {
                if (!getVec3(obj["center"], center)) { error = "orbit_camera: bad 'center'"; return false; }
            }
            
            // Get current camera state
            CameraState cs = m_camera.getCameraState();
            
            // Calculate current distance from center
            float distance = glm::length(cs.position - center);
            
            // Apply yaw and pitch rotation around center
            cs.yaw += deltaYaw;
            cs.pitch += deltaPitch;
            
            // Clamp pitch to avoid gimbal lock
            if (cs.pitch > 89.0f) cs.pitch = 89.0f;
            if (cs.pitch < -89.0f) cs.pitch = -89.0f;
            
            // Calculate new position based on spherical coordinates
            float yawRad = glm::radians(cs.yaw);
            float pitchRad = glm::radians(cs.pitch);
            
            cs.position.x = center.x + distance * cos(pitchRad) * cos(yawRad);
            cs.position.y = center.y + distance * sin(pitchRad);
            cs.position.z = center.z + distance * cos(pitchRad) * sin(yawRad);
            
            // Update front vector to look at center
            cs.front = glm::normalize(center - cs.position);
            
            m_camera.setCameraState(cs);
            
            // Sync renderer
            m_renderer.setCamera(m_camera.getCameraState());
            m_renderer.updateViewMatrix();
            return true;
        }
        else if (op == "frame_object") {
            if (!obj.HasMember("name") || !obj["name"].IsString()) { error = "frame_object: missing 'name'"; return false; }
            std::string name = obj["name"].GetString();

            auto* targetObj = m_scene.findObjectByName(name);
            if (!targetObj) { error = std::string("frame_object: object '") + name + "' not found"; return false; }

            // Optional margin parameter
            float margin = 0.25f;
            if (obj.HasMember("margin") && obj["margin"].IsNumber()) {
                margin = (float)obj["margin"].GetDouble();
                if (margin < 0.0f) { error = "frame_object: margin must be >= 0"; return false; }
            }

            // Compute world-space AABB of the object (transform 8 corners)
            glm::vec3 aabbMin = targetObj->objLoader.getMinBounds();
            glm::vec3 aabbMax = targetObj->objLoader.getMaxBounds();
            glm::vec3 worldMin(std::numeric_limits<float>::max());
            glm::vec3 worldMax(std::numeric_limits<float>::lowest());
            for (int j = 0; j < 8; ++j) {
                glm::vec3 v((j & 1) ? aabbMax.x : aabbMin.x,
                            (j & 2) ? aabbMax.y : aabbMin.y,
                            (j & 4) ? aabbMax.z : aabbMin.z);
                glm::vec3 w = glm::vec3(targetObj->modelMatrix * glm::vec4(v, 1.0f));
                worldMin = glm::min(worldMin, w);
                worldMax = glm::max(worldMax, w);
            }

            glm::vec3 center = (worldMin + worldMax) * 0.5f;
            glm::vec3 size = worldMax - worldMin;
            float radius = glm::length(size) * 0.5f; // bounding sphere

            // Compute required distance from center using vertical FOV
            CameraState cs = m_camera.getCameraState();
            float fovRad = glm::radians(cs.fov);
            float dist = (radius * (1.0f + margin)) / std::max(0.0001f, std::tan(fovRad * 0.5f));

            // Place camera along its current view direction to look at center
            glm::vec3 dir = (glm::length(cs.front) > 0.0f) ? glm::normalize(cs.front) : glm::vec3(0, 0, -1);
            glm::vec3 up = (glm::length(cs.up) > 0.0f) ? glm::normalize(cs.up) : glm::vec3(0, 1, 0);
            glm::vec3 newPos = center - dir * dist;
            m_camera.setTarget(newPos, center, up);

            // Sync renderer
            m_renderer.setCamera(m_camera.getCameraState());
            m_renderer.updateViewMatrix();
            return true;
        }
        else if (op == "select") {
            if (!obj.HasMember("name") || !obj["name"].IsString()) { error = "select: missing 'name'"; return false; }
            std::string name = obj["name"].GetString();
            
            int index = m_scene.findObjectIndex(name);
            if (index == -1) { error = "select: object '" + name + "' not found"; return false; }
            
            m_scene.setSelectedObjectIndex(index);
            return true;
        }
        else if (op == "set_background") {
            // Support both color and skybox types
            if (obj.HasMember("color") && obj["color"].IsArray()) {
                glm::vec3 color;
                if (!getVec3(obj["color"], color)) { error = "set_background: bad 'color'"; return false; }
                m_renderer.setBackgroundColor(color);
                return true;
            }
            else if (obj.HasMember("skybox") && obj["skybox"].IsString()) {
                std::string skyboxPath = obj["skybox"].GetString();
                bool okSky = m_renderer.loadSkybox(skyboxPath);
                if (!okSky) { error = std::string("set_background: failed to load skybox '") + skyboxPath + "'"; return false; }
                return true;
            }
            else {
                error = "set_background: missing 'color' or 'skybox'";
                return false;
            }
        }
        else if (op == "exposure") {
            if (!obj.HasMember("value") || !obj["value"].IsNumber()) { error = "exposure: missing 'value'"; return false; }
            float exposureValue = (float)obj["value"].GetDouble();
            m_renderer.setExposure(exposureValue);
            return true;
        }
        else if (op == "tone_map") {
            if (!obj.HasMember("type") || !obj["type"].IsString()) { error = "tone_map: missing 'type'"; return false; }
            std::string toneMapType = obj["type"].GetString();

            // Validate tone mapping type and map to enum
            RenderToneMapMode mode;
            std::string lowerType = toneMapType; std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), [](unsigned char c){ return (char)std::tolower(c); });
            if (lowerType == "linear") mode = RenderToneMapMode::Linear;
            else if (lowerType == "reinhard") mode = RenderToneMapMode::Reinhard;
            else if (lowerType == "filmic") mode = RenderToneMapMode::Filmic;
            else if (lowerType == "aces") mode = RenderToneMapMode::ACES;
            else { error = std::string("tone_map: invalid type '") + toneMapType + "' (must be 'linear', 'reinhard', 'filmic', or 'aces')"; return false; }

            // Optional parameters
            float gamma = 2.2f;
            if (obj.HasMember("gamma") && obj["gamma"].IsNumber()) {
                gamma = (float)obj["gamma"].GetDouble();
                if (gamma <= 0.0f) { error = "tone_map: gamma must be > 0"; return false; }
            }

            m_renderer.setToneMapping(mode);
            m_renderer.setGamma(gamma);
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
