#pragma once
#include <functional>
#include <string>
#include "ui_bridge.h"

namespace panels {

// Terminal-style scene tree (ASCII/box-drawing) on the left
// Renders a tree using UIState.objectNames and optional UIState.objectParentIndex (-1 for roots)
// Dispatches UICommand events via onCommand
void RenderSceneTree(const UIState& state, const std::function<void(const UICommandData&)>& onCommand);

}

