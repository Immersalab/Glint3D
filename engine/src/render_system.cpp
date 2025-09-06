#include "render_system.h"
#include "scene_manager.h"
#include "light.h"
#include "axisrenderer.h"
#include "grid.h" 
#include "gizmo.h"
#include "skybox.h"
#include "raytracer.h"
#include "shader.h"
#include "gl_platform.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cstdio>

// PNG writer
#ifndef __EMSCRIPTEN__
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "stb_image_write.h"
#endif

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
    m_skybox = std::make_unique<Skybox>();
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
    glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, 1.0f);

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

    // Load raytracing screen quad shader
    m_screenQuadShader = std::make_unique<Shader>();
    if (!m_screenQuadShader->load("shaders/rayscreen.vert", "shaders/rayscreen.frag"))
        std::cerr << "[RenderSystem] Failed to load rayscreen shader.\n";

    // Init helpers
    if (m_grid) m_grid->init(m_gridShader.get(), 200, 1.0f);
    if (m_axisRenderer) m_axisRenderer->init();
    if (m_skybox) m_skybox->init();
    
    // Initialize raytracing resources only when needed
    if (m_renderMode == RenderMode::Raytrace) {
        initScreenQuad();
        initRaytraceTexture();
    }
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

    // Initialize MSAA targets for onscreen rendering if enabled
    m_fbWidth = windowWidth;
    m_fbHeight = windowHeight;
    createOrResizeTargets(windowWidth, windowHeight);

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
    destroyTargets();
}

void RenderSystem::render(const SceneManager& scene, const Light& lights)
{
    // Reset per-frame stats counters
    m_stats = {};
    
    // Optimize clear operations - only clear if background changed
    static glm::vec3 lastBgColor{-1.0f};
    if (lastBgColor != m_backgroundColor) {
        glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, 1.0f);
        lastBgColor = m_backgroundColor;
    }
    // Recreate MSAA targets if viewport changed or flagged
    GLint vp[4] = {0,0,0,0};
    glGetIntegerv(GL_VIEWPORT, vp);
    if (vp[2] != m_fbWidth || vp[3] != m_fbHeight) {
        m_fbWidth = vp[2];
        m_fbHeight = vp[3];
        m_recreateTargets = true;
    }
    if (m_recreateTargets) {
        createOrResizeTargets(m_fbWidth, m_fbHeight);
        m_recreateTargets = false;
    }
    // Bind target for rendering
    if (m_samples > 1 && m_msaaFBO != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFBO);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
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
    
    // Batch debug element rendering for fewer state changes
    renderDebugElements(scene, lights);

    // Selection outline for currently selected object (wireframe overlay)
    {
        int selObj = scene.getSelectedObjectIndex();
        const auto& objs = scene.getObjects();
        if (selObj >= 0 && selObj < (int)objs.size() && m_basicShader) {
            const auto& obj = objs[selObj];
            if (obj.VAO != 0) {
                Shader* s = m_basicShader.get();
                s->use();
                // Post-processing uniforms for standard shader (selection overlay respects gamma/exposure)
                s->setFloat("exposure", m_exposure);
                s->setFloat("gamma", m_gamma);
                s->setInt("toneMappingMode", static_cast<int>(m_tonemap));
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
                // Selection overlay adds an extra draw call
                m_stats.drawCalls += 1;
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
            // Approximate draw call for gizmo
            m_stats.drawCalls += 1;
        }
    }
    
    updateRenderStats(scene);

    // Resolve MSAA render to default framebuffer if enabled
    if (m_samples > 1 && m_msaaFBO != 0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, m_fbWidth, m_fbHeight,
                          0, 0, m_fbWidth, m_fbHeight,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
}

bool RenderSystem::loadSkybox(const std::string& path)
{
    // Minimal implementation: ensure skybox is initialized and enable it.
    // If needed, future enhancement can parse `path` for cubemap faces.
    if (!m_skybox) return false;
    if (!m_skybox->init()) return false;
    setShowSkybox(true);
    return true;
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

    // Use offscreen aspect ratio; restore later
    glm::mat4 prevProj = m_projectionMatrix;
    updateProjectionMatrix(width, height);

    if (m_samples > 1) {
        // MSAA path: render into multisampled RBOs and resolve into provided texture
        GLuint fboMSAA = 0, rboColorMSAA = 0, rboDepthMSAA = 0;
        GLuint fboResolve = 0;

        glGenFramebuffers(1, &fboMSAA);
        glBindFramebuffer(GL_FRAMEBUFFER, fboMSAA);

        glGenRenderbuffers(1, &rboColorMSAA);
        glBindRenderbuffer(GL_RENDERBUFFER, rboColorMSAA);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_RGBA8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboColorMSAA);

        glGenRenderbuffers(1, &rboDepthMSAA);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepthMSAA);
#ifndef __EMSCRIPTEN__
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthMSAA);
#else
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH_COMPONENT16, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepthMSAA);
#endif

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
            if (rboColorMSAA) glDeleteRenderbuffers(1, &rboColorMSAA);
            if (rboDepthMSAA) glDeleteRenderbuffers(1, &rboDepthMSAA);
            if (fboMSAA) glDeleteFramebuffers(1, &fboMSAA);
            m_projectionMatrix = prevProj;
            glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            return false;
        }

        // Render to MSAA framebuffer
        glViewport(0, 0, width, height);
        glClearColor(0.10f, 0.11f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_renderMode == RenderMode::Raytrace) {
            renderRaytraced(scene, lights);
        } else {
            renderRasterized(scene, lights);
        }

        // Create resolve framebuffer with provided texture
        glGenFramebuffers(1, &fboResolve);
        glBindFramebuffer(GL_FRAMEBUFFER, fboResolve);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
