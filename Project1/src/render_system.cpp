#include "render_system.h"
#include "scene_manager.h"
#include "light.h"
#include "axisrenderer.h"
#include "grid.h" 
#include "gizmo.h"
#include "raytracer.h"
#include "shader.h"
#include "gl_platform.h"
#include <iostream>

#ifdef OIDN_ENABLED
#include <OpenImageDenoise/oidn.hpp>
#endif

RenderSystem::RenderSystem()
{
    // Initialize renderer components (GL resources are created in init)
    m_axisRenderer = std::make_unique<AxisRenderer>();
    m_grid = std::make_unique<Grid>();
    m_raytracer = std::make_unique<Raytracer>();
    m_gizmo = std::make_unique<Gizmo>();
}

RenderSystem::~RenderSystem()
{
    shutdown();
}

bool RenderSystem::init(int windowWidth, int windowHeight)
{
    // Initialize OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
#ifndef __EMSCRIPTEN__
    if (m_framebufferSRGBEnabled) glEnable(GL_FRAMEBUFFER_SRGB); else glDisable(GL_FRAMEBUFFER_SRGB);
#endif
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.10f, 0.11f, 0.12f, 1.0f);

    // Load shaders
    m_basicShader = std::make_unique<Shader>();
    if (!m_basicShader->load("shaders/standard.vert", "shaders/standard.frag"))
        std::cerr << "[RenderSystem] Failed to load standard shader.\n";

    m_pbrShader = std::make_unique<Shader>();
    if (!m_pbrShader->load("shaders/pbr.vert", "shaders/pbr.frag"))
        std::cerr << "[RenderSystem] Failed to load PBR shader.\n";

    m_gridShader = std::make_unique<Shader>();
    if (!m_gridShader->load("shaders/grid.vert", "shaders/grid.frag"))
        std::cerr << "[RenderSystem] Failed to load grid shader.\n";

    // Init helpers
    if (m_grid) m_grid->init(m_gridShader.get(), 200, 1.0f);
    if (m_axisRenderer) m_axisRenderer->init();
    if (m_gizmo) m_gizmo->init();

    // Create a 1x1 depth texture as a dummy shadow map to satisfy shaders
    glGenTextures(1, &m_dummyShadowTex);
    glBindTexture(GL_TEXTURE_2D, m_dummyShadowTex);
    // Allocate 1x1 depth = 1.0
    float depthOne = 1.0f;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &depthOne);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifndef __EMSCRIPTEN__
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.f,1.f,1.f,1.f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
    glBindTexture(GL_TEXTURE_2D, 0);

    // Initialize sub-systems' matrices
    updateProjectionMatrix(windowWidth, windowHeight);
    updateViewMatrix();

    return true;
}

void RenderSystem::shutdown()
{
    // Cleanup OpenGL resources
    if (m_axisRenderer) { m_axisRenderer->cleanup(); }
    if (m_grid) { m_grid->cleanup(); }
    if (m_gizmo) { m_gizmo->cleanup(); }
    m_raytracer.reset();
    m_basicShader.reset();
    m_pbrShader.reset();
    m_gridShader.reset();
    if (m_dummyShadowTex) { glDeleteTextures(1, &m_dummyShadowTex); m_dummyShadowTex = 0; }
}

