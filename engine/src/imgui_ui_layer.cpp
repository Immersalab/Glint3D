#include "imgui_ui_layer.h"
#include "ui_bridge.h"
#include "panels/scene_tree_panel.h"
#include "gl_platform.h"
#include "render_utils.h"
#include "resource_paths.h"
#include "user_paths.h"
#include <GLFW/glfw3.h>
#include "file_dialog.h"
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace {
    static std::vector<std::string> s_history;
    static int s_histPos = -1; // -1 means new line

    static void load_history()
    {
        std::filesystem::path histPath = glint::getDataPath("history.txt");
        std::ifstream in(histPath, std::ios::in);
        if (!in) return;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty()) s_history.emplace_back(line);
        }
        s_histPos = -1;
    }

    static void save_history()
    {
        std::filesystem::path histPath = glint::getDataPath("history.txt");
        std::ofstream out(histPath, std::ios::out | std::ios::trunc);
        if (!out) return;
        const size_t maxKeep = 200;
        size_t start = s_history.size() > maxKeep ? s_history.size() - maxKeep : 0;
        for (size_t i = start; i < s_history.size(); ++i) {
            out << s_history[i] << "\n";
        }
    }
    
    // Helper function to escape strings for JSON
    static std::string escapeJsonString(const std::string& input) {
        std::string escaped;
        escaped.reserve(input.length() + input.length() / 4); // Reserve extra space for escapes
        
        for (char c : input) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (c >= 0 && c < 0x20) {
                        // Control characters
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        escaped += buf;
                    } else {
                        escaped += c;
                    }
                    break;
            }
        }
        return escaped;
    }

    static const std::string& defaultHDRPath() {
        static const std::string path = ResourcePaths::resolve("assets/img/studio_small_08_4k.exr").string();
        return path;
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
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Set platform-specific ImGui ini file location
    static std::string s_iniPath = glint::getConfigPath("imgui.ini").string();
    io.IniFilename = s_iniPath.c_str();

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
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load persistent console history
    load_history();

    // Set dark theme
    setupDarkTheme();

    return true;
}