#ifndef __EMSCRIPTEN__
        const GLenum drawBufsR[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBufsR);
#endif
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
            if (fboResolve) glDeleteFramebuffers(1, &fboResolve);
            if (rboColorMSAA) glDeleteRenderbuffers(1, &rboColorMSAA);
            if (rboDepthMSAA) glDeleteRenderbuffers(1, &rboDepthMSAA);
            if (fboMSAA) glDeleteFramebuffers(1, &fboMSAA);
            m_projectionMatrix = prevProj;
            glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            return false;
        }

        // Resolve
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMSAA);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboResolve);
        glBlitFramebuffer(0, 0, width, height,
                          0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Cleanup and restore
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        if (fboResolve) glDeleteFramebuffers(1, &fboResolve);
        if (rboColorMSAA) glDeleteRenderbuffers(1, &rboColorMSAA);
        if (rboDepthMSAA) glDeleteRenderbuffers(1, &rboDepthMSAA);
        if (fboMSAA) glDeleteFramebuffers(1, &fboMSAA);

        m_projectionMatrix = prevProj;
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
        return true;
    } else {
        // Single-sample path (previous behavior)
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
            m_projectionMatrix = prevProj;
            glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
            return false;
        }

        // Set viewport and clear
        glViewport(0, 0, width, height);
        glClearColor(0.10f, 0.11f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
}

bool RenderSystem::renderToPNG(const SceneManager& scene, const Light& lights,
                               const std::string& path, int width, int height)
{
#ifdef __EMSCRIPTEN__
    (void)scene; (void)lights; (void)path; (void)width; (void)height;
    std::cerr << "renderToPNG is not supported on Web builds.\n";
    return false;
#else
    if (width <= 0 || height <= 0) return false;

    // Preserve current framebuffer and viewport
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Create color texture (RGBA8)
    GLuint colorTex = 0;
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Render scene into the texture
    bool ok = renderToTexture(scene, lights, colorTex, width, height);
    if (!ok) {
        glDeleteTextures(1, &colorTex);
        return false;
    }

    // Read back pixels from the texture via a temporary FBO
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &colorTex);
        return false;
    }

    glViewport(0, 0, width, height);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    const int comp = 4;
    const int rowStride = width * comp;
    std::vector<std::uint8_t> pixels(height * rowStride);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically for conventional image orientation
    std::vector<std::uint8_t> flipped(height * rowStride);
    for (int y = 0; y < height; ++y) {
        std::memcpy(flipped.data() + (height - 1 - y) * rowStride,
                    pixels.data() + y * rowStride,
                    rowStride);
    }

    // Write PNG
    int writeOK = stbi_write_png(path.c_str(), width, height, comp, flipped.data(), rowStride);

    // Restore previous framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    // Cleanup
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &colorTex);

    return writeOK != 0;
#endif
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
    // Render skybox first as background
    if (m_showSkybox && m_skybox) {
        m_skybox->render(m_viewMatrix, m_projectionMatrix);
        m_stats.drawCalls += 1;
    }
    
    // Optimized object rendering with batching by material/shader
    renderObjectsBatched(scene, lights);
}

void RenderSystem::renderRaytraced(const SceneManager& scene, const Light& lights)
{
    if (!m_raytracer) {
        std::cerr << "[RenderSystem] Raytracer not initialized\n";
        return;
    }

    if (!m_screenQuadShader) {
        std::cerr << "[RenderSystem] Screen quad shader not loaded\n";
        return;
    }

    if (!m_screenQuadVAO)
        initScreenQuad();
    if (!m_raytraceTexture)
        initRaytraceTexture();

    // Clear existing raytracer data and load all scene objects
    m_raytracer = std::make_unique<Raytracer>();
    
    // Set the seed for deterministic rendering
    m_raytracer->setSeed(m_seed);
    
    const auto& objects = scene.getObjects();
    std::cout << "[RenderSystem] Loading " << objects.size() << " objects into raytracer\n";
    
    for (const auto& obj : objects) {
        if (obj.objLoader.getVertCount() == 0) continue; // Skip objects with no geometry
        
        // Load object into raytracer with its transform and material
        float reflectivity = 0.1f; // Default reflectivity
        if (obj.material.specular.r > 0.8f || obj.material.specular.g > 0.8f || obj.material.specular.b > 0.8f) {
            reflectivity = 0.5f; // Higher reflectivity for shiny materials
        }
        
        m_raytracer->loadModel(obj.objLoader, obj.modelMatrix, reflectivity, obj.material);
    }

    // Create output buffer for raytraced image
    std::vector<glm::vec3> raytraceBuffer(m_raytraceWidth * m_raytraceHeight);
    
    std::cout << "[RenderSystem] Raytracing " << m_raytraceWidth << "x" << m_raytraceHeight << " image...\n";
    
    // Render the image using the raytracer
    if (m_raytracer) {
        m_raytracer->setSeed(m_seed);
    }
    m_raytracer->renderImage(raytraceBuffer, m_raytraceWidth, m_raytraceHeight,
                            m_camera.position, m_camera.front, m_camera.up, 
                            m_camera.fov, lights);
    
    // Apply OIDN denoising if enabled
    if (m_denoiseEnabled) {
        std::cout << "[RenderSystem] Applying OIDN denoising...\n";
        if (!denoise(raytraceBuffer, m_raytraceWidth, m_raytraceHeight)) {
            std::cerr << "[RenderSystem] Denoising failed, using raw raytraced image\n";
        }
    }
    
    // Upload raytraced image to texture
    glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_raytraceWidth, m_raytraceHeight, 
                    GL_RGB, GL_FLOAT, raytraceBuffer.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST); // Disable depth testing for screen quad
    
    // Render the raytraced result using screen quad
    m_screenQuadShader->use();
    
    // Set post-processing uniforms
    m_screenQuadShader->setFloat("exposure", m_exposure);
    m_screenQuadShader->setFloat("gamma", m_gamma);
    m_screenQuadShader->setInt("toneMappingMode", static_cast<int>(m_tonemap));
    
    // Bind the raytraced texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
    m_screenQuadShader->setInt("rayTex", 0);
    
    // Draw the screen quad
    glBindVertexArray(m_screenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    // One draw call for the screen quad
    m_stats.drawCalls += 1;
    
    glEnable(GL_DEPTH_TEST); // Re-enable depth testing
    
    std::cout << "[RenderSystem] Raytracing complete\n";
}

void RenderSystem::renderObject(const SceneObject& obj, const Light& lights)
{
    // Basic object rendering - optimized for minimal state changes
    if (obj.VAO == 0) return;

    // Choose shader path
    bool usePBR = (obj.baseColorTex || obj.mrTex || obj.normalTex);
    Shader* s = usePBR && m_pbrShader ? m_pbrShader.get() : m_basicShader.get();
    if (!s) return;
    
    // Cache shader state to avoid redundant use() calls
    static Shader* lastShader = nullptr;
    if (s != lastShader) {
        s->use();
        lastShader = s;
        
        // Set common uniforms once per shader switch
        setupCommonUniforms(s);
    }

    // Matrices
    s->setMat4("model", obj.modelMatrix);
    s->setMat4("view", m_viewMatrix);
    s->setMat4("projection", m_projectionMatrix);

    // Camera and shading
    s->setVec3("viewPos", m_camera.position);
    s->setInt("shadingMode", (int)m_shadingMode);

    if (s == m_basicShader.get()) {
        // Post-processing uniforms for standard shader
        s->setFloat("exposure", m_exposure);
        s->setFloat("gamma", m_gamma);
        s->setInt("toneMappingMode", static_cast<int>(m_tonemap));
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
        
        // Set post-processing uniforms for PBR shader
        s->setFloat("exposure", m_exposure);
        s->setFloat("gamma", m_gamma);
        s->setInt("toneMappingMode", static_cast<int>(m_tonemap));
        
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

    // Optimized render mode handling - cache state
    static RenderMode lastRenderMode = static_cast<RenderMode>(-1);
    if (m_renderMode != lastRenderMode) {
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
        lastRenderMode = m_renderMode;
    }
    
    // Optimized draw call
    if (obj.EBO != 0) {
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
    }

    // Count one draw call for this object
    m_stats.drawCalls += 1;
}

void RenderSystem::updateRenderStats(const SceneManager& scene)
{
    // Update rendering statistics (non-invasive; keep existing drawCalls accumulated during render)
    const auto& objects = scene.getObjects();

    // Triangles in scene geometry
    size_t tris = 0;
    for (const auto& obj : objects) {
        tris += static_cast<size_t>(obj.objLoader.getIndexCount()) / 3u;
    }
    m_stats.totalTriangles = tris;

    // Unique textures and total texture memory estimate
    std::unordered_set<const Texture*> uniqueTex;
    size_t textureBytes = 0;
    for (const auto& obj : objects) {
        const Texture* texes[4] = { obj.texture, obj.baseColorTex, obj.normalTex, obj.mrTex };
        for (const Texture* t : texes) {
            if (!t) continue;
            if (uniqueTex.insert(t).second) {
                // New unique texture; bytes = W * H * channels (approx; compressed formats use channels=4)
                const int w = t->width();
                const int h = t->height();
                const int c = std::max(1, t->channels());
                textureBytes += static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(c);
            }
        }
    }
    m_stats.uniqueTextures = uniqueTex.size();
    m_stats.texturesMB = static_cast<float>(textureBytes) / (1024.0f * 1024.0f);

    // Geometry memory estimate (positions + normals + uvs + tangents + indices)
    size_t geoBytes = 0;
    for (const auto& obj : objects) {
        const size_t vcount = static_cast<size_t>(obj.objLoader.getVertCount());
        const size_t icount = static_cast<size_t>(obj.objLoader.getIndexCount());
        // positions (3 floats)
        geoBytes += vcount * 3u * sizeof(float);
        // normals if present
        if (obj.objLoader.getNormals()) {
            geoBytes += vcount * 3u * sizeof(float);
        }
        // uvs if present
        if (obj.objLoader.hasTexcoords()) {
            geoBytes += vcount * 2u * sizeof(float);
        }
        // tangents if present
        if (obj.objLoader.hasTangents()) {
            geoBytes += vcount * 3u * sizeof(float);
        }
        // indices (uint32)
        geoBytes += icount * sizeof(unsigned int);
    }
    m_stats.geometryMB = static_cast<float>(geoBytes) / (1024.0f * 1024.0f);

    // Unique material keys and top-shared material key
    struct MatKeyHash {
        size_t operator()(const std::string& s) const noexcept { return std::hash<std::string>()(s); }
    };
    std::unordered_map<std::string, int, MatKeyHash> matCounts;
    matCounts.reserve(objects.size());
    auto makeMatKey = [](const SceneObject& o) -> std::string {
        // Build a compact string key from material + PBR factors presence of textures
        char buf[256];
        const auto& m = o.material;
        // Reduce precision to keep keys compact; 2 decimals is enough for grouping
        snprintf(buf, sizeof(buf),
                 "D%.2f,%.2f,%.2f|S%.2f,%.2f,%.2f|A%.2f,%.2f,%.2f|Ns%.2f|R%.2f|M%.2f|BCF%.2f,%.2f,%.2f,%.2f|mr%.2f|rf%.2f|t%d%d%d",
                 m.diffuse.x, m.diffuse.y, m.diffuse.z,
                 m.specular.x, m.specular.y, m.specular.z,
                 m.ambient.x, m.ambient.y, m.ambient.z,
                 m.shininess, m.roughness, m.metallic,
                 o.baseColorFactor.x, o.baseColorFactor.y, o.baseColorFactor.z, o.baseColorFactor.w,
                 o.metallicFactor, o.roughnessFactor,
                 o.baseColorTex ? 1 : 0, o.normalTex ? 1 : 0, o.mrTex ? 1 : 0);
        return std::string(buf);
    };
    for (const auto& obj : objects) {
        std::string key = makeMatKey(obj);
        matCounts[key] += 1;
    }
    m_stats.uniqueMaterialKeys = static_cast<int>(matCounts.size());
    // Compute most shared key
    m_stats.topSharedCount = 0;
    m_stats.topSharedKey.clear();
    for (const auto& kv : matCounts) {
        if (kv.second > m_stats.topSharedCount) {
            m_stats.topSharedCount = kv.second;
            m_stats.topSharedKey = kv.first;
        }
    }

    // Final VRAM estimate
    m_stats.vramMB = m_stats.texturesMB + m_stats.geometryMB;
}

void RenderSystem::initScreenQuad()
{
    // Full-screen quad vertices with UV coordinates
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,  // top left
        -1.0f, -1.0f,  0.0f, 0.0f,  // bottom left
         1.0f, -1.0f,  1.0f, 0.0f,  // bottom right
        
        -1.0f,  1.0f,  0.0f, 1.0f,  // top left
         1.0f, -1.0f,  1.0f, 0.0f,  // bottom right
         1.0f,  1.0f,  1.0f, 1.0f   // top right
    };
    
    glGenVertexArrays(1, &m_screenQuadVAO);
    glGenBuffers(1, &m_screenQuadVBO);
    
    glBindVertexArray(m_screenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_screenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
    
    std::cout << "[RenderSystem] Screen quad initialized for raytracing\n";
}

void RenderSystem::initRaytraceTexture()
{
    glGenTextures(1, &m_raytraceTexture);
    glBindTexture(GL_TEXTURE_2D, m_raytraceTexture);
    
    // Create texture with RGB floating point format for HDR
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_raytraceWidth, m_raytraceHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    std::cout << "[RenderSystem] Raytracing texture initialized (" << m_raytraceWidth << "x" << m_raytraceHeight << ")\n";
}

void RenderSystem::renderDebugElements(const SceneManager& scene, const Light& lights)
{
    // Batch debug element rendering to minimize state changes
    if (m_showGrid && m_grid) {
        m_grid->render(m_viewMatrix, m_projectionMatrix);
        m_stats.drawCalls += 1;
    }
    if (m_showAxes && m_axisRenderer) {
        glm::mat4 I(1.0f);
        m_axisRenderer->render(I, m_viewMatrix, m_projectionMatrix);
        m_stats.drawCalls += 1;
    }
    
    // Light indicators
    lights.renderIndicators(m_viewMatrix, m_projectionMatrix, m_selectedLightIndex);
    m_stats.drawCalls += 1;

    // Selection outline for currently selected object (wireframe overlay)
    renderSelectionOutline(scene);
    
    // Draw gizmo at selected object's or light's center
    renderGizmo(scene, lights);
}

void RenderSystem::renderSelectionOutline(const SceneManager& scene)
{
    int selObj = scene.getSelectedObjectIndex();
    const auto& objs = scene.getObjects();
    if (selObj >= 0 && selObj < (int)objs.size() && m_basicShader) {
        const auto& obj = objs[selObj];
        if (obj.VAO != 0) {
            Shader* s = m_basicShader.get();
            s->use();
            // Post-processing uniforms for standard shader (selection overlay respects gamma/exposure)
            s->setFloat("exposure", m_exposure);
            s->setFloat("gamma", m_gamma);
            s->setInt("toneMappingMode", static_cast<int>(m_tonemap));
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
            // Selection overlay adds an extra draw call
            m_stats.drawCalls += 1;
#ifndef __EMSCRIPTEN__
            glPolygonMode(GL_FRONT_AND_BACK, prevPolyMode[0]);
            glDisable(GL_POLYGON_OFFSET_LINE);
#endif
        }
    }
}

void RenderSystem::renderGizmo(const SceneManager& scene, const Light& lights)
{
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
            // Approximate draw call for gizmo
            m_stats.drawCalls += 1;
        }
    }
}

void RenderSystem::renderObjectsBatched(const SceneManager& scene, const Light& lights)
{
    const auto& objects = scene.getObjects();
    if (objects.empty()) return;
    
    // Group objects by shader type to minimize state changes
    std::vector<const SceneObject*> basicShaderObjects;
    std::vector<const SceneObject*> pbrShaderObjects;
    
    for (const auto& obj : objects) {
        if (obj.VAO == 0) continue;
        
        bool usePBR = (obj.baseColorTex || obj.mrTex || obj.normalTex);
        if (usePBR && m_pbrShader) {
            pbrShaderObjects.push_back(&obj);
        } else {
            basicShaderObjects.push_back(&obj);
        }
    }
    
    // Render basic shader objects in batch
    if (!basicShaderObjects.empty() && m_basicShader) {
        m_basicShader->use();
        setupCommonUniforms(m_basicShader.get());
        // Apply lighting for this shader
        lights.applyLights(m_basicShader->getID());
        for (const auto* obj : basicShaderObjects) {
            renderObjectFast(*obj, lights, m_basicShader.get());
        }
    }
    
    // Render PBR shader objects in batch
    if (!pbrShaderObjects.empty() && m_pbrShader) {
        m_pbrShader->use();
        setupCommonUniforms(m_pbrShader.get());
        // Apply lighting for this shader
        lights.applyLights(m_pbrShader->getID());
        for (const auto* obj : pbrShaderObjects) {
            renderObjectFast(*obj, lights, m_pbrShader.get());
        }
    }
    
    // Reset VAO binding once at the end
    glBindVertexArray(0);
}

