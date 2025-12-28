// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/ui_command.h","purpose":"Declares the UI command for launching the interactive application.","exports":["glint::cli::UiCommand"],"depends_on":["glint/cli/command_dispatcher.h"],"notes":["launches_interactive_ui","desktop_only"]}
// Human Summary
// Provides the UI command that launches the interactive Glint3D application with ImGui interface.

#pragma once

#include "glint/cli/command_dispatcher.h"

namespace glint::cli {

/**
 * @brief Launches the interactive Glint3D UI application.
 *
 * The UI command starts the full desktop application with:
 * - OpenGL viewport for real-time scene rendering
 * - ImGui panels for scene hierarchy, object properties, and controls
 * - Interactive camera controls
 * - Live material and lighting editing
 *
 * This command is desktop-only and requires OpenGL/GLFW support.
 */
class UiCommand : public ICommand {
public:
    /**
     * @brief Execute the UI command to launch the interactive application.
     * @param context Command execution context with arguments
     * @return CLIExitCode::Success if UI launched successfully, error code otherwise
     *
     * @note This command blocks until the UI window is closed.
     * @note Any command-line arguments are currently ignored.
     */
    CLIExitCode run(const CommandExecutionContext& context) override;
};

} // namespace glint::cli