void ImGuiUILayer::shutdown()
{
    if (ImGui::GetCurrentContext()) {
        // Save console history
        save_history();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

void ImGuiUILayer::render(const UIState& state)
{
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
    
    // Left Scene Tree panel (terminal-style)
    panels::RenderSceneTree(state, onCommand);

    // Render settings panel
    if (m_showSettingsPanel) {
        renderSettingsPanel(state);
    }
    // Scene hierarchy is now integrated into the settings panel
    
    // Render performance HUD
    if (m_showPerfHUD) {
        renderPerformanceHUD(state);
    }
    
    // Render console
    renderConsole(state);
    
    // Render help dialogs
    renderHelpDialogs();

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
    if (ImGui::BeginMainMenuBar()) {
        // File menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Asset...", "Ctrl+O")) {
                UICommandData cmd;
                cmd.command = UICommand::ImportAsset;
                if (onCommand) onCommand(cmd);
            }
            
            if (ImGui::MenuItem("Export Scene...", "Ctrl+E")) {
                UICommandData cmd;
                cmd.command = UICommand::ExportScene;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Separator();
            if (ImGui::BeginMenu("Recent Files")) {
                if (state.recentFiles.empty()) {
                    ImGui::MenuItem("(empty)", nullptr, false, false);
                } else {
                    int shown = 0;
                    for (const auto& path : state.recentFiles) {
                        if (ImGui::MenuItem(path.c_str())) {
                            UICommandData cmd; cmd.command = UICommand::OpenFile; cmd.stringParam = path; if (onCommand) onCommand(cmd);
                        }
                        if (++shown >= 10) break;
                    }
                }
                ImGui::EndMenu();
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
            
            // Camera Settings
            if (ImGui::BeginMenu("Camera")) {
                // Camera speed
                ImGui::Text("Movement Speed");
                ImGui::SetNextItemWidth(150);
                if (ImGui::SliderFloat("##camera_speed", const_cast<float*>(&state.cameraSpeed), 0.01f, 2.0f, "%.2f")) {
                    UICommandData cmd;
                    cmd.command = UICommand::SetCameraSpeed;
                    cmd.floatParam = state.cameraSpeed;
                    if (onCommand) onCommand(cmd);
                }
                
                // Mouse sensitivity
                ImGui::Text("Mouse Sensitivity");
                ImGui::SetNextItemWidth(150);
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
                
                ImGui::EndMenu();
            }
            
            
            ImGui::EndMenu();
        }
        
        // Help menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Controls", "F1")) {
                m_showControlsHelp = true;
            }
            
            if (ImGui::MenuItem("JSON Operations")) {
                m_showJsonOpsHelp = true;
            }
            
            if (ImGui::MenuItem("About Glint3D")) {
                m_showAboutDialog = true;
            }
            
            ImGui::EndMenu();
        }
        
        // Top-right camera preset quick buttons
        {
            ImGuiStyle& style = ImGui::GetStyle();
            struct Btn { const char* label; const char* tip; CameraPreset preset; };
            Btn buttons[] = {
                {"F",  "Front View (1)",            CameraPreset::Front},
                {"B",  "Back View (2)",             CameraPreset::Back},
                {"L",  "Left View (3)",             CameraPreset::Left},
                {"R",  "Right View (4)",            CameraPreset::Right},
                {"T",  "Top View (5)",              CameraPreset::Top},
                {"D",  "Bottom/Down View (6)",      CameraPreset::Bottom},
                {"FL", "Isometric Front-Left (7)",  CameraPreset::IsoFL},
                {"BR", "Isometric Back-Right (8)",  CameraPreset::IsoBR},
            };

            // Compute total width to right-align the toolbar
            float total_w = 0.0f;
            for (int i = 0; i < (int)(sizeof(buttons)/sizeof(buttons[0])); ++i) {
                ImVec2 ts = ImGui::CalcTextSize(buttons[i].label);
                float bw = ts.x + style.FramePadding.x * 2.0f; // approximate SmallButton width
                total_w += bw;
                if (i != 7) total_w += style.ItemSpacing.x;
            }
            float right_margin = 8.0f;
            ImGui::SameLine(ImGui::GetWindowWidth() - total_w - right_margin);

            for (int i = 0; i < (int)(sizeof(buttons)/sizeof(buttons[0])); ++i) {
                if (ImGui::SmallButton(buttons[i].label)) {
                    UICommandData cmd; 
                    cmd.command = UICommand::SetCameraPreset; 
                    cmd.intParam = static_cast<int>(buttons[i].preset);
                    if (onCommand) onCommand(cmd);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(buttons[i].tip);
                    ImGui::EndTooltip();
                }
                if (i != 7) ImGui::SameLine();
            }
        }

        ImGui::EndMainMenuBar();
    }
}


void ImGuiUILayer::renderSettingsPanel(const UIState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    float rightW = 380.0f;
    float consoleH = 180.0f; // Slightly taller console at bottom
    
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - rightW - 16.0f, 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(rightW, io.DisplaySize.y - consoleH - 32.0f), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    if (ImGui::Begin("Settings", nullptr, window_flags)) {
        
        // Render Image Section (moved to top)
        if (ImGui::CollapsingHeader("Render Image##section", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            
            // Output path setting
            ImGui::Text("Output Path for Renders");
            static char renderOutputPathBuffer[256] = "";
            static bool renderBufferInitialized = false;
            
            // Initialize with default path on first use
            if (!renderBufferInitialized) {
                std::string defaultPath = "D:\\class\\Glint3D\\renders\\";
                #ifdef _WIN32
                strncpy_s(renderOutputPathBuffer, sizeof(renderOutputPathBuffer), defaultPath.c_str(), _TRUNCATE);
                #else
                strncpy(renderOutputPathBuffer, defaultPath.c_str(), sizeof(renderOutputPathBuffer) - 1);
                renderOutputPathBuffer[sizeof(renderOutputPathBuffer) - 1] = '\0';
                #endif
                renderBufferInitialized = true;
            }
            
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##render_output_path", renderOutputPathBuffer, sizeof(renderOutputPathBuffer))) {
                // Store the path for later use
            }
            
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset##render_default")) {
                std::string defaultPath = "D:\\class\\Glint3D\\renders\\";
                #ifdef _WIN32
                strncpy_s(renderOutputPathBuffer, sizeof(renderOutputPathBuffer), defaultPath.c_str(), _TRUNCATE);
                #else
                strncpy(renderOutputPathBuffer, defaultPath.c_str(), sizeof(renderOutputPathBuffer) - 1);
                renderOutputPathBuffer[sizeof(renderOutputPathBuffer) - 1] = '\0';
                #endif
            }
            
            // Render button
            // Use an ID suffix to avoid conflicting with the section header label
            if (ImGui::Button("Render Image##render_button", ImVec2(-1, 0))) {
                UICommandData cmd;
                cmd.command = UICommand::RenderToPNG;
                cmd.stringParam = std::string(renderOutputPathBuffer);
                cmd.intParam = 800;  // width
                cmd.floatParam = 600.0f;  // height
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Spacing();
        }
        
        // Scene Settings (hierarchy moved to left panel)
        if (ImGui::CollapsingHeader("Scene Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            // Lights listing
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Lights (%d)", state.lightCount);
            
            // Enhanced keyboard shortcuts for lights when focused on this section
            if (ImGui::IsWindowFocused() && state.selectedLightIndex >= 0) {
                // Delete key: Delete selected light
                if (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat)) {
                    if (state.selectedLightIndex >= 0 && state.selectedLightIndex < (int)state.lights.size()) {
                        UICommandData cmd; 
                        cmd.command = UICommand::DeleteLight; 
                        cmd.intParam = state.selectedLightIndex; 
                        if (onCommand) onCommand(cmd);
                    }
                }
            }
            
            for (int i = 0; i < (int)state.lights.size(); ++i) {
                ImGui::PushID(10000 + i);
                const auto& L = state.lights[i];
                const char* typePrefix = (L.type == 2) ? "[S]" : (L.type == 1 ? "[D]" : "[P]");
                const char* typeName = (L.type == 2) ? "Spot" : (L.type == 1 ? "Dir" : "Point");
                char buf[128];
                std::snprintf(buf, sizeof(buf), "%s  %s Light %d", typePrefix, typeName, i + 1);
                bool selected = (i == state.selectedLightIndex);
                if (ImGui::Selectable(buf, selected)) {
                    UICommandData cmd; cmd.command = UICommand::SelectLight; cmd.intParam = i; if (onCommand) onCommand(cmd);
                }
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Select")) { UICommandData cmd; cmd.command = UICommand::SelectLight; cmd.intParam = i; if (onCommand) onCommand(cmd); }
                    if (ImGui::MenuItem("Delete", "Del")) { UICommandData cmd; cmd.command = UICommand::DeleteLight; cmd.intParam = i; if (onCommand) onCommand(cmd); }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            
            // Add light buttons in a compact layout
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Add Lights:");
            if (ImGui::Button("Point", ImVec2((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3, 0))) {
                UICommandData cmd; cmd.command = UICommand::AddPointLight; if (onCommand) onCommand(cmd);
            }
            ImGui::SameLine();
            if (ImGui::Button("Directional", ImVec2((ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2, 0))) {
                UICommandData cmd; cmd.command = UICommand::AddDirectionalLight; if (onCommand) onCommand(cmd);
            }
            ImGui::SameLine();
            if (ImGui::Button("Spot", ImVec2(-1, 0))) {
                UICommandData cmd; cmd.command = UICommand::AddSpotLight; if (onCommand) onCommand(cmd);
            }
            
            ImGui::Spacing();
        }
        
        // Environment & Lighting Section  
        if (ImGui::CollapsingHeader("Environment & Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            
            // Environment/IBL controls
            ImGui::Text("Environment & Lighting");

            // Background controls: Solid / Gradient / HDR/EXR
            ImGui::Separator();
            ImGui::Text("Background");
            int bgMode = static_cast<int>(state.backgroundMode);
            const char* bgItems[] = { "Solid", "Gradient", "HDR/EXR" };
            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##background_mode", &bgMode, bgItems, IM_ARRAYSIZE(bgItems))) {
                // Apply default op for selected mode with current values
                if (bgMode == 0) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "[{\"op\":\"set_background\",\"color\":[%.4f,%.4f,%.4f]}]", state.backgroundSolid.x, state.backgroundSolid.y, state.backgroundSolid.z);
                    UICommandData cmd; cmd.command = UICommand::ApplyJsonOps; cmd.stringParam = std::string(buf); if (onCommand) onCommand(cmd);
                } else if (bgMode == 1) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "[{\"op\":\"set_background\",\"top\":[%.4f,%.4f,%.4f],\"bottom\":[%.4f,%.4f,%.4f]}]",
                                  state.backgroundTop.x, state.backgroundTop.y, state.backgroundTop.z,
                                  state.backgroundBottom.x, state.backgroundBottom.y, state.backgroundBottom.z);
                    UICommandData cmd; cmd.command = UICommand::ApplyJsonOps; cmd.stringParam = std::string(buf); if (onCommand) onCommand(cmd);
                } else if (bgMode == 2) {
                    // HDR stub: send current path or placeholder
                    const std::string& fallback = defaultHDRPath();
                    std::string path = state.backgroundHDRPath.empty() ? fallback : state.backgroundHDRPath;
                    std::string payload = std::string("[{\"op\":\"set_background\",\"hdr\":\"") + escapeJsonString(path) + "\"}]";
                    UICommandData cmd; cmd.command = UICommand::ApplyJsonOps; cmd.stringParam = payload; if (onCommand) onCommand(cmd);
                }
            }
            // Color editors for current mode
            if (bgMode == 0) {
                glm::vec3 c = state.backgroundSolid;
                if (ImGui::ColorEdit3("Solid Color", &c.x, ImGuiColorEditFlags_NoAlpha)) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "[{\"op\":\"set_background\",\"color\":[%.4f,%.4f,%.4f]}]", c.x, c.y, c.z);
                    UICommandData cmd; cmd.command = UICommand::ApplyJsonOps; cmd.stringParam = std::string(buf); if (onCommand) onCommand(cmd);
                }
            } else if (bgMode == 1) {
                glm::vec3 top = state.backgroundTop;
                glm::vec3 bottom = state.backgroundBottom;
                if (ImGui::ColorEdit3("Top", &top.x, ImGuiColorEditFlags_NoAlpha)) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "[{\"op\":\"set_background\",\"top\":[%.4f,%.4f,%.4f],\"bottom\":[%.4f,%.4f,%.4f]}]",
                                  top.x, top.y, top.z,
                                  bottom.x, bottom.y, bottom.z);
                    UICommandData cmd; cmd.command = UICommand::ApplyJsonOps; cmd.stringParam = std::string(buf); if (onCommand) onCommand(cmd);
                }
                if (ImGui::ColorEdit3("Bottom", &bottom.x, ImGuiColorEditFlags_NoAlpha)) {
                    char buf[256];
                    std::snprintf(buf, sizeof(buf), "[{\"op\":\"set_background\",\"top\":[%.4f,%.4f,%.4f],\"bottom\":[%.4f,%.4f,%.4f]}]",
                                  top.x, top.y, top.z,
                                  bottom.x, bottom.y, bottom.z);
                    UICommandData cmd; cmd.command = UICommand::ApplyJsonOps; cmd.stringParam = std::string(buf); if (onCommand) onCommand(cmd);
                }
            } else if (bgMode == 2) {
                // HDR/EXR path input
                static char hdrBuf[256] = "";
                if (std::strlen(hdrBuf) == 0) {
                    std::string init = state.backgroundHDRPath.empty() ? defaultHDRPath() : state.backgroundHDRPath;
#ifdef _WIN32
                    strncpy_s(hdrBuf, sizeof(hdrBuf), init.c_str(), _TRUNCATE);
#else
                    std::strncpy(hdrBuf, init.c_str(), sizeof(hdrBuf) - 1);
                    hdrBuf[sizeof(hdrBuf) - 1] = '\0';
#endif
                }
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("HDR/EXR Path", hdrBuf, sizeof(hdrBuf))) {
                    std::string payload = std::string("[{\"op\":\"set_background\",\"hdr\":\"") + escapeJsonString(std::string(hdrBuf)) + "\"}]";
                    UICommandData cmd; cmd.command = UICommand::ApplyJsonOps; cmd.stringParam = payload; if (onCommand) onCommand(cmd);
                }

                // Browse button for selecting HDR/EXR file
                if (ImGui::Button("Browse HDR/EXR...")) {
                    auto filters = FileDialog::getImageFilters();
                    std::string sel = FileDialog::openFile("Select HDR/EXR Environment", filters, "");
                    if (!sel.empty()) {
                        #ifdef _WIN32
                        strncpy_s(hdrBuf, sizeof(hdrBuf), sel.c_str(), _TRUNCATE);
                        #else
                        std::strncpy(hdrBuf, sel.c_str(), sizeof(hdrBuf)-1); hdrBuf[sizeof(hdrBuf)-1] = '\0';
                        #endif
                        std::string payload = std::string("[{\"op\":\"set_background\",\"hdr\":\"") + escapeJsonString(sel) + "\"}]";
                        UICommandData cmd; cmd.command = UICommand::ApplyJsonOps; cmd.stringParam = payload; if (onCommand) onCommand(cmd);
                    }
                }

                // Load HDR/EXR environment button (only show when HDRI is selected)
                if (ImGui::Button("Load HDR/EXR Environment", ImVec2(-1, 0))) {
                    UICommandData cmd;
                    cmd.command = UICommand::LoadHDREnvironment;
                    // Use current renderer state, or default if empty
                    std::string path = state.backgroundHDRPath.empty() ? defaultHDRPath() : state.backgroundHDRPath;
                    cmd.stringParam = path;
                    if (onCommand) onCommand(cmd);
                }
            }
            ImGui::Spacing();
            
            // Skybox intensity slider
            ImGui::Text("Skybox Intensity");
            ImGui::SetNextItemWidth(-1);
            float skyboxIntensity = state.skyboxIntensity;
            if (ImGui::SliderFloat("##skybox_intensity", &skyboxIntensity, 0.0f, 5.0f, "%.2f")) {
                UICommandData cmd;
                cmd.command = UICommand::SetSkyboxIntensity;
                cmd.floatParam = skyboxIntensity;
                if (onCommand) onCommand(cmd);
            }
            
            // IBL intensity slider
            ImGui::Text("IBL Intensity");
            ImGui::SetNextItemWidth(-1);
            float iblIntensity = state.iblIntensity;
            if (ImGui::SliderFloat("##ibl_intensity", &iblIntensity, 0.0f, 5.0f, "%.2f")) {
                UICommandData cmd;
                cmd.command = UICommand::SetIBLIntensity;
                cmd.floatParam = iblIntensity;
                if (onCommand) onCommand(cmd);
            }
            
            ImGui::Spacing();
        }
        
        // Camera Presets moved to top-right toolbar in the main menu
        
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
        
        // Light Properties Section (shows when a light is selected)
        if (state.selectedLightIndex >= 0 && state.selectedLightIndex < (int)state.lights.size()) {
            if (ImGui::CollapsingHeader("Light Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Spacing();
                
                const auto& L = state.lights[state.selectedLightIndex];
                ImGui::Text("Light %d Properties:", state.selectedLightIndex + 1);
                const char* typeLabel = (L.type == 2) ? "Spot" : (L.type == 1 ? "Directional" : "Point");
                ImGui::Text("Type: %s", typeLabel);

                bool enabled = L.enabled;
                if (ImGui::Checkbox("Enabled", &enabled)) {
                    UICommandData cmd;
                    cmd.command = UICommand::SetLightEnabled;
                    cmd.intParam = state.selectedLightIndex;
                    cmd.boolParam = enabled;
                    if (onCommand) onCommand(cmd);
                }

                float intensity = L.intensity;
                if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f, "%.2f")) {
                    UICommandData cmd;
                    cmd.command = UICommand::SetLightIntensity;
                    cmd.intParam = state.selectedLightIndex;
                    cmd.floatParam = intensity;
                    if (onCommand) onCommand(cmd);
                }

                if (L.type == 1) {
                    // Directional
                    glm::vec3 dir = L.direction;
                    if (ImGui::InputFloat3("Direction", &dir.x)) {
                        UICommandData cmd;
                        cmd.command = UICommand::SetLightDirection;
                        cmd.intParam = state.selectedLightIndex;
                        cmd.vec3Param = dir;
                        if (onCommand) onCommand(cmd);
                    }
                } else if (L.type == 0) {
                    // Point
                    glm::vec3 pos = L.position;
                    if (ImGui::InputFloat3("Position", &pos.x)) {
                        UICommandData cmd;
                        cmd.command = UICommand::SetLightPosition;
                        cmd.intParam = state.selectedLightIndex;
                        cmd.vec3Param = pos;
                        if (onCommand) onCommand(cmd);
                    }
                } else if (L.type == 2) {
                    // Spot
                    glm::vec3 pos = L.position;
                    if (ImGui::InputFloat3("Position", &pos.x)) {
                        UICommandData cmd;
                        cmd.command = UICommand::SetLightPosition;
                        cmd.intParam = state.selectedLightIndex;
                        cmd.vec3Param = pos;
                        if (onCommand) onCommand(cmd);
                    }
                    glm::vec3 dir = L.direction;
                    if (ImGui::InputFloat3("Direction", &dir.x)) {
                        UICommandData cmd;
                        cmd.command = UICommand::SetLightDirection;
                        cmd.intParam = state.selectedLightIndex;
                        cmd.vec3Param = dir;
                        if (onCommand) onCommand(cmd);
                    }
                    float inner = L.innerConeDeg;
                    float outer = L.outerConeDeg;
                    if (ImGui::SliderFloat("Inner Cone (deg)", &inner, 0.0f, 89.0f, "%.1f") ) {
                        if (inner > outer) inner = outer;
                        UICommandData cmd;
                        cmd.command = UICommand::SetLightInnerCone;
                        cmd.intParam = state.selectedLightIndex;
                        cmd.floatParam = inner;
                        if (onCommand) onCommand(cmd);
                    }
                    if (ImGui::SliderFloat("Outer Cone (deg)", &outer, 0.0f, 89.0f, "%.1f") ) {
                        if (outer < inner) outer = inner;
                        UICommandData cmd;
                        cmd.command = UICommand::SetLightOuterCone;
                        cmd.intParam = state.selectedLightIndex;
                        cmd.floatParam = outer;
                        if (onCommand) onCommand(cmd);
                    }
                }
                
                ImGui::Spacing();
            }
        }
        
    }
    ImGui::End();
}

void ImGuiUILayer::renderPerformanceHUD(const UIState& state)
{
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
            const int dc = state.renderStats.drawCalls;
            const size_t tris = state.renderStats.totalTriangles;
            const float vram = state.renderStats.vramMB;
            const float texMB = state.renderStats.texturesMB;

            // Thresholds (tuned conservatively)
            const int   DC_WARN = 500, DC_DANGER = 1000;
            const size_t TRI_WARN = 2000000ull, TRI_DANGER = 5000000ull;

            ImGui::Text("Draw Calls:");
            ImGui::SameLine(100);
            if (dc > DC_DANGER) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "%d", dc);
            } else if (dc > DC_WARN) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%d", dc);
            } else {
                ImGui::Text("%d", dc);
            }
            
            ImGui::Text("Triangles:");
            ImGui::SameLine(100);
            if (tris > 1000000ull) {
                if (tris > TRI_DANGER) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "%.1fM", tris / 1000000.0f);
                } else if (tris > TRI_WARN) {
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%.1fM", tris / 1000000.0f);
                } else {
                    ImGui::Text("%.1fM", tris / 1000000.0f);
                }
            } else if (tris > 1000ull) {
                ImGui::Text("%.1fK", tris / 1000.0f);
            } else {
                ImGui::Text("%zu", tris);
            }
            
            ImGui::Text("Materials:");
            ImGui::SameLine(100);
            ImGui::Text("%d", state.renderStats.uniqueMaterialKeys);
            
            ImGui::Text("Textures:");
            ImGui::SameLine(100);
            if (texMB > 100.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%zu (%.1f MB)", state.renderStats.uniqueTextures, texMB);
            } else {
                ImGui::Text("%zu (%.1f MB)", state.renderStats.uniqueTextures, texMB);
            }
            
            ImGui::Text("Est. VRAM:");
            ImGui::SameLine(100);
            if (vram > 500.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "%.1f MB", vram);  // Red for high usage
            } else if (vram > 200.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%.1f MB", vram);  // Orange for medium
            } else {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%.1f MB", vram);  // Green for low
            }
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}

