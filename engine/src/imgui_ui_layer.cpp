#include "imgui_ui_layer.h"
#include "ui_bridge.h"
#include "gl_platform.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <cstring>

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
            if (ImGui::MenuItem("Load Cube", "Ctrl+L")) {
                UICommandData cmd;
                cmd.command = UICommand::LoadObject;
                cmd.stringParam = "assets/models/cube.obj";
                cmd.vec3Param = glm::vec3(0.0f, 0.0f, -2.0f);
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Load Cow", "Ctrl+Alt+C")) {
                UICommandData cmd;
                cmd.command = UICommand::LoadObject;
                cmd.stringParam = "engine/assets/models/cow.obj";
                cmd.vec3Param = glm::vec3(0.0f, 0.0f, -2.0f);
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
            ImGui::TextDisabled("Render Mode");
            ImGui::Separator();
            
            const char* renderModes[] = {"Points", "Wireframe", "Solid", "Raytrace"};
            const char* shortcuts[] = {"1", "2", "3", "4"};
            
            for (int i = 0; i < 4; ++i) {
                bool selected = ((int)state.renderMode == i);
                if (ImGui::MenuItem(renderModes[i], shortcuts[i], selected)) {
                    UICommandData cmd;
                    cmd.command = UICommand::SetRenderMode;
                    cmd.intParam = i;
                    if (onCommand) onCommand(cmd);
                }
            }
            
            ImGui::Separator();
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
        
        // Render Settings Section
        if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            
            ImGui::Text("Render Mode");
            
            // Modern button layout with better spacing
            const char* renderModes[] = {"Points", "Wireframe", "Solid"};
            int currentMode = (int)state.renderMode;
            
            for (int i = 0; i < 3; ++i) {
                if (i > 0) ImGui::SameLine();
                
                bool isSelected = (currentMode == i);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.43f, 0.69f, 0.89f, 1.00f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.51f, 0.77f, 0.97f, 1.00f));
                }
                
                if (ImGui::Button(renderModes[i], ImVec2((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3, 0))) {
                    UICommandData cmd;
                    cmd.command = UICommand::SetRenderMode;
                    cmd.intParam = i;
                    if (onCommand) onCommand(cmd);
                }
                
                if (isSelected) {
                    ImGui::PopStyleColor(2);
                }
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
        
        // View Options Section
        if (ImGui::CollapsingHeader("View Options")) {
            ImGui::Spacing();
            
            bool showGrid = state.showGrid;
            if (ImGui::Checkbox("Show Grid", &showGrid)) {
                UICommandData cmd;
                cmd.command = UICommand::ToggleGrid;
                if (onCommand) onCommand(cmd);
            }
            
            bool showAxes = state.showAxes;
            if (ImGui::Checkbox("Show Axes", &showAxes)) {
                UICommandData cmd;
                cmd.command = UICommand::ToggleAxes;
                if (onCommand) onCommand(cmd);
            }
            
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
        // Simple in-session command history
        static std::vector<std::string> s_history;
        static int s_histPos = -1; // -1 means new line
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
    
    // Modern dark theme inspired by VS Code Dark+ and Discord
    
    // Background colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);           // Main background
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 0.00f);            // Child windows
    colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.17f, 0.18f, 0.92f);            // Popups
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);          // Menu bar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);        // Scrollbar background
    
    // Border colors
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.27f, 0.29f, 0.80f);             // Borders
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);       // Border shadows
    
    // Text colors
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);               // Primary text
    colors[ImGuiCol_TextDisabled] = ImVec4(0.64f, 0.66f, 0.68f, 1.00f);       // Disabled text
    
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
    
    // Checkmark and selection
    colors[ImGuiCol_CheckMark] = ImVec4(0.43f, 0.69f, 0.89f, 1.00f);          // Checkmarks (blue accent)
    colors[ImGuiCol_SliderGrab] = ImVec4(0.43f, 0.69f, 0.89f, 1.00f);         // Slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.51f, 0.77f, 0.97f, 1.00f);   // Slider grab active
    
    // Selection colors
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.43f, 0.69f, 0.89f, 0.35f);     // Text selection
    
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
    colors[ImGuiCol_DockingPreview] = ImVec4(0.43f, 0.69f, 0.89f, 0.40f);     // Docking preview
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
