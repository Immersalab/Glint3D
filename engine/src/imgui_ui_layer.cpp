#include "imgui_ui_layer.h"
#include "ui_bridge.h"
#include "gl_platform.h"
#include <GLFW/glfw3.h>

#ifndef WEB_USE_HTML_UI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

ImGuiUILayer::ImGuiUILayer()
{
}

ImGuiUILayer::~ImGuiUILayer()
{
    shutdown();
}

bool ImGuiUILayer::init(int windowWidth, int windowHeight)
{
#ifdef WEB_USE_HTML_UI
    return true; // No ImGui on web
#else
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
#ifdef IMGUI_HAS_DOCKING
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

    // Get window from GLFW current context (this is a simplification)
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) {
        return false;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif

    // Set dark theme
    setupDarkTheme();
    
    return true;
#endif
}

void ImGuiUILayer::shutdown()
{
#ifndef WEB_USE_HTML_UI
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
#endif
}

void ImGuiUILayer::render(const UIState& state)
{
#ifdef WEB_USE_HTML_UI
    return; // No ImGui rendering on web
#else
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render main docking space
#ifdef IMGUI_HAS_DOCKING
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif

    // Render main menu bar
    renderMainMenuBar(state);
    
    // Render settings panel
    if (state.showSettingsPanel) {
        renderSettingsPanel(state);
    }
    
    // Render performance HUD
    if (state.showPerfHUD) {
        renderPerformanceHUD(state);
    }
    
    // Render console
    renderConsole(state);

    // Finalize ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifdef IMGUI_HAS_DOCKING
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
#endif
#endif
}

void ImGuiUILayer::handleResize(int width, int height)
{
    // ImGui handles resize automatically
}

void ImGuiUILayer::renderMainMenuBar(const UIState& state)
{
#ifndef WEB_USE_HTML_UI
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Cube")) {
                UICommandData cmd;
                cmd.command = UICommand::LoadObject;
                cmd.stringParam = "assets/models/cube.obj";
                cmd.vec3Param = glm::vec3(0.0f, 0.0f, -2.0f);
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Copy Share Link")) {
                // Share link functionality
            }
            
            if (ImGui::MenuItem("Toggle Settings Panel", nullptr, state.showSettingsPanel)) {
                // Toggle settings - this would need a command
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            const char* renderModes[] = {"Points", "Wireframe", "Solid", "Raytrace"};
            for (int i = 0; i < 4; ++i) {
                bool selected = ((int)state.renderMode == i);
                if (ImGui::MenuItem(renderModes[i], nullptr, selected)) {
                    UICommandData cmd;
                    cmd.command = UICommand::SetRenderMode;
                    cmd.intParam = i;
                    if (onCommand) onCommand(cmd);
                }
            }
            ImGui::Separator();
            
            if (ImGui::MenuItem("Performance HUD", nullptr, state.showPerfHUD)) {
                // Toggle perf HUD
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
#endif
}

void ImGuiUILayer::renderSettingsPanel(const UIState& state)
{
#ifndef WEB_USE_HTML_UI
    ImGuiIO& io = ImGui::GetIO();
    float rightW = 350.0f;
    float consoleH = 120.0f;
    
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - rightW - 10.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightW, io.DisplaySize.y - consoleH - 20.0f), ImGuiCond_Always);
    
    if (ImGui::Begin("Settings & Diagnostics")) {
        // Camera settings
        ImGui::Text("Camera");
        if (ImGui::SliderFloat("Speed", const_cast<float*>(&state.cameraSpeed), 0.01f, 2.0f)) {
            UICommandData cmd;
            cmd.command = UICommand::SetCameraSpeed;
            cmd.floatParam = state.cameraSpeed;
            if (onCommand) onCommand(cmd);
        }
        
        if (ImGui::SliderFloat("Sensitivity", const_cast<float*>(&state.sensitivity), 0.01f, 1.0f)) {
            UICommandData cmd;
            cmd.command = UICommand::SetMouseSensitivity;
            cmd.floatParam = state.sensitivity;
            if (onCommand) onCommand(cmd);
        }
        
        ImGui::Separator();
        
        // Render mode buttons
        ImGui::Text("Render Mode");
        if (ImGui::Button("Points")) {
            UICommandData cmd;
            cmd.command = UICommand::SetRenderMode;
            cmd.intParam = (int)RenderMode::Points;
            if (onCommand) onCommand(cmd);
        }
        ImGui::SameLine();
        if (ImGui::Button("Wireframe")) {
            UICommandData cmd;
            cmd.command = UICommand::SetRenderMode;
            cmd.intParam = (int)RenderMode::Wireframe;
            if (onCommand) onCommand(cmd);
        }
        ImGui::SameLine();
        if (ImGui::Button("Solid")) {
            UICommandData cmd;
            cmd.command = UICommand::SetRenderMode;
            cmd.intParam = (int)RenderMode::Solid;
            if (onCommand) onCommand(cmd);
        }
        
        ImGui::Separator();
        // Controls
        bool requireRMB = state.requireRMBToMove;
        if (ImGui::Checkbox("Hold RMB to move", &requireRMB)) {
            UICommandData cmd; cmd.command = UICommand::SetRequireRMBToMove; cmd.boolParam = requireRMB; if (onCommand) onCommand(cmd);
        }

        // Scene info
        ImGui::Text("Scene");
        ImGui::Text("Objects: %d", state.objectCount);
        ImGui::Text("Lights: %d", state.lightCount);
        if (!state.selectedObjectName.empty()) {
            ImGui::Text("Selected: %s", state.selectedObjectName.c_str());
        }
        
        ImGui::Separator();
        
        // Statistics
        ImGui::Text("Performance");
        ImGui::Text("Draw Calls: %d", state.renderStats.drawCalls);
        ImGui::Text("Triangles: %zu", state.renderStats.totalTriangles);
    }
    ImGui::End();
#endif
}

void ImGuiUILayer::renderPerformanceHUD(const UIState& state)
{
#ifndef WEB_USE_HTML_UI
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    
    if (ImGui::Begin("Performance HUD", nullptr, 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Draw calls: %d", state.renderStats.drawCalls);
        ImGui::Text("Triangles: %zu", state.renderStats.totalTriangles);
        ImGui::Text("Materials: %d", state.renderStats.uniqueMaterialKeys);
        ImGui::Text("Textures: %zu (%.2f MB)", state.renderStats.uniqueTextures, state.renderStats.texturesMB);
        ImGui::Text("VRAM est: %.2f MB", state.renderStats.vramMB);
    }
    ImGui::End();
#endif
}

void ImGuiUILayer::renderConsole(const UIState& state)
{
#ifndef WEB_USE_HTML_UI
    ImGuiIO& io = ImGui::GetIO();
    float H = 120.0f;
    
    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - H), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, H), ImGuiCond_Always);
    
    if (ImGui::Begin("Console", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        // AI controls row (top of console)
        bool useAI = state.useAI;
        if (ImGui::Checkbox("Use AI", &useAI)) {
            UICommandData cmd; cmd.command = UICommand::SetUseAI; cmd.boolParam = useAI; if (onCommand) onCommand(cmd);
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Endpoint:");
        ImGui::SameLine();
        static char endpointBuf[256] = "";
        // Sync buffer if different
        if (endpointBuf[0] == '\0' || std::string(endpointBuf) != state.aiEndpoint) {
            std::snprintf(endpointBuf, sizeof(endpointBuf), "%s", state.aiEndpoint.empty() ? "http://127.0.0.1:11434" : state.aiEndpoint.c_str());
        }
        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::InputText("##ai_endpoint", endpointBuf, sizeof(endpointBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            UICommandData cmd; cmd.command = UICommand::SetAIEndpoint; cmd.stringParam = std::string(endpointBuf); if (onCommand) onCommand(cmd);
        }
        ImGui::Separator();
        // Console output
        if (ImGui::BeginChild("##console_scrollback", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true)) {
            for (const auto& line : state.consoleLog) {
                ImGui::TextUnformatted(line.c_str());
            }
            ImGui::SetScrollHereY(1.0f); // Auto-scroll to bottom
        }
        ImGui::EndChild();
        
        // Console input
        static char inputBuf[512] = "";
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##console_input", inputBuf, sizeof(inputBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (strlen(inputBuf) > 0) {
                UICommandData cmd;
                cmd.command = UICommand::ExecuteConsoleCommand;
                cmd.stringParam = std::string(inputBuf);
                if (onCommand) onCommand(cmd);
                inputBuf[0] = '\0'; // Clear input
            }
        }
    }
    ImGui::End();
#endif
}

void ImGuiUILayer::setupDarkTheme()
{
#ifndef WEB_USE_HTML_UI
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.32f, 0.36f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.28f, 0.31f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.25f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
#endif
}