void ImGuiUILayer::renderConsole(const UIState& state)
{
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
        
        // Header with title
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.43f, 0.69f, 0.89f, 1.00f));
        ImGui::Text("CONSOLE");
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
}

void ImGuiUILayer::setupDarkTheme()
{
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
}

void ImGuiUILayer::renderHelpDialogs()
{
    // Controls Help Dialog
    if (m_showControlsHelp) {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Controls Help", &m_showControlsHelp)) {
            ImGui::TextWrapped("Glint3D Controls:");
            ImGui::Separator();
            
            ImGui::Text("Camera Movement:");
            ImGui::BulletText("WASD - Move camera forward/left/back/right");
            ImGui::BulletText("Space - Move camera up");
            ImGui::BulletText("Shift - Move camera down");
            ImGui::BulletText("Mouse drag - Rotate camera view");
            ImGui::BulletText("Scroll wheel - Zoom in/out");
            
            ImGui::Spacing();
            ImGui::Text("Camera Presets:");
            ImGui::BulletText("1-8 Keys - Quick camera presets (Front, Back, Left, Right, Top, Bottom, IsoFL, IsoBR)");
            
            ImGui::Spacing();
            ImGui::Text("UI Controls:");
            ImGui::BulletText("F1 - Toggle this help");
            ImGui::BulletText("Tab - Toggle settings panel");
            ImGui::BulletText("F11 - Toggle fullscreen (if supported)");
            
            ImGui::Spacing();
            ImGui::Text("Object Interaction:");
            ImGui::BulletText("Click on objects to select them");
            ImGui::BulletText("Selected objects show in the settings panel");
            ImGui::BulletText("Use gizmos to transform selected objects");
            
            ImGui::Spacing();
            ImGui::Text("Delete & Duplicate Keys:");
            ImGui::BulletText("Delete - Delete selected object or light");
            ImGui::BulletText("Ctrl+D - Duplicate selected object");
            
            ImGui::Spacing();
            ImGui::Text("File Operations:");
            ImGui::BulletText("Ctrl+O - Import asset");
            ImGui::BulletText("Ctrl+E - Export scene");
        }
        ImGui::End();
    }
    
    // JSON Operations Help Dialog
    if (m_showJsonOpsHelp) {
        ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("JSON Operations v1.3 Reference", &m_showJsonOpsHelp)) {
            ImGui::TextWrapped("Complete reference for JSON Operations v1.3:");
            ImGui::Separator();
            
            if (ImGui::CollapsingHeader("Object Management", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BulletText("load - Load models: {\"op\":\"load\", \"path\":\"model.obj\", \"name\":\"MyObject\"}");
                ImGui::BulletText("duplicate - Copy objects: {\"op\":\"duplicate\", \"source\":\"Original\", \"name\":\"Copy\", \"position\":[3,0,0]}");
                ImGui::BulletText("remove/delete - Remove: {\"op\":\"remove\", \"name\":\"ObjectName\"} (aliases)");
                ImGui::BulletText("select - Select object: {\"op\":\"select\", \"name\":\"ObjectName\"}");
                ImGui::BulletText("transform - Apply transforms: {\"op\":\"transform\", \"name\":\"Obj\", \"translate\":[1,0,0]}");
            }
            
            if (ImGui::CollapsingHeader("Camera Control")) {
                ImGui::BulletText("set_camera - Set position/target: {\"op\":\"set_camera\", \"position\":[0,2,5], \"target\":[0,0,0]}");
                ImGui::BulletText("set_camera_preset - Quick views: {\"op\":\"set_camera_preset\", \"preset\":\"front|back|left|right|top|bottom|iso_fl|iso_br\"}");
                ImGui::BulletText("orbit_camera - Rotate around center: {\"op\":\"orbit_camera\", \"yaw\":45, \"pitch\":15}");
                ImGui::BulletText("frame_object - Focus on object: {\"op\":\"frame_object\", \"name\":\"ObjectName\"}");
            }
            
            if (ImGui::CollapsingHeader("Lighting")) {
                ImGui::BulletText("add_light - Add lights:");
                ImGui::Indent();
                ImGui::BulletText("Point: {\"op\":\"add_light\", \"type\":\"point\", \"position\":[0,5,0], \"intensity\":2.0}");
                ImGui::BulletText("Directional: {\"op\":\"add_light\", \"type\":\"directional\", \"direction\":[-1,-1,-1]}");
                ImGui::BulletText("Spot: {\"op\":\"add_light\", \"type\":\"spot\", \"position\":[0,5,0], \"direction\":[0,-1,0], \"inner_deg\":15, \"outer_deg\":30}");
                ImGui::Unindent();
            }
            
            if (ImGui::CollapsingHeader("Materials & Appearance")) {
                ImGui::BulletText("set_material - Modify materials: {\"op\":\"set_material\", \"target\":\"Obj\", \"material\":{\"color\":[1,0,0], \"roughness\":0.5}}");
                ImGui::BulletText("set_background - Solid: {\"op\":\"set_background\", \"color\":[0.2,0.4,0.8]}");
                ImGui::BulletText("set_background - Gradient: {\"op\":\"set_background\", \"top\":[0.1,0.1,0.2], \"bottom\":[0.0,0.0,0.0]}");
                ImGui::BulletText("set_background - HDR/EXR: {\"op\":\"set_background\", \"hdr\":\"assets/env/studio.hdr|assets/env/studio.exr\"}");
                ImGui::BulletText("load_hdr_environment - Load HDR/EXR for IBL: {\"op\":\"load_hdr_environment\", \"path\":\"assets/env/studio.hdr|assets/env/studio.exr\"}");
                ImGui::BulletText("set_skybox_intensity - Set skybox brightness: {\"op\":\"set_skybox_intensity\", \"value\":1.5}");
                ImGui::BulletText("set_ibl_intensity - Set IBL strength: {\"op\":\"set_ibl_intensity\", \"value\":2.0}");
                ImGui::BulletText("exposure - Adjust exposure: {\"op\":\"exposure\", \"value\":-1.0}");
                ImGui::BulletText("tone_map - Configure tone mapping: {\"op\":\"tone_map\", \"type\":\"filmic|linear|reinhard|aces\"}");
            }
            
            if (ImGui::CollapsingHeader("Rendering")) {
                ImGui::BulletText("render_image - Render to PNG: {\"op\":\"render_image\", \"path\":\"output.png\", \"width\":800, \"height\":600}");
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextWrapped("Tips:");
            ImGui::BulletText("All vector parameters use [x, y, z] format");
            ImGui::BulletText("Rotation values are in degrees, not radians");
            ImGui::BulletText("See examples/json-ops/ for complete examples");
            ImGui::BulletText("Schema validation: schemas/json_ops_v1.json");
            ImGui::BulletText("Use console command 'json_ops' for quick reference");
        }
        ImGui::End();
    }
    
    // About Dialog
    if (m_showAboutDialog) {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("About Glint3D", &m_showAboutDialog)) {
            ImGui::Text("Glint3D Engine");
            ImGui::Text("Version 0.3.0");
            ImGui::Separator();
            
            ImGui::TextWrapped("A modern 3D rendering engine with dual OpenGL rasterization and CPU raytracing capabilities.");
            
            ImGui::Spacing();
            ImGui::Text("Features:");
            ImGui::BulletText("OpenGL/WebGL rendering");
            ImGui::BulletText("CPU raytracer with BVH acceleration");
            ImGui::BulletText("PBR material system");
            ImGui::BulletText("JSON Operations API v1.3");
            ImGui::BulletText("Cross-platform (Desktop & Web)");
            ImGui::BulletText("Point, Directional, and Spot lighting");
            ImGui::BulletText("Camera presets and controls");
            ImGui::BulletText("Asset import (OBJ, glTF, FBX via Assimp)");
            
            ImGui::Spacing();
            ImGui::Text("Built with:");
            ImGui::BulletText("OpenGL 3.3+");
            ImGui::BulletText("ImGui for desktop UI");
            ImGui::BulletText("React + Tailwind for web UI");
            ImGui::BulletText("Emscripten for web deployment");
            ImGui::BulletText("GLM for mathematics");
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("© 2025 Glint3D Project");
        }
        ImGui::End();
    }
}
