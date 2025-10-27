#include "application_core.h"
#include "gl_platform.h"
#include <GLFW/glfw3.h>
#include "scene_manager.h"
#include "render_system.h" 
#include "camera_controller.h"
#include "light.h"
#include "ui_bridge.h"
#include "imgui_ui_layer.h"
#include "RayUtils.h"
#include "json_ops.h"
#include "imgui.h"
#include "stb_image.h"
#include <iostream>
#include <memory>
#include <limits>
#include <cmath>
#include <vector>

ApplicationCore::ApplicationCore()
{
    // Create core systems
    m_scene = std::make_unique<SceneManager>();
    m_renderer = std::make_unique<RenderSystem>();
    m_camera = std::make_unique<CameraController>();
    m_lights = std::make_unique<Light>();
    // Create JSON ops executor (headless + programmatic ops)
    m_ops = std::make_unique<JsonOpsExecutor>(*m_scene, *m_renderer, *m_camera, *m_lights);
    
    // Create UI bridge
    m_uiBridge = std::make_unique<UIBridge>(*m_scene, *m_renderer, *m_camera, *m_lights);
}

ApplicationCore::~ApplicationCore()
{
    shutdown();
}

bool ApplicationCore::init(const std::string& windowTitle, int width, int height, bool headless)
{
    m_windowWidth = width;
    m_windowHeight = height;
    m_headless = headless;
    
    // Initialize GLFW and create window
    if (!initGLFW(windowTitle, width, height)) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    
    // Set window icon
    setWindowIcon();
    
    // Initialize OpenGL function loading
    if (!initGLAD()) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    
    // Initialize core systems
    if (!m_renderer->init(width, height)) {
        std::cerr << "Failed to initialize render system\n";
        return false;
    }
    // Initialize gizmo defaults on renderer
    m_renderer->setGizmoMode(m_gizmoMode);
    m_renderer->setGizmoAxis(m_gizmoAxis);
    m_renderer->setGizmoLocalSpace(m_gizmoLocal);
    // Initialize light indicator visuals (shader + geometry)
    if (m_lights) {
        m_lights->initIndicator();
        if (!m_lights->initIndicatorShader()) {
            std::cerr << "Failed to initialize light indicator shader\n";
        }
    }
    
    // Set up callbacks
    initCallbacks();
    
    // Initialize UI (skip for headless or web HTML UI)
    if (!m_headless) {
        // Create ImGui UI layer
        auto imguiLayer = std::make_unique<ImGuiUILayer>();
        m_uiBridge->setUILayer(std::move(imguiLayer));
        
        if (!m_uiBridge->initUI(width, height)) {
            std::cerr << "Failed to initialize UI layer\n";
            return false;
        }
    }
    
    // Create default scene content
    createDefaultScene();
    
    return true;
}

void ApplicationCore::run()
{
    while (!glfwWindowShouldClose(m_window)) {
        frame();
    }
}

void ApplicationCore::frame()
{
    glfwPollEvents();
    // Delta time
    double now = glfwGetTime();
    if (m_lastFrameTime == 0.0) m_lastFrameTime = now;
    float dt = static_cast<float>(now - m_lastFrameTime);
    m_lastFrameTime = now;
    
    // Pull UI settings
    if (m_uiBridge) {
        m_requireRMBToMove = m_uiBridge->getRequireRMBToMove();
    }
    // Update camera
    m_camera->update(dt);
    
    // Per-frame movement polling (smooth hold-to-move)
    if (m_rightMousePressed || !m_requireRMBToMove) {
        float speed = m_camera->getSpeed() * dt * 5.0f; // base speed
        if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
            speed *= 2.5f; // sprint
        }
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_camera->moveForward(speed);
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) m_camera->moveBackward(speed);
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) m_camera->moveLeft(speed);
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) m_camera->moveRight(speed);
        if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) m_camera->moveUp(speed);
        if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) m_camera->moveDown(speed);
        if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS) m_camera->moveUp(speed);
        if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) m_camera->moveDown(speed);
    }
    
    // Skip rendering if window is minimized (0x0 framebuffer)
    if (m_windowWidth > 0 && m_windowHeight > 0) {
        // Update render system camera
        m_renderer->setCamera(m_camera->getCameraState());
        m_renderer->updateViewMatrix();
        m_renderer->updateProjectionMatrix(m_windowWidth, m_windowHeight);
        
        // Clear and render scene
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_renderer->render(*m_scene, *m_lights);
        
        // Render UI
        if (!m_headless) {
            m_uiBridge->renderUI();
        }
        
        glfwSwapBuffers(m_window);
    }
}