void RenderSystem::render(const SceneManager& scene, const Light& lights)
{
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Choose render path based on mode
    switch (m_renderMode) {
        case RenderMode::Raytrace:
            renderRaytraced(scene, lights);
            break;
        default:
            renderRasterized(scene, lights);
            break;
    }
    
    // Render debug elements
    if (m_showGrid && m_grid) {
        m_grid->render(m_viewMatrix, m_projectionMatrix);
    }
    if (m_showAxes && m_axisRenderer) {
        glm::mat4 I(1.0f);
        m_axisRenderer->render(I, m_viewMatrix, m_projectionMatrix);
    }
    // Light indicators
    lights.renderIndicators(m_viewMatrix, m_projectionMatrix, m_selectedLightIndex);

    // Selection outline for currently selected object (wireframe overlay)
    {
        int selObj = scene.getSelectedObjectIndex();
        const auto& objs = scene.getObjects();
        if (selObj >= 0 && selObj < (int)objs.size() && m_basicShader) {
            const auto& obj = objs[selObj];
            if (obj.VAO != 0) {
                Shader* s = m_basicShader.get();
                s->use();
                // Matrices
                s->setMat4("model", obj.modelMatrix);
                s->setMat4("view", m_viewMatrix);
                s->setMat4("projection", m_projectionMatrix);
                // Solid highlight color via ambient-only lighting
                s->setInt("shadingMode", 0); // flat path
                s->setBool("useTexture", false);
                s->setVec3("objectColor", glm::vec3(0.2f, 0.7f, 1.0f)); // cyan-ish
                s->setVec3("viewPos", m_camera.position);
                // Force bright ambient and no direct lights
                glActiveTexture(GL_TEXTURE0 + 7);
                glBindTexture(GL_TEXTURE_2D, m_dummyShadowTex);
                s->setInt("shadowMap", 7);
                s->setMat4("lightSpaceMatrix", glm::mat4(1.0f));
                // Material ambient = 1, other params not used in flat ambient-only path
                s->setVec3("material.ambient", glm::vec3(1.0f));
                s->setInt("numLights", 0);
                s->setVec4("globalAmbient", glm::vec4(1.0f));

                // Draw as wireframe overlay with slight depth bias to reduce z-fighting
#ifndef __EMSCRIPTEN__
                GLint prevPolyMode[2];
                glGetIntegerv(GL_POLYGON_MODE, prevPolyMode);
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0f, -1.0f);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(1.5f);
#endif
                glBindVertexArray(obj.VAO);
                if (obj.EBO != 0) {
                    glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
                } else {
                    glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
                }
                glBindVertexArray(0);
#ifndef __EMSCRIPTEN__
                glPolygonMode(GL_FRONT_AND_BACK, prevPolyMode[0]);
                glDisable(GL_POLYGON_OFFSET_LINE);
#endif
            }
        }
    }

    // Draw gizmo at selected object's or light's center
    if (m_gizmo) {
        int selObj = -1;
        const auto& objs = scene.getObjects();
        // Use SceneManager selection if available
        selObj = scene.getSelectedObjectIndex();
        bool haveObj = (selObj >= 0 && selObj < (int)objs.size());
        bool haveLight = (m_selectedLightIndex >= 0 && m_selectedLightIndex < (int)lights.m_lights.size());
        if (haveObj || haveLight) {
            glm::vec3 center(0.0f);
            glm::mat3 R(1.0f);
            if (haveObj) {
                const auto& obj = objs[selObj];
                center = glm::vec3(obj.modelMatrix[3]);
                if (m_gizmoLocal) {
                    glm::mat3 M3(obj.modelMatrix);
                    R[0] = glm::normalize(glm::vec3(M3[0]));
                    R[1] = glm::normalize(glm::vec3(M3[1]));
                    R[2] = glm::normalize(glm::vec3(M3[2]));
                }
            } else {
                center = lights.m_lights[(size_t)m_selectedLightIndex].position;
                R = glm::mat3(1.0f); // lights are treated as world-aligned
            }
            float dist = glm::length(m_camera.position - center);
            float gscale = glm::clamp(dist * 0.15f, 0.5f, 10.0f);
            m_gizmo->render(m_viewMatrix, m_projectionMatrix, center, R, gscale, m_gizmoAxis, m_gizmoMode);
        }
    }
    
    updateRenderStats(scene);
}

bool RenderSystem::renderToTexture(const SceneManager& scene, const Light& lights,
                                  GLuint textureId, int width, int height)
{
    if (textureId == 0 || width <= 0 || height <= 0) return false;

    // Preserve current framebuffer and viewport
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Create FBO
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Attach provided color texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
#ifndef __EMSCRIPTEN__
    const GLenum drawBufs[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBufs);
#endif

    // Create and attach depth (and stencil if available) renderbuffer
    GLuint rboDepth = 0;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
#ifndef __EMSCRIPTEN__
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
#else
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
#endif

    // Validate
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        if (rboDepth) glDeleteRenderbuffers(1, &rboDepth);
        if (fbo) glDeleteFramebuffers(1, &fbo);
        return false;
    }

    // Set viewport and clear
    glViewport(0, 0, width, height);
    glClearColor(0.10f, 0.11f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use offscreen aspect ratio; restore later
    glm::mat4 prevProj = m_projectionMatrix;
    updateProjectionMatrix(width, height);

    // Render scene using current mode
    if (m_renderMode == RenderMode::Raytrace) {
        renderRaytraced(scene, lights);
    } else {
        renderRasterized(scene, lights);
    }

    // Restore projection, framebuffer and viewport
    m_projectionMatrix = prevProj;
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    // Cleanup
    glDeleteRenderbuffers(1, &rboDepth);
    glDeleteFramebuffers(1, &fbo);
    return true;
}

bool RenderSystem::renderToPNG(const SceneManager& scene, const Light& lights,
                               const std::string& path, int width, int height)
{
    // TODO: Implement offscreen render to PNG.
    // Steps:
    // 1) Create MSAA FBO (optional) and color/depth attachments.
    // 2) Render, then resolve MSAA to a single-sampled RGBA8 texture.
    // 3) Read back pixels, flip vertically, write via stb_image_write.
    // 4) Return true on success.
    std::cout << "PNG rendering not implemented yet: " << path << "\n";
    return false;
}

