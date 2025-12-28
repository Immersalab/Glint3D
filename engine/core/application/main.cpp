// Machine Summary Block
// {"file":"engine/core/application/main.cpp","purpose":"Entry point dispatching CLI verbs before launching the interactive application.","depends_on":["application_core.h","cli_parser.h","glint/cli/command_dispatcher.h"],"notes":["cli_dispatcher","ui_fallback","new_cli_platform"]}
// Human Summary
// Dispatches supported CLI verbs via CommandDispatcher, falling back to interactive UI mode when no command is recognized.

#include "application_core.h"
#include "cli_parser.h"
#include "glint/cli/command_dispatcher.h"
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static const char* GLINT_VERSION = "0.3.0";

int main(int argc, char** argv)
{
    // Try to dispatch via new CLI platform
    glint::cli::CommandDispatcher dispatcher;
    if (auto dispatched = dispatcher.tryRun(argc, argv)) {
        return *dispatched;
    }

    // No command recognized
    // If run with no arguments, show help instead of launching UI
    if (argc < 2) {
#ifdef _WIN32
        // Ensure Windows console uses UTF-8 so Unicode ASCII art renders correctly
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif
        CLIParser::printHelp();
        return 0;
    }

    // Unknown command - launch interactive UI mode as fallback
#ifdef _WIN32
    // Ensure Windows console uses UTF-8 so Unicode ASCII art renders correctly
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Logger::info("Glint 3D Engine v" + std::string(GLINT_VERSION));
    Logger::info("Unknown command - launching interactive UI mode");
    Logger::info("Use 'glint help' to see available commands");

    // Initialize application in UI mode
    auto* app = new ApplicationCore();
    if (!app->init("Glint 3D", 800, 600, false)) {
        Logger::error("Failed to initialize application");
        delete app;
        return static_cast<int>(CLIExitCode::RuntimeError);
    }

    app->run();
    delete app;
    return 0;
}