void ApplicationCore::shutdown()
{
    if (m_uiBridge) {
        m_uiBridge->shutdownUI();
    }

    // Ensure GPU resources owned by the scene are released before tearing down GL
    if (m_scene) {
        m_scene->clear();
    }
    if (m_renderer) {
        m_renderer->shutdown();
    }

    // Destroy objects that may delete GL resources in their destructors
    m_uiBridge.reset();
    m_renderer.reset();
    m_lights.reset();
    m_scene.reset();

    cleanupGL();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

bool ApplicationCore::loadObject(const std::string& name, const std::string& path,
                                const glm::vec3& position, const glm::vec3& scale)
{
    return m_scene->loadObject(name, path, position, scale);
}

bool ApplicationCore::renderToPNG(const std::string& path, int width, int height)
{
    return m_renderer->renderToPNG(*m_scene, *m_lights, path, width, height);
}

bool ApplicationCore::applyJsonOpsV1(const std::string& json, std::string& error)
{
    if (m_ops) return m_ops->apply(json, error);
    error = "JSON ops executor not initialized";
    return false;
}

void ApplicationCore::setStrictSchema(bool enabled, const std::string& version)
{
    if (m_ops) {
        m_ops->setStrictSchema(enabled, version);
    }
}

bool ApplicationCore::isStrictSchemaEnabled() const
{
    if (m_ops) {
        return m_ops->isStrictSchemaEnabled();
    }
    return false;
}

void ApplicationCore::setRenderSettings(const RenderSettings& settings)
{
    m_renderSettings = settings;
    
    // Apply settings to renderer immediately
    if (m_renderer) {
        // Map ToneMappingMode to RenderToneMapMode
        RenderToneMapMode renderToneMode;
        switch (settings.toneMapping) {
            case ToneMappingMode::Linear: renderToneMode = RenderToneMapMode::Linear; break;
            case ToneMappingMode::Reinhard: renderToneMode = RenderToneMapMode::Reinhard; break;
            case ToneMappingMode::ACES: renderToneMode = RenderToneMapMode::ACES; break;
            case ToneMappingMode::Filmic: renderToneMode = RenderToneMapMode::Filmic; break;
        }
        
        m_renderer->setToneMapping(renderToneMode);
        m_renderer->setExposure(settings.exposure);
        m_renderer->setGamma(settings.gamma);
        m_renderer->setSeed(settings.seed);
        m_renderer->setSampleCount(settings.samples);
    }
}

const RenderSettings& ApplicationCore::getRenderSettings() const
{
    return m_renderSettings;
}

std::string ApplicationCore::buildShareLink() const
{
    return m_uiBridge->buildShareLink();
}

std::string ApplicationCore::sceneToJson() const
{
    return m_uiBridge->sceneToJson();
}

void ApplicationCore::setDenoiseEnabled(bool enabled) 
{
    m_renderer->setDenoiseEnabled(enabled);
}

bool ApplicationCore::isDenoiseEnabled() const 
{
    return m_renderer->isDenoiseEnabled();
}

void ApplicationCore::setRaytraceMode(bool enabled) 
{
    m_renderer->setRenderMode(enabled ? RenderMode::Raytrace : RenderMode::Solid);
}

bool ApplicationCore::isRaytraceMode() const 
{
    return m_renderer->getRenderMode() == RenderMode::Raytrace;
}

void ApplicationCore::setReflectionSpp(int spp) 
{
    m_renderer->setReflectionSpp(spp);
}

int ApplicationCore::getReflectionSpp() const 
{
    return m_renderer->getReflectionSpp();
}

void ApplicationCore::handleMouseMove(double xpos, double ypos)
{
    if (m_firstMouse) {
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouse = false;
    }
    
    double deltaX = xpos - m_lastMouseX;
    double deltaY = m_lastMouseY - ypos; // Reversed for correct direction
    
    m_lastMouseX = xpos;
    m_lastMouseY = ypos;
    
    // Gizmo drag updates
    if (m_gizmoDragging) {
        // Build ray and compute new s along drag axis using closest-approach on lines
        auto makeRay = [&](double mx, double my){
            float xNDC = static_cast<float>(2.0 * mx / m_windowWidth - 1.0);
            float yNDC = static_cast<float>(1.0 - 2.0 * my / m_windowHeight);
            glm::vec4 rayClip(xNDC, yNDC, -1.0f, 1.0f);
            glm::mat4 invProj = glm::inverse(m_renderer->getProjectionMatrix());
            glm::vec4 rayEye = invProj * rayClip; rayEye.z = -1.0f; rayEye.w = 0.0f;
            glm::mat4 invView = glm::inverse(m_renderer->getViewMatrix());
            glm::vec3 dir = glm::normalize(glm::vec3(invView * rayEye));
            return Ray(m_camera->getCameraState().position, dir);
        };
        auto closestParams = [](const glm::vec3& r0, const glm::vec3& rd, const glm::vec3& s0, const glm::vec3& sd, float& t, float& s)->bool{
            const float a = glm::dot(rd, rd);
            const float b = glm::dot(rd, sd);
            const float c = glm::dot(sd, sd);
            const glm::vec3 w0 = r0 - s0;
            const float d = glm::dot(rd, w0);
            const float e = glm::dot(sd, w0);
            const float denom = a*c - b*b; if (fabsf(denom) < 1e-6f) return false; t = (b*e - c*d) / denom; s = (a*e - b*d) / denom; return true; };
        Ray ray = makeRay(xpos, ypos);
        float tParam = 0.0f, sNow = m_axisStartS;
        if (closestParams(ray.origin, ray.direction, m_dragOriginWorld, m_dragAxisDir, tParam, sNow)) {
            sNow = glm::max(0.0f, sNow);
            float deltaS = sNow - m_axisStartS;
            if (m_gizmoMode == GizmoMode::Translate) {
                glm::vec3 delta = m_dragAxisDir * deltaS;
                if (m_dragObjectIndex >= 0) {
                    auto& obj = m_scene->getObjects()[m_dragObjectIndex];
                    obj.modelMatrix = glm::translate(glm::mat4(1.0f), delta) * m_modelStart;
                } else if (m_dragLightIndex >= 0 && m_dragLightIndex < (int)m_lights->m_lights.size()) {
                    m_lights->m_lights[(size_t)m_dragLightIndex].position = m_dragOriginWorld + delta;
                }
            }
        }
        return;
    }

    // Handle camera rotation when RMB is pressed or RMB not required
    if (m_rightMousePressed || !m_requireRMBToMove) {
        // Respect ImGui capture unless RMB is held
        if (!m_rightMousePressed && ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
        m_camera->rotate((float)deltaX * m_camera->getSensitivity(), (float)deltaY * m_camera->getSensitivity());
    }
}

void ApplicationCore::handleMouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        m_leftMousePressed = (action == GLFW_PRESS);
        if (action == GLFW_PRESS) {
            // Skip scene picking if UI wants the mouse
            if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
                return;
            }
            // Gizmo axis grab has priority
            int selObj = m_scene->getSelectedObjectIndex();
            bool grabbed = false;
            if (selObj >= 0 || m_selectedLightIndex >= 0) {
                glm::vec3 center(0.0f);
                glm::mat3 R(1.0f);
                if (selObj >= 0) {
                    const auto& obj = m_scene->getObjects()[selObj];
                    center = glm::vec3(obj.modelMatrix[3]);
                    // Use renderer's gizmo space setting to match rendered gizmo
                    if (m_renderer->gizmoLocalSpace()) {
                        glm::mat3 M3(obj.modelMatrix);
                        R[0] = glm::normalize(glm::vec3(M3[0]));
                        R[1] = glm::normalize(glm::vec3(M3[1]));
                        R[2] = glm::normalize(glm::vec3(M3[2]));
                    }
                } else {
                    center = m_lights->m_lights[(size_t)m_selectedLightIndex].position;
                }
                double mx, my; glfwGetCursorPos(m_window, &mx, &my);
                float xNDC = static_cast<float>(2.0 * mx / m_windowWidth - 1.0);
                float yNDC = static_cast<float>(1.0 - 2.0 * my / m_windowHeight);
                glm::vec4 rayClip(xNDC, yNDC, -1.0f, 1.0f);
                glm::mat4 invProj = glm::inverse(m_renderer->getProjectionMatrix());
                glm::vec4 rayEye = invProj * rayClip; rayEye.z = -1.0f; rayEye.w = 0.0f;
                glm::mat4 invView = glm::inverse(m_renderer->getViewMatrix());
                glm::vec3 dir = glm::normalize(glm::vec3(invView * rayEye));
                Ray ray(m_camera->getCameraState().position, dir);
                float dist = glm::length(m_camera->getCameraState().position - center);
                float gscale = glm::clamp(dist * 0.15f, 0.5f, 10.0f);
                GizmoAxis ax; float s0; glm::vec3 axisDir;
                if (m_renderer->getGizmo() && m_renderer->getGizmo()->pickAxis(ray, center, R, gscale, ax, s0, axisDir)) {
                    m_gizmoAxis = ax; m_renderer->setGizmoAxis(ax);
                    m_axisStartS = s0; m_dragOriginWorld = center; m_dragAxisDir = axisDir;
                    m_dragObjectIndex = (selObj >= 0) ? selObj : -1;
                    m_dragLightIndex = (selObj < 0) ? m_selectedLightIndex : -1;
                    if (m_dragObjectIndex >= 0) {
                        m_modelStart = m_scene->getObjects()[m_dragObjectIndex].modelMatrix;
                    }
                    m_gizmoDragging = true; grabbed = true;
                }
            }
            if (grabbed) return;

            // Basic picking: select the closest object's AABB under cursor
            // Build ray from screen coordinates
            auto makeRay = [&](double mx, double my){
                float xNDC = static_cast<float>(2.0 * mx / m_windowWidth - 1.0);
                float yNDC = static_cast<float>(1.0 - 2.0 * my / m_windowHeight);
                glm::vec4 rayClip(xNDC, yNDC, -1.0f, 1.0f);
                glm::mat4 invProj = glm::inverse(m_renderer->getProjectionMatrix());
                glm::vec4 rayEye = invProj * rayClip; rayEye.z = -1.0f; rayEye.w = 0.0f;
                glm::mat4 invView = glm::inverse(m_renderer->getViewMatrix());
                glm::vec3 dir = glm::normalize(glm::vec3(invView * rayEye));
                return Ray(m_camera->getCameraState().position, dir);
            };

            double mx = 0.0, my = 0.0;
            glfwGetCursorPos(m_window, &mx, &my);
            Ray ray = makeRay(mx, my);

            const auto& objects = m_scene->getObjects();
            int picked = -1; int pickedLight = -1; float closestT = std::numeric_limits<float>::max();
            for (size_t i = 0; i < objects.size(); ++i) {
                const auto& obj = objects[i];
                if (obj.objLoader.getVertCount() == 0) continue;
                glm::vec3 aabbMin = obj.objLoader.getMinBounds();
                glm::vec3 aabbMax = obj.objLoader.getMaxBounds();
                // Transform AABB corners to world to compute world-space AABB
                glm::vec3 worldMin(std::numeric_limits<float>::max());
                glm::vec3 worldMax(std::numeric_limits<float>::lowest());
                for (int j=0;j<8;++j) {
                    glm::vec3 v((j&1)?aabbMax.x:aabbMin.x,
                                 (j&2)?aabbMax.y:aabbMin.y,
                                 (j&4)?aabbMax.z:aabbMin.z);
                    glm::vec3 w = glm::vec3(obj.modelMatrix * glm::vec4(v,1.0f));
                    worldMin = glm::min(worldMin, w);
                    worldMax = glm::max(worldMax, w);
                }
                float t;
                if (rayIntersectsAABB(ray, worldMin, worldMax, t)) {
                    if (t < closestT) { closestT = t; picked = static_cast<int>(i); pickedLight = -1; }
                }
            }
            // Lights pick test
            for (size_t i = 0; i < m_lights->m_lights.size(); ++i) {
                glm::vec3 pos = m_lights->m_lights[i].position;
                glm::vec3 he(0.12f);
                glm::vec3 mn = pos - he;
                glm::vec3 mx = pos + he;
                float t;
                if (rayIntersectsAABB(ray, mn, mx, t)) {
                    if (t < closestT) { closestT = t; pickedLight = (int)i; picked = -1; }
                }
            }
            m_scene->setSelectedObjectIndex(picked);
            m_selectedLightIndex = pickedLight;
            m_renderer->setSelectedLightIndex(pickedLight);
            if (m_uiBridge) m_uiBridge->setSelectedLightIndex(pickedLight);
        } else if (action == GLFW_RELEASE) {
            m_gizmoDragging = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        m_rightMousePressed = (action == GLFW_PRESS);
        
        if (action == GLFW_PRESS) {
            // Disable cursor for camera movement
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // Reset mouse baseline to prevent initial jerk when entering look mode
            glfwGetCursorPos(m_window, &m_lastMouseX, &m_lastMouseY);
            m_firstMouse = true;
        } else {
            // Re-enable cursor
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            // End any gizmo drag on release of RMB
            m_gizmoDragging = false;
            // Reset baseline on exit as well
            glfwGetCursorPos(m_window, &m_lastMouseX, &m_lastMouseY);
            m_firstMouse = true;
        }
    }
}

void ApplicationCore::handleFramebufferResize(int width, int height)
{
    // Don't update anything if window is minimized (0x0 framebuffer)
    if (width <= 0 || height <= 0) {
        return;
    }
    
    m_windowWidth = width;
    m_windowHeight = height;
    glViewport(0, 0, width, height);
    
    m_renderer->updateProjectionMatrix(width, height);
    
    if (m_uiBridge) {
        m_uiBridge->handleResize(width, height);
    }
}

void ApplicationCore::handleKey(int key, int scancode, int action, int mods)
{
    // Camera preset hotkeys (1-8 keys)
    if (action == GLFW_PRESS && mods == 0) {
        int presetKey = -1;
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
            presetKey = key - GLFW_KEY_1 + 1; // Convert to 1-8 range
        }
        
        if (presetKey >= 1 && presetKey <= 8) {
            CameraPreset preset = CameraController::presetFromHotkey(presetKey);
            m_camera->setCameraPreset(preset, *m_scene);
            
            std::string presetName = CameraController::presetName(preset);
            std::string message = "Camera preset: " + presetName;
            if (m_uiBridge) m_uiBridge->addConsoleMessage(message);
        }
    }
    
    // Delete key functionality - comprehensive delete system
    if (action == GLFW_PRESS && key == GLFW_KEY_DELETE && m_uiBridge) {
        UICommandData cmd;
        
        // Priority: Selected light > Selected object
        if (m_selectedLightIndex >= 0) {
            // Delete selected light
            cmd.command = UICommand::DeleteLight;
            cmd.intParam = m_selectedLightIndex;
            m_uiBridge->handleUICommand(cmd);
        } else {
            // Delete selected object
            int selectedObjectIndex = m_scene->getSelectedObjectIndex();
            if (selectedObjectIndex >= 0) {
                cmd.command = UICommand::RemoveObject;
                cmd.intParam = selectedObjectIndex;
                m_uiBridge->handleUICommand(cmd);
            } else {
                // No selection - show help message
                if (m_uiBridge) {
                    m_uiBridge->addConsoleMessage("Delete: Select an object or light first (click to select)");
                }
            }
        }
    }
    
    
    // File operation shortcuts
    if ((mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        if (m_uiBridge) {
            UICommandData cmd;
            switch (key) {
                case GLFW_KEY_O:
                    cmd.command = UICommand::ImportAsset;
                    m_uiBridge->handleUICommand(cmd);
                    break;
                case GLFW_KEY_E:
                    cmd.command = UICommand::ExportScene;
                    m_uiBridge->handleUICommand(cmd);
                    break;
                case GLFW_KEY_D:
                    // Ctrl+D: Duplicate selected object (alternative to Insert)
                    {
                        int selectedObjectIndex = m_scene->getSelectedObjectIndex();
                        if (selectedObjectIndex >= 0) {
                            cmd.command = UICommand::DuplicateObject;
                            cmd.intParam = selectedObjectIndex;
                            m_uiBridge->handleUICommand(cmd);
                        }
                    }
                    break;
            }
        }
    }
    
    // Quick gizmo mode switching while holding Shift
    if ((mods & GLFW_MOD_SHIFT) && action == GLFW_PRESS) {
        if (key == GLFW_KEY_Q) { m_gizmoMode = GizmoMode::Translate; m_renderer->setGizmoMode(m_gizmoMode); }
        else if (key == GLFW_KEY_W) { m_gizmoMode = GizmoMode::Rotate; m_renderer->setGizmoMode(m_gizmoMode); }
        else if (key == GLFW_KEY_E) { m_gizmoMode = GizmoMode::Scale; m_renderer->setGizmoMode(m_gizmoMode); }
    }
}

