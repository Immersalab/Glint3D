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

private:
    void renderMainMenuBar(const UIState& state);
    void renderSettingsPanel(const UIState& state);
    void renderPerformanceHUD(const UIState& state);
    void renderConsole(const UIState& state);
    void setupDarkTheme();
};