// Machine Summary Block
// {"file":"cli/src/commands/ui_command.cpp","purpose":"Implements the UI command for launching the interactive application.","depends_on":["glint/cli/commands/ui_command.h","glint/cli/logger.h","application_core.h","<iostream>"],"notes":["launches_application_core","blocks_until_window_closed"]}
// Human Summary
// Implements the UI command by initializing and running the ApplicationCore interactive window.

#include "glint/cli/commands/ui_command.h"
#include "glint/cli/logger.h"
#include "application_core.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <cstdlib>

namespace glint::cli {

namespace {

constexpr const char* kManifestName = "glint.project.json";

std::optional<std::filesystem::path> findWorkspaceByManifest(const std::filesystem::path& start)
{
    std::error_code existsError;
    std::filesystem::path cursor = start;
    while (!cursor.empty()) {
        const std::filesystem::path manifest = cursor / kManifestName;
        if (std::filesystem::exists(manifest, existsError) && !existsError) {
            return cursor;
        }

        const std::filesystem::path parent = cursor.parent_path();
        if (parent == cursor) {
            break;
        }
        cursor = parent;
    }
    return std::nullopt;
}

std::filesystem::path normalizeWorkspace(const std::filesystem::path& path)
{
    std::error_code canonicalError;
    auto canonical = std::filesystem::weakly_canonical(path, canonicalError);
    if (!canonicalError) {
        return canonical;
    }
    return path.lexically_normal();
}

std::filesystem::path resolveWorkspaceRoot(const CommandExecutionContext& context,
                                           bool& workspaceDetected)
{
    workspaceDetected = false;
    std::filesystem::path workspace;

    for (size_t i = 0; i + 1 < context.arguments.size(); ++i) {
        if (context.arguments[i] == "--workspace") {
            workspace = context.arguments[i + 1];
            workspaceDetected = true;
            break;
        }
    }

    if (workspace.empty()) {
        if (const char* env = std::getenv("GLINT_WORKSPACE"); env && *env) {
            workspace = env;
            workspaceDetected = true;
        }
    }

    if (workspace.empty() && !context.globals.projectPath.empty()) {
        workspace = std::filesystem::path(context.globals.projectPath).parent_path();
        workspaceDetected = true;
    }

    if (!workspaceDetected) {
        if (auto manifestRoot = findWorkspaceByManifest(std::filesystem::current_path())) {
            workspace = *manifestRoot;
            workspaceDetected = true;
        }
    }

    if (workspace.empty()) {
        workspace = std::filesystem::current_path();
    }

    return normalizeWorkspace(workspace);
}

} // namespace

CLIExitCode UiCommand::run(const CommandExecutionContext& context)
{
    // Resolve workspace from flag or active project
    bool workspaceDetected = false;
    const std::filesystem::path workspacePath = resolveWorkspaceRoot(context, workspaceDetected);

    std::error_code cwdEc;
    std::filesystem::current_path(workspacePath, cwdEc);
    if (cwdEc) {
        Logger::warn("Failed to change to workspace directory: " + cwdEc.message());
    }

    // Log UI launch
    if (!context.globals.jsonOutput) {
        Logger::info("Launching Glint3D interactive UI...");
        Logger::info("Workspace: " + workspacePath.generic_string());
        Logger::info("Close the window or press ESC to exit.");
    }

    // Initialize the application
    auto* app = new ApplicationCore();
    if (workspaceDetected) {
        app->setWorkspaceRoot(workspacePath);
    }
    if (!app->init("Glint 3D", 800, 600, false)) {
        Logger::error("Failed to initialize UI application");
        delete app;
        return CLIExitCode::RuntimeError;
    }

    // Run the application (blocks until window closes)
    app->run();

    // Cleanup
    delete app;

    if (!context.globals.jsonOutput) {
        Logger::info("UI closed successfully");
        Logger::info("Use Open Workspace / Save Workspace inside the UI to switch or persist changes.");
    }

    return CLIExitCode::Success;
}

} // namespace glint::cli