bool ApplicationCore::initGLFW(const std::string& windowTitle, int width, int height)
{
    if (!glfwInit()) {
        return false;
    }
    
    // Set OpenGL context version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Default framebuffer samples: keep single-sample; MSAA is handled via offscreen FBO
    glfwWindowHint(GLFW_SAMPLES, 1);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    
    // Create window
    if (m_headless) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    
    m_window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync
    
    return true;
}

bool ApplicationCore::initGLAD()
{
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

void ApplicationCore::initCallbacks()
{
    // Store this pointer in window user data for callbacks
    glfwSetWindowUserPointer(m_window, this);
    
    // Set callbacks
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetDropCallback(m_window, dropCallback);
}

void ApplicationCore::createDefaultScene()
{
    // Add default lighting
    m_lights->addLight(
        glm::vec3(2.0f, 4.0f, 2.0f),     // position
        glm::vec3(0.8f, 0.8f, 0.7f),     // color
        1.0f                              // intensity
    );
    
    // Set default camera position
    CameraState defaultCam;
    defaultCam.position = glm::vec3(0.0f, 2.0f, 5.0f);
    defaultCam.front = glm::vec3(0.0f, 0.0f, -1.0f);
    defaultCam.up = glm::vec3(0.0f, 1.0f, 0.0f);
    m_camera->setCameraState(defaultCam);
    // Load a default object only when running with the UI so the viewport isn't empty
    // Note: path is relative to runtime working directory containing the engine/assets folder
    if (!m_headless)
        m_scene->loadObject("Sphere", "engine/assets/models/sphere.obj", glm::vec3(0.0f, 0.0f, -4.0f), glm::vec3(1.0f));
}

void ApplicationCore::cleanupGL()
{
    // OpenGL cleanup is handled by the systems themselves
}

void ApplicationCore::setWindowIcon()
{
    if (!m_window) return;
    
    // Try multiple possible icon paths for different build systems
    const std::vector<std::string> iconPaths = {
        "engine/assets/img/Glint3DIcon.png",    // CMake build from repo root
        "../engine/assets/img/Glint3DIcon.png", // If running from build dir
        "../../engine/assets/img/Glint3DIcon.png" // Alternative build dir structure
    };
    
    int width, height, channels;
    unsigned char* data = nullptr;
    std::string successPath;
    
    for (const auto& path : iconPaths) {
        data = stbi_load(path.c_str(), &width, &height, &channels, 4); // Force RGBA
        if (data) {
            successPath = path;
            break;
        }
    }
    
    if (!data) {
        std::cerr << "Failed to load window icon from any of the attempted paths" << std::endl;
        return;
    }
    
    GLFWimage icon;
    icon.width = width;
    icon.height = height;
    icon.pixels = data;
    
    glfwSetWindowIcon(m_window, 1, &icon);
    
    stbi_image_free(data);
}

// GLFW callback wrappers
void ApplicationCore::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleMouseMove(xpos, ypos);
    }
}

void ApplicationCore::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleMouseButton(button, action, mods);
    }
}

void ApplicationCore::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleFramebufferResize(width, height);
    }
}

void ApplicationCore::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleKey(key, scancode, action, mods);
    }
}

void ApplicationCore::dropCallback(GLFWwindow* window, int count, const char** paths)
{
    ApplicationCore* app = static_cast<ApplicationCore*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleFileDrop(count, paths);
    }
}

void ApplicationCore::handleFileDrop(int count, const char** paths)
{
    if (!m_uiBridge) return;
    for (int i = 0; i < count; ++i) {
        const char* p = paths[i];
        if (!p) continue;
        UICommandData cmd; cmd.command = UICommand::OpenFile; cmd.stringParam = std::string(p);
        m_uiBridge->handleUICommand(cmd);
    }
}
