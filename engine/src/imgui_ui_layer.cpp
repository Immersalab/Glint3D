#include "imgui_ui_layer.h"
#include "ui_bridge.h"
#include "gl_platform.h"
#include "render_utils.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>

#ifndef WEB_USE_HTML_UI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

namespace {
    static std::vector<std::string> s_history;
    static int s_histPos = -1; // -1 means new line
    static const char* kHistFile = ".glint_history";

    static void load_history()
    {
        std::ifstream in(kHistFile, std::ios::in);
        if (!in) return;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty()) s_history.emplace_back(line);
        }
        s_histPos = -1;
    }

    static void save_history()
    {
        std::ofstream out(kHistFile, std::ios::out | std::ios::trunc);
        if (!out) return;
        const size_t maxKeep = 200;
        size_t start = s_history.size() > maxKeep ? s_history.size() - maxKeep : 0;
        for (size_t i = start; i < s_history.size(); ++i) {
            out << s_history[i] << "\n";
        }
    }
}

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

    // Load persistent console history
    load_history();

    // Set dark theme
    setupDarkTheme();
    
    return true;
#endif
}

void ImGuiUILayer::shutdown()
{
#ifndef WEB_USE_HTML_UI
    if (ImGui::GetCurrentContext()) {
        // Save console history
        save_history();
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
    if (m_showSettingsPanel) {
        renderSettingsPanel(state);
    }
    
    // Render performance HUD
    if (m_showPerfHUD) {
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

void ImGuiUILayer::handleCommand(const UICommandData& cmd)
{
    switch (cmd.command) {
        case UICommand::ToggleSettingsPanel:
            m_showSettingsPanel = !m_showSettingsPanel;
            break;
        case UICommand::TogglePerfHUD:
            m_showPerfHUD = !m_showPerfHUD;
            break;
        default:
            // Other commands are handled by the UIBridge
            break;
    }
}

void ImGuiUILayer::renderMainMenuBar(const UIState& state)
{
#ifndef WEB_USE_HTML_UI
    if (ImGui::BeginMainMenuBar()) {
        // File menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Model...", "Ctrl+O")) {
                UICommandData cmd;
                cmd.command = UICommand::ImportModel;
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Load Scene JSON...", "Ctrl+L")) {
                UICommandData cmd;
                cmd.command = UICommand::ImportSceneJSON;
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Export Scene...", "Ctrl+E")) {
                UICommandData cmd;
                cmd.command = UICommand::ExportScene;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Copy Share Link", "Ctrl+Shift+C")) {
                UICommandData cmd;
                cmd.command = UICommand::CopyShareLink;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                UICommandData cmd;
                cmd.command = UICommand::ExitApplication;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::EndMenu();
        }
        
        // View menu
        if (ImGui::BeginMenu("View")) {
            ImGui::TextDisabled("Panels");
            ImGui::Separator();
            
            if (ImGui::MenuItem("Settings Panel", "F1", m_showSettingsPanel)) {
                UICommandData cmd;
                cmd.command = UICommand::ToggleSettingsPanel;
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Performance HUD", "F2", m_showPerfHUD)) {
                UICommandData cmd;
                cmd.command = UICommand::TogglePerfHUD;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Separator();
            ImGui::TextDisabled("Scene Elements");
            ImGui::Separator();
            
            if (ImGui::MenuItem("Grid", "G", state.showGrid)) {
                UICommandData cmd;
                cmd.command = UICommand::ToggleGrid;
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Axes", "A", state.showAxes)) {
                UICommandData cmd;
                cmd.command = UICommand::ToggleAxes;
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Skybox", "S", state.showSkybox)) {
                UICommandData cmd;
                cmd.command = UICommand::ToggleSkybox;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::EndMenu();
        }
        
        // Tools menu
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Center Camera", "Home")) {
                UICommandData cmd;
                cmd.command = UICommand::CenterCamera;
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Reset Scene", "Ctrl+R")) {
                UICommandData cmd;
                cmd.command = UICommand::ResetScene;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Separator();
            
            if (ImGui::BeginMenu("Add Light")) {
                if (ImGui::MenuItem("Point Light")) {
                    UICommandData cmd;
                    cmd.command = UICommand::AddLight;
                    cmd.stringParam = "point";
                    cmd.vec3Param = glm::vec3(0.0f, 2.0f, 0.0f); // Default position
                    if (onCommand) onCommand(cmd);
                }
                if (ImGui::MenuItem("Directional Light")) {
                    UICommandData cmd;
                    cmd.command = UICommand::AddLight;
                    cmd.stringParam = "directional";
                    cmd.vec3Param = glm::vec3(0.0f, 5.0f, 0.0f); // Default position
                    if (onCommand) onCommand(cmd);
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenu();
        }
        
        // Help menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Controls", "F1")) {
                // Show controls help
            }
            
            if (ImGui::MenuItem("About Glint3D")) {
                // Show about dialog
            }
            
            ImGui::EndMenu();
        }
        
        // Show engine info on the right side
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
        ImGui::TextDisabled("Glint3D Engine v0.3.0");
        
        ImGui::EndMainMenuBar();
    }
#endif
}

void ImGuiUILayer::renderSettingsPanel(const UIState& state)
{
#ifndef WEB_USE_HTML_UI
    ImGuiIO& io = ImGui::GetIO();
    float rightW = 380.0f;
    float consoleH = 180.0f; // Slightly taller console at bottom
    
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - rightW - 16.0f, 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(rightW, io.DisplaySize.y - consoleH - 32.0f), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    if (ImGui::Begin("Settings & Diagnostics", nullptr, window_flags)) {
        
        // Camera Settings Section
        if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            
            // Camera speed with improved layout
            ImGui::Text("Movement Speed");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##camera_speed", const_cast<float*>(&state.cameraSpeed), 0.01f, 2.0f, "%.2f")) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraSpeed;
                cmd.floatParam = state.cameraSpeed;
                if (onCommand) onCommand(cmd);
            }
            
            // Mouse sensitivity
            ImGui::Text("Mouse Sensitivity");
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##mouse_sensitivity", const_cast<float*>(&state.sensitivity), 0.01f, 1.0f, "%.2f")) {
                UICommandData cmd;
                cmd.command = UICommand::SetMouseSensitivity;
                cmd.floatParam = state.sensitivity;
                if (onCommand) onCommand(cmd);
            }
            
            // Control options
            bool requireRMB = state.requireRMBToMove;
            if (ImGui::Checkbox("Hold RMB to move camera", &requireRMB)) {
                UICommandData cmd; 
                cmd.command = UICommand::SetRequireRMBToMove; 
                cmd.boolParam = requireRMB; 
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Spacing();
        }
        
        // File & Render Settings Section  
        if (ImGui::CollapsingHeader("File & Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            
            // Import/Export buttons
            ImGui::Text("Import/Export");
            if (ImGui::Button("Import Model", ImVec2((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::ImportModel;
                if (onCommand) onCommand(cmd);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Scene JSON", ImVec2(-1, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::ImportSceneJSON;
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::Button("Export Scene", ImVec2(-1, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::ExportScene;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Output path setting
            ImGui::Text("Output Path for Renders");
            static char outputPathBuffer[256] = "";
            static bool bufferInitialized = false;
            
            // Initialize with default path on first use
            if (!bufferInitialized) {
                std::string defaultPath = RenderUtils::getDefaultOutputPath();
                #ifdef _WIN32
                strncpy_s(outputPathBuffer, sizeof(outputPathBuffer), defaultPath.c_str(), _TRUNCATE);
                #else
                strncpy(outputPathBuffer, defaultPath.c_str(), sizeof(outputPathBuffer) - 1);
                outputPathBuffer[sizeof(outputPathBuffer) - 1] = '\0';
                #endif
                bufferInitialized = true;
            }
            
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##output_path", outputPathBuffer, sizeof(outputPathBuffer))) {
                // Store the path for later use
            }
            
            ImGui::SameLine();
            if (ImGui::SmallButton("Default")) {
                std::string defaultPath = RenderUtils::getDefaultOutputPath();
                #ifdef _WIN32
                strncpy_s(outputPathBuffer, sizeof(outputPathBuffer), defaultPath.c_str(), _TRUNCATE);
                #else
                strncpy(outputPathBuffer, defaultPath.c_str(), sizeof(outputPathBuffer) - 1);
                outputPathBuffer[sizeof(outputPathBuffer) - 1] = '\0';
                #endif
            }
            
            // Render button
            if (ImGui::Button("Render Image", ImVec2(-1, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::RenderToPNG;
                // Process the output path to ensure it uses default directory if needed
                cmd.stringParam = RenderUtils::processOutputPath(std::string(outputPathBuffer));
                cmd.intParam = 800;  // width
                cmd.floatParam = 600.0f;  // height
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Spacing();
        }
        
        // Camera Presets Section
        if (ImGui::CollapsingHeader("Camera Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            ImGui::Text("Quick camera angles (Hotkeys 1-8):");
            ImGui::Spacing();
            
            // Create a 4x2 grid of preset buttons
            const float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            
            // Row 1: Front, Back
            if (ImGui::Button("1-Front", ImVec2(buttonWidth, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraPreset;
                cmd.intParam = static_cast<int>(CameraPreset::Front);
                if (onCommand) onCommand(cmd);
            }
            ImGui::SameLine();
            if (ImGui::Button("2-Back", ImVec2(buttonWidth, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraPreset;
                cmd.intParam = static_cast<int>(CameraPreset::Back);
                if (onCommand) onCommand(cmd);
            }
            
            // Row 2: Left, Right
            if (ImGui::Button("3-Left", ImVec2(buttonWidth, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraPreset;
                cmd.intParam = static_cast<int>(CameraPreset::Left);
                if (onCommand) onCommand(cmd);
            }
            ImGui::SameLine();
            if (ImGui::Button("4-Right", ImVec2(buttonWidth, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraPreset;
                cmd.intParam = static_cast<int>(CameraPreset::Right);
                if (onCommand) onCommand(cmd);
            }
            
            // Row 3: Top, Bottom
            if (ImGui::Button("5-Top", ImVec2(buttonWidth, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraPreset;
                cmd.intParam = static_cast<int>(CameraPreset::Top);
                if (onCommand) onCommand(cmd);
            }
            ImGui::SameLine();
            if (ImGui::Button("6-Bottom", ImVec2(buttonWidth, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraPreset;
                cmd.intParam = static_cast<int>(CameraPreset::Bottom);
                if (onCommand) onCommand(cmd);
            }
            
            // Row 4: Isometric views
            if (ImGui::Button("7-Iso FL", ImVec2(buttonWidth, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraPreset;
                cmd.intParam = static_cast<int>(CameraPreset::IsoFL);
                if (onCommand) onCommand(cmd);
            }
            ImGui::SameLine();
            if (ImGui::Button("8-Iso BR", ImVec2(buttonWidth, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::SetCameraPreset;
                cmd.intParam = static_cast<int>(CameraPreset::IsoBR);
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Spacing();
        }
        
        // Scene Information Section
        if (ImGui::CollapsingHeader("Scene Information", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            
            // Scene stats with better formatting
            ImGui::BeginGroup();
            {
                ImGui::Text("Objects:");
                ImGui::SameLine(100);
                ImGui::Text("%d", state.objectCount);
                
                ImGui::Text("Lights:");
                ImGui::SameLine(100);
                ImGui::Text("%d", state.lightCount);
                
                if (!state.selectedObjectName.empty()) {
                    ImGui::Text("Selected:");
                    ImGui::SameLine(100);
                    ImGui::TextColored(ImVec4(0.43f, 0.69f, 0.89f, 1.00f), "%s", state.selectedObjectName.c_str());
                }
            }
            ImGui::EndGroup();
            
            ImGui::Spacing();
        }
        
        // Performance Statistics Section
        if (ImGui::CollapsingHeader("Performance Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            
            // Performance stats with improved layout
            ImGui::BeginGroup();
            {
                ImGui::Text("Draw Calls:");
                ImGui::SameLine(120);
                ImGui::Text("%d", state.renderStats.drawCalls);
                
                ImGui::Text("Triangles:");
                ImGui::SameLine(120);
                ImGui::Text("%zu", state.renderStats.totalTriangles);
                
                ImGui::Text("Materials:");
                ImGui::SameLine(120);
                ImGui::Text("%d", state.renderStats.uniqueMaterialKeys);
                
                ImGui::Text("Textures:");
                ImGui::SameLine(120);
                ImGui::Text("%zu (%.1f MB)", state.renderStats.uniqueTextures, state.renderStats.texturesMB);
                
                ImGui::Text("Est. VRAM:");
                ImGui::SameLine(120);
                ImGui::Text("%.1f MB", state.renderStats.vramMB);
            }
            ImGui::EndGroup();
            
            ImGui::Spacing();
        }
        
        // Lighting Section
        if (ImGui::CollapsingHeader("Lighting")) {
            ImGui::Spacing();
            
            // Add Point Light button
            if (ImGui::Button("Add Point Light", ImVec2(-1, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::AddPointLight;
                if (onCommand) onCommand(cmd);
            }
            
            // Add Directional Light button
            if (ImGui::Button("Add Directional Light", ImVec2(-1, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::AddDirectionalLight;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Separator();
            
            // Light controls (if any lights exist)
            if (state.lightCount > 0) {
                ImGui::Text("Lights (%d):", state.lightCount);

                static int selectedLight = -1;
                // keep selection in sync if list size shrinks
                if (selectedLight >= state.lightCount) selectedLight = -1;
                for (int i = 0; i < state.lightCount; i++) {
                    ImGui::PushID(i);
                    // Build display name with type
                    const auto& L = (i < (int)state.lights.size()) ? state.lights[i] : UIState::LightUI{};
                    const char* typeName = (L.type == 1) ? "Directional" : "Point";
                    std::string lightName = std::string(typeName) + " Light " + std::to_string(i + 1);
                    bool selected = (selectedLight == i);
                    if (ImGui::Selectable(lightName.c_str(), selected)) {
                        selectedLight = i;
                        UICommandData cmd;
                        cmd.command = UICommand::SelectLight;
                        cmd.intParam = i;
                        if (onCommand) onCommand(cmd);
                    }
                    
                    // Context menu for light operations
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Delete Light")) {
                            UICommandData cmd;
                            cmd.command = UICommand::DeleteLight;
                            cmd.intParam = i;
                            if (onCommand) onCommand(cmd);
                            selectedLight = -1;
                        }
                        ImGui::EndPopup();
                    }
                    
                    ImGui::PopID();
                }
                
                // Light properties editing
                if (selectedLight >= 0 && selectedLight < (int)state.lights.size()) {
                    const auto& L = state.lights[selectedLight];
                    ImGui::Separator();
                    ImGui::Text("Light %d Properties:", selectedLight + 1);
                    ImGui::Text("Type: %s", L.type == 1 ? "Directional" : "Point");

                    bool enabled = L.enabled;
                    if (ImGui::Checkbox("Enabled", &enabled)) {
                        UICommandData cmd;
                        cmd.command = UICommand::SetLightEnabled;
                        cmd.intParam = selectedLight;
                        cmd.boolParam = enabled;
                        if (onCommand) onCommand(cmd);
                    }

                    float intensity = L.intensity;
                    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f, "%.2f")) {
                        UICommandData cmd;
                        cmd.command = UICommand::SetLightIntensity;
                        cmd.intParam = selectedLight;
                        cmd.floatParam = intensity;
                        if (onCommand) onCommand(cmd);
                    }

                    if (L.type == 1) {
                        // Directional
                        glm::vec3 dir = L.direction;
                        if (ImGui::InputFloat3("Direction", &dir.x)) {
                            UICommandData cmd;
                            cmd.command = UICommand::SetLightDirection;
                            cmd.intParam = selectedLight;
                            cmd.vec3Param = dir;
                            if (onCommand) onCommand(cmd);
                        }
                    } else {
                        // Point
                        glm::vec3 pos = L.position;
                        if (ImGui::InputFloat3("Position", &pos.x)) {
                            UICommandData cmd;
                            cmd.command = UICommand::SetLightPosition;
                            cmd.intParam = selectedLight;
                            cmd.vec3Param = pos;
                            if (onCommand) onCommand(cmd);
                        }
                    }
                }
            }
            
            ImGui::Spacing();
        }
        
        // Diagnostic Options Section
        if (ImGui::CollapsingHeader("Diagnostics")) {
            ImGui::Spacing();
            
            bool showPerfHUD = m_showPerfHUD;
            if (ImGui::Checkbox("Show Performance HUD", &showPerfHUD)) {
                UICommandData cmd;
                cmd.command = UICommand::TogglePerfHUD;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Spacing();
        }
    }
    ImGui::End();
#endif
}

void ImGuiUILayer::renderPerformanceHUD(const UIState& state)
{
#ifndef WEB_USE_HTML_UI
    ImGui::SetNextWindowPos(ImVec2(16, 45), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.90f);  // More visible background
    
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;
    
    if (ImGui::Begin("Performance Monitor", nullptr, window_flags)) {
        // Add some visual flair with colored sections
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.43f, 0.69f, 0.89f, 1.00f));  // Blue accent
        ImGui::Text("RENDER STATS");
        ImGui::PopStyleColor();
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // Create a table-like layout with consistent spacing
        ImGui::BeginGroup();
        {
            ImGui::Text("Draw Calls:");
            ImGui::SameLine(100);
            ImGui::Text("%d", state.renderStats.drawCalls);
            
            ImGui::Text("Triangles:");
            ImGui::SameLine(100);
            if (state.renderStats.totalTriangles > 1000000) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%.1fM", state.renderStats.totalTriangles / 1000000.0f);
            } else if (state.renderStats.totalTriangles > 1000) {
                ImGui::Text("%.1fK", state.renderStats.totalTriangles / 1000.0f);
            } else {
                ImGui::Text("%zu", state.renderStats.totalTriangles);
            }
            
            ImGui::Text("Materials:");
            ImGui::SameLine(100);
            ImGui::Text("%d", state.renderStats.uniqueMaterialKeys);
            
            ImGui::Text("Textures:");
            ImGui::SameLine(100);
            if (state.renderStats.texturesMB > 100.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%zu (%.1f MB)", state.renderStats.uniqueTextures, state.renderStats.texturesMB);
            } else {
                ImGui::Text("%zu (%.1f MB)", state.renderStats.uniqueTextures, state.renderStats.texturesMB);
            }
            
            ImGui::Text("Est. VRAM:");
            ImGui::SameLine(100);
            if (state.renderStats.vramMB > 500.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "%.1f MB", state.renderStats.vramMB);  // Red for high usage
            } else if (state.renderStats.vramMB > 200.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%.1f MB", state.renderStats.vramMB);  // Orange for medium
            } else {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%.1f MB", state.renderStats.vramMB);  // Green for low
            }
        }
        ImGui::EndGroup();
    }
    ImGui::End();
#endif
}

void ImGuiUILayer::renderConsole(const UIState& state)
{
#ifndef WEB_USE_HTML_UI
    ImGuiIO& io = ImGui::GetIO();
    static float s_consoleHeight = 180.0f;  // User-resizable height
    float minH = 120.0f;
    float maxH = io.DisplaySize.y * 0.8f;
    
    // Anchor bottom-left and allow resizing height from top edge
    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y), ImGuiCond_Always, ImVec2(0, 1));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, s_consoleHeight), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x, minH), ImVec2(io.DisplaySize.x, maxH));
    
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoTitleBar | 
        /* resizable height */ 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;
    
    if (ImGui::Begin("Console", nullptr, window_flags)) {
        // Capture new size if user resized; keep width full, persist height
        ImVec2 cur = ImGui::GetWindowSize();
        if (fabsf(cur.y - s_consoleHeight) > 0.5f) {
            s_consoleHeight = cur.y;
            if (s_consoleHeight < minH) s_consoleHeight = minH;
            if (s_consoleHeight > maxH) s_consoleHeight = maxH;
        }
        
        // Header with title and AI controls
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.43f, 0.69f, 0.89f, 1.00f));
        ImGui::Text("CONSOLE");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        
        // Push controls to the right
        float remaining_width = ImGui::GetContentRegionAvail().x;
        float ai_controls_width = 350.0f;
        ImGui::SameLine(remaining_width - ai_controls_width);
        
        // AI controls in a more compact layout
        bool useAI = state.useAI;
        if (ImGui::Checkbox("AI", &useAI)) {
            UICommandData cmd; 
            cmd.command = UICommand::SetUseAI; 
            cmd.boolParam = useAI; 
            if (onCommand) onCommand(cmd);
        }
        
        ImGui::SameLine();
        ImGui::Text("Endpoint:");
        ImGui::SameLine();
        
        static char endpointBuf[256] = "";
        // Sync buffer if different
        if (endpointBuf[0] == '\0' || std::string(endpointBuf) != state.aiEndpoint) {
            std::snprintf(endpointBuf, sizeof(endpointBuf), "%s", 
                         state.aiEndpoint.empty() ? "http://127.0.0.1:11434" : state.aiEndpoint.c_str());
        }
        
        ImGui::SetNextItemWidth(200.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.13f, 0.14f, 1.00f));  // Darker input
        if (ImGui::InputText("##ai_endpoint", endpointBuf, sizeof(endpointBuf), 
                            ImGuiInputTextFlags_EnterReturnsTrue)) {
            UICommandData cmd; 
            cmd.command = UICommand::SetAIEndpoint; 
            cmd.stringParam = std::string(endpointBuf); 
            if (onCommand) onCommand(cmd);
        }
        ImGui::PopStyleColor();
        
        ImGui::Separator();
        
        // Console output with modern styling
        float input_height = ImGui::GetFrameHeightWithSpacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.09f, 0.10f, 1.00f));  // Darker console background
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.12f, 0.13f, 0.14f, 1.00f));
        
        if (ImGui::BeginChild("##console_scrollback", ImVec2(0, -input_height - 4), true)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));  // Tighter line spacing
            
            for (size_t i = 0; i < state.consoleLog.size(); ++i) {
                const auto& line = state.consoleLog[i];
                
                // Add subtle line numbers or prefixes for better readability
                if (i == state.consoleLog.size() - 1) {  // Latest line
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.43f, 0.69f, 0.89f, 1.00f));  // Blue for latest
                } else if (line.find("Error") != std::string::npos || line.find("error") != std::string::npos) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.00f));  // Red for errors
                } else if (line.find("Warning") != std::string::npos || line.find("warning") != std::string::npos) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.00f));  // Yellow for warnings
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.00f));  // Default console text
                }
                
                ImGui::TextUnformatted(line.c_str());
                ImGui::PopStyleColor();
            }
            
            ImGui::PopStyleVar();
            
            // Auto-scroll to bottom only if we're near the bottom
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0f) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        
        // Console input with modern styling
        ImGui::Text(">");
        ImGui::SameLine();
        
        static char inputBuf[512] = "";
        ImGui::SetNextItemWidth(-1);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.11f, 0.12f, 1.00f));  // Dark input
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));     // Bright text
        
        // InputText with basic history navigation
        bool enter_pressed = ImGui::InputText("##console_input", inputBuf, sizeof(inputBuf), 
                                             ImGuiInputTextFlags_EnterReturnsTrue);
        bool input_active = ImGui::IsItemActive();
        if (input_active) {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                if (!s_history.empty()) {
                    if (s_histPos == -1) s_histPos = (int)s_history.size() - 1;
                    else if (s_histPos > 0) s_histPos--;
                    std::snprintf(inputBuf, sizeof(inputBuf), "%s", s_history[s_histPos].c_str());
                }
            } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                if (!s_history.empty() && s_histPos != -1) {
                    if (s_histPos < (int)s_history.size() - 1) {
                        s_histPos++;
                        std::snprintf(inputBuf, sizeof(inputBuf), "%s", s_history[s_histPos].c_str());
                    } else {
                        s_histPos = -1;
                        inputBuf[0] = '\0';
                    }
                }
            }
        }
        ImGui::PopStyleColor(2);
        
        if (enter_pressed && strlen(inputBuf) > 0) {
            // Push to history (avoid immediate duplicates)
            if (s_history.empty() || s_history.back() != std::string(inputBuf)) {
                s_history.emplace_back(inputBuf);
            }
            s_histPos = -1; // reset browsing position
            UICommandData cmd;
            cmd.command = UICommand::ExecuteConsoleCommand;
            cmd.stringParam = std::string(inputBuf);
            if (onCommand) onCommand(cmd);
            inputBuf[0] = '\0'; // Clear input
        }
        
        // Auto-focus the input field
        if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
            ImGui::SetKeyboardFocusHere(-1);
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
    
    // Ultra-modern dark theme inspired by GitHub Dark, Discord, and modern IDEs
    
    // Background colors - deeper and more sophisticated
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.10f, 1.00f);           // Main background (darker)
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.09f, 0.10f, 0.00f);            // Child windows
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.13f, 0.14f, 0.95f);            // Popups (more opaque)
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);          // Menu bar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.09f, 0.10f, 1.00f);        // Scrollbar background
    
    // Border colors - subtle accents
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.24f, 0.90f);             // Borders (more visible)
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);       // Border shadows
    
    // Text colors - crisp whites
    colors[ImGuiCol_Text] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);               // Primary text (brighter)
    colors[ImGuiCol_TextDisabled] = ImVec4(0.58f, 0.60f, 0.62f, 1.00f);       // Disabled text
    
    // Header colors (tabs, collapsing headers)
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.24f, 0.26f, 0.80f);             // Header default
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.32f, 0.34f, 1.00f);      // Header hovered
    colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.29f, 0.31f, 1.00f);       // Header active
    
    // Button colors
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);             // Button default
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.30f, 0.32f, 1.00f);      // Button hovered
    colors[ImGuiCol_ButtonActive] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);       // Button active
    
    // Frame colors (inputs, sliders)
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);            // Input background
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);     // Input hovered
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);      // Input active
    
    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);            // Title bar
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);      // Title bar active
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.17f, 0.18f, 0.75f);   // Title bar collapsed
    
    // Checkmark and selection - modern deep blue/purple accent (logo-inspired)
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);          // Checkmarks (modern blue)
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.90f, 1.00f);         // Slider grab (deep blue-purple)
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.50f, 1.00f, 1.00f);   // Slider grab active (brighter blue)
    
    // Selection colors
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);     // Text selection (modern blue)
    
    // Scrollbar colors
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.27f, 0.29f, 1.00f);      // Scrollbar grab
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.32f, 0.34f, 1.00f); // Scrollbar grab hovered
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.37f, 0.39f, 1.00f); // Scrollbar grab active
    
    // Tab colors
    colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);                // Tab default
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.32f, 0.34f, 1.00f);         // Tab hovered
    colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);          // Tab active
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.14f, 0.15f, 0.16f, 1.00f);       // Tab unfocused
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.19f, 0.20f, 1.00f); // Tab unfocused active
    
    // Docking colors (only if available)
#ifdef ImGuiCol_DockingPreview
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);     // Docking preview (modern blue)
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);     // Docking empty background
#endif
    
    // Plot colors
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);          // Plot lines
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);   // Plot lines hovered
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);      // Plot histogram
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f); // Plot histogram hovered
    
    // Modern styling
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.WindowRounding = 8.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildRounding = 6.0f;
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.FrameRounding = 4.0f;
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 21.0f;
    style.ScrollbarSize = 14.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabMinSize = 12.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.TabBorderSize = 0.0f;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
    
    // Anti-aliasing
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;
    style.AntiAliasedFill = true;
#endif
}