void RenderSystem::updateViewMatrix()
{
    glm::vec3 target = m_camera.position + m_camera.front;
    m_viewMatrix = glm::lookAt(m_camera.position, target, m_camera.up);
}

void RenderSystem::updateProjectionMatrix(int windowWidth, int windowHeight)
{
    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    m_projectionMatrix = glm::perspective(glm::radians(m_camera.fov), aspect, 
                                         m_camera.nearClip, m_camera.farClip);
}

bool RenderSystem::denoise(std::vector<glm::vec3>& color,
                          const std::vector<glm::vec3>* normal,
                          const std::vector<glm::vec3>* albedo)
{
#ifdef OIDN_ENABLED
    try {
        if (color.empty()) {
            std::cerr << "[RenderSystem::denoise] Empty color buffer\n";
            return false;
        }

        // Create OIDN device
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();
        
        // Check for errors
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN device error: " << errorMessage << "\n";
            return false;
        }

        // Try to guess square dimensions (common for raytracer)
        int width = static_cast<int>(std::sqrt(color.size()));
        int height = width;
        
        // Verify we have a proper square image
        if (width * height != static_cast<int>(color.size())) {
            std::cerr << "[RenderSystem::denoise] Cannot determine image dimensions from buffer size " 
                      << color.size() << ". Use the overload with explicit width/height.\n";
            return false;
        }

        // Create the filter for ray tracing denoising
        oidn::FilterRef filter = device.newFilter("RT"); // Ray tracing filter
        
        // Set input images
        filter.setImage("color", color.data(), oidn::Format::Float3, width, height);
        
        // Optional: Set auxiliary buffers if available
        if (normal && normal->size() == color.size()) {
            filter.setImage("normal", normal->data(), oidn::Format::Float3, width, height);
        }
        
        if (albedo && albedo->size() == color.size()) {
            filter.setImage("albedo", albedo->data(), oidn::Format::Float3, width, height);
        }
        
        // Set output image (in-place denoising)
        filter.setImage("output", color.data(), oidn::Format::Float3, width, height);
        
        // Set filter quality (optional - can be "fast", "balanced", "high")
        filter.set("hdr", true); // Enable HDR processing for better quality
        filter.set("srgb", false); // We're working in linear space
        
        // Commit the filter
        filter.commit();
        
        // Check for errors
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN filter setup error: " << errorMessage << "\n";
            return false;
        }

        // Execute the filter
        filter.execute();
        
        // Check for errors after execution
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN execution error: " << errorMessage << "\n";
            return false;
        }

        std::cout << "[RenderSystem::denoise] Successfully denoised " 
                  << width << "x" << height << " image\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[RenderSystem::denoise] Exception: " << e.what() << "\n";
        return false;
    }
#else
    // Fallback for builds without OIDN - just return false to indicate no denoising was done
    std::cout << "[RenderSystem::denoise] Intel Open Image Denoise not available in this build\n";
    return false;
#endif
}

bool RenderSystem::denoise(std::vector<glm::vec3>& color, int width, int height,
                          const std::vector<glm::vec3>* normal,
                          const std::vector<glm::vec3>* albedo)
{
#ifdef OIDN_ENABLED
    try {
        if (color.empty()) {
            std::cerr << "[RenderSystem::denoise] Empty color buffer\n";
            return false;
        }
        
        if (width <= 0 || height <= 0) {
            std::cerr << "[RenderSystem::denoise] Invalid dimensions: " << width << "x" << height << "\n";
            return false;
        }

        // Verify buffer size matches dimensions
        if (static_cast<int>(color.size()) != width * height) {
            std::cerr << "[RenderSystem::denoise] Buffer size " << color.size() 
                      << " doesn't match dimensions " << width << "x" << height << "\n";
            return false;
        }

        // Create OIDN device
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();
        
        // Check for errors
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN device error: " << errorMessage << "\n";
            return false;
        }

        // Create the filter for ray tracing denoising
        oidn::FilterRef filter = device.newFilter("RT"); // Ray tracing filter
        
        // Set input images
        filter.setImage("color", color.data(), oidn::Format::Float3, width, height);
        
        // Optional: Set auxiliary buffers if available
        if (normal && static_cast<int>(normal->size()) == width * height) {
            filter.setImage("normal", normal->data(), oidn::Format::Float3, width, height);
        }
        
        if (albedo && static_cast<int>(albedo->size()) == width * height) {
            filter.setImage("albedo", albedo->data(), oidn::Format::Float3, width, height);
        }
        
        // Set output image (in-place denoising)
        filter.setImage("output", color.data(), oidn::Format::Float3, width, height);
        
        // Set filter quality
        filter.set("hdr", true); // Enable HDR processing for better quality
        filter.set("srgb", false); // We're working in linear space
        
        // Commit the filter
        filter.commit();
        
        // Check for errors
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN filter setup error: " << errorMessage << "\n";
            return false;
        }

        // Execute the filter
        filter.execute();
        
        // Check for errors after execution
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cerr << "[RenderSystem::denoise] OIDN execution error: " << errorMessage << "\n";
            return false;
        }

        std::cout << "[RenderSystem::denoise] Successfully denoised " 
                  << width << "x" << height << " image with explicit dimensions\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[RenderSystem::denoise] Exception: " << e.what() << "\n";
        return false;
    }