void RenderSystem::setupCommonUniforms(Shader* shader)
{
    // Set common uniforms that don't change per object
    shader->setMat4("view", m_viewMatrix);
    shader->setMat4("projection", m_projectionMatrix);
    shader->setVec3("viewPos", m_camera.position);
    shader->setInt("shadingMode", (int)m_shadingMode);
    shader->setFloat("exposure", m_exposure);
    shader->setFloat("gamma", m_gamma);
    shader->setInt("toneMappingMode", static_cast<int>(m_tonemap));
    
    // Bind dummy shadow map
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D, m_dummyShadowTex);
    shader->setInt("shadowMap", 7);
    shader->setMat4("lightSpaceMatrix", glm::mat4(1.0f));
}

void RenderSystem::renderObjectFast(const SceneObject& obj, const Light& lights, Shader* shader)
{
    // Fast object rendering with minimal per-object state changes
    shader->setMat4("model", obj.modelMatrix);
    
    if (shader == m_basicShader.get()) {
        // Standard shader material uniforms
        shader->setVec3("material.diffuse",  obj.material.diffuse);
        shader->setVec3("material.specular", obj.material.specular);
        shader->setVec3("material.ambient",  obj.material.ambient);
        shader->setFloat("material.shininess", obj.material.shininess);
        shader->setFloat("material.roughness", obj.material.roughness);
        shader->setFloat("material.metallic",  obj.material.metallic);
        
        // Texturing
        if (obj.texture) {
            obj.texture->bind(0);
            shader->setBool("useTexture", true);
            shader->setInt("cowTexture", 0);
        } else {
            shader->setBool("useTexture", false);
            shader->setVec3("objectColor", obj.color);
        }
    } else {
        // PBR shader uniforms
        shader->setVec4("baseColorFactor", obj.baseColorFactor);
        shader->setFloat("metallicFactor", obj.metallicFactor);
        shader->setFloat("roughnessFactor", obj.roughnessFactor);
        shader->setBool("hasBaseColorMap", obj.baseColorTex != nullptr);
        shader->setBool("hasNormalMap", obj.normalTex != nullptr && obj.VBO_tangents != 0);
        shader->setBool("hasMRMap", obj.mrTex != nullptr);
        shader->setBool("hasTangents", obj.VBO_tangents != 0);
        
        int unit = 0;
        if (obj.baseColorTex) { obj.baseColorTex->bind(unit); shader->setInt("baseColorTex", unit++); }
        if (obj.normalTex && obj.VBO_tangents != 0) { obj.normalTex->bind(unit); shader->setInt("normalTex", unit++); }
        if (obj.mrTex) { obj.mrTex->bind(unit); shader->setInt("mrTex", unit++); }
    }
    
    glBindVertexArray(obj.VAO);
    
    // Draw call
    if (obj.EBO != 0) {
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, obj.objLoader.getVertCount());
    }
    
    m_stats.drawCalls += 1;
}

