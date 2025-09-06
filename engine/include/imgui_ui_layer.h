#pragma once
#include "ui_bridge.h"

class ImGuiUILayer : public IUILayer
{
public:
    ImGuiUILayer();
    ~ImGuiUILayer();

    // IUILayer interface
    bool init(int windowWidth, int windowHeight) override;
    void shutdown() override;
    void render(const UIState& state) override;
    void handleResize(int width, int height) override;
    
    // Handle UI-specific commands (like toggling panels)
    void handleCommand(const UICommandData& cmd);

private:
    void renderMainMenuBar(const UIState& state);
    void renderSettingsPanel(const UIState& state);
    void renderPerformanceHUD(const UIState& state);
    void renderConsole(const UIState& state);
    void renderHelpDialogs();
    void setupDarkTheme();
    
    // Local UI state that can be toggled independently
    bool m_showSettingsPanel = true;
    bool m_showPerfHUD = false;
    
    // Help dialogs
    bool m_showControlsHelp = false;
    bool m_showJsonOpsHelp = false;
    bool m_showAboutDialog = false;
};