#else
    // Fallback for builds without OIDN
    std::cout << "[RenderSystem::denoise] Intel Open Image Denoise not available in this build (explicit dimensions version)\n";
    return false;
#endif
}

void RenderSystem::renderRasterized(const SceneManager& scene, const Light& lights)
{
    // Render all scene objects using OpenGL
    const auto& objects = scene.getObjects();
    
    for (const auto& obj : objects) {
        renderObject(obj, lights);
    }
}

void RenderSystem::renderRaytraced(const SceneManager& scene, const Light& lights)
{
    // TODO: Implement CPU raytracer path or remove this mode.
    // A minimal version could trace to a CPU buffer and upload to a screen quad.
    if (m_raytracer) { std::cout << "Raytracing not fully implemented\n"; }
}

void RenderSystem::renderObject(const SceneObject& obj, const Light& lights)
{
    // Basic object rendering
    if (obj.VAO == 0) return;

    // Choose shader path
    bool usePBR = (obj.baseColorTex || obj.mrTex || obj.normalTex);
    Shader* s = usePBR && m_pbrShader ? m_pbrShader.get() : m_basicShader.get();
    if (!s) return;

    s->use();

    // Matrices
    s->setMat4("model", obj.modelMatrix);
    s->setMat4("view", m_viewMatrix);
    s->setMat4("projection", m_projectionMatrix);

    // Camera and shading
    s->setVec3("viewPos", m_camera.position);
    s->setInt("shadingMode", (int)m_shadingMode);

    if (s == m_basicShader.get()) {
        // Material for standard shader
        s->setVec3("material.diffuse",  obj.material.diffuse);
        s->setVec3("material.specular", obj.material.specular);
        s->setVec3("material.ambient",  obj.material.ambient);
        s->setFloat("material.shininess", obj.material.shininess);
        s->setFloat("material.roughness", obj.material.roughness);
        s->setFloat("material.metallic",  obj.material.metallic);
    } else {
        // PBR uniforms
        s->setVec4("baseColorFactor", obj.baseColorFactor);
        s->setFloat("metallicFactor", obj.metallicFactor);
        s->setFloat("roughnessFactor", obj.roughnessFactor);
        s->setBool("hasBaseColorMap", obj.baseColorTex != nullptr);
        s->setBool("hasNormalMap", obj.normalTex != nullptr && obj.VBO_tangents != 0);
        s->setBool("hasMRMap", obj.mrTex != nullptr);
        s->setBool("hasTangents", obj.VBO_tangents != 0);
        int unit = 0;
        if (obj.baseColorTex) { obj.baseColorTex->bind(unit); s->setInt("baseColorTex", unit++); }
        if (obj.normalTex && obj.VBO_tangents != 0) { obj.normalTex->bind(unit); s->setInt("normalTex", unit++); }
        if (obj.mrTex) { obj.mrTex->bind(unit); s->setInt("mrTex", unit++); }
    }

    // Lights (sets globalAmbient, lights[], numLights)
    lights.applyLights(s->getID());

    // Bind dummy shadow map and identity lightSpaceMatrix to avoid undefined sampling
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D, m_dummyShadowTex);
    s->setInt("shadowMap", 7);
    s->setMat4("lightSpaceMatrix", glm::mat4(1.0f));

    if (s == m_basicShader.get()) {
        // Texturing or solid color
        if (obj.texture) {
            obj.texture->bind(0);
            s->setBool("useTexture", true);
            s->setInt("cowTexture", 0);
        } else {
            s->setBool("useTexture", false);
            s->setVec3("objectColor", obj.color);
        }
    }

    glBindVertexArray(obj.VAO);

    // Set render mode
    switch (m_renderMode) {
        case RenderMode::Points:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case RenderMode::Wireframe:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        default:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
    }
    // Draw the object
    if (obj.EBO != 0) {
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
    }
    
    glBindVertexArray(0);
    
    // Reset polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderSystem::updateRenderStats(const SceneManager& scene)
{
    // Update rendering statistics
    m_stats = {}; // Reset stats
    
    const auto& objects = scene.getObjects();
    m_stats.drawCalls = (int)objects.size();
    
    for (const auto& obj : objects) {
        m_stats.totalTriangles += obj.objLoader.getIndexCount() / 3;
    }
}