void RenderSystem::cleanupRaytracing()
{
    if (m_screenQuadVAO) {
        glDeleteVertexArrays(1, &m_screenQuadVAO);
        m_screenQuadVAO = 0;
    }
    if (m_screenQuadVBO) {
        glDeleteBuffers(1, &m_screenQuadVBO);
        m_screenQuadVBO = 0;
    }
    if (m_raytraceTexture) {
        glDeleteTextures(1, &m_raytraceTexture);
        m_raytraceTexture = 0;
    }
}

void RenderSystem::destroyTargets()
{
    if (m_msaaColorRBO) { glDeleteRenderbuffers(1, &m_msaaColorRBO); m_msaaColorRBO = 0; }
    if (m_msaaDepthRBO) { glDeleteRenderbuffers(1, &m_msaaDepthRBO); m_msaaDepthRBO = 0; }
    if (m_msaaFBO) { glDeleteFramebuffers(1, &m_msaaFBO); m_msaaFBO = 0; }
}

void RenderSystem::createOrResizeTargets(int width, int height)
{
    // Destroy existing
    destroyTargets();

    if (m_samples <= 1) {
        // No offscreen MSAA path needed
        return;
    }

    // Create MSAA FBO
    glGenFramebuffers(1, &m_msaaFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFBO);

    // Color RBO
    glGenRenderbuffers(1, &m_msaaColorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msaaColorRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msaaColorRBO);

    // Depth/Stencil RBO
    glGenRenderbuffers(1, &m_msaaDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msaaDepthRBO);
#ifndef __EMSCRIPTEN__
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthRBO);
#else
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthRBO);
#endif

    // Validate
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Cleanup and disable MSAA path on failure
        destroyTargets();
        m_samples = 1;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
