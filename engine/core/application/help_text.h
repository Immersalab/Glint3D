// Machine Summary Block
// {"file":"engine/core/application/help_text.h","purpose":"Defines CLI banner and usage text for Glint3D.","exports":["for_each_glint_ascii","print_cli_help"],"depends_on":["cstdio","string"],"notes":["paths_reference_output_renders"]}
// Human Summary
// Banner and command-line help strings, including default render output directory guidance.

// Centralized, art-free help and intro utilities
#pragma once

#include <cstdio>
#include <functional>
#include <string>

// Emit the GLINT 3D ASCII banner (no sparkle). Reusable via callback.
static inline void for_each_glint_ascii(const std::function<void(const std::string&)>& emit)
{
    static const char* kGlintAscii[] = {
        "Welcome to...",
        "  _____ _      _____ _   _ _______ ____  _____",
        " / ____| |    |_   _| \\ | |__   __|___ \\|  __ \\",
        "| |  __| |      | | |  \\| |  | |    __) | |  | |",
        "| | |_ | |      | | | . ` |  | |   |__ <| |  | |",
        "| |__| | |____ _| |_| |\\  |  | |   ___) | |__| |",
        " \\_____|______|_____|_| \\_|  |_|  |____/|_____/",
        ""
    };
    for (const char* line : kGlintAscii) emit(std::string(line));
}

static inline void print_cli_help()
{
    // Print ASCII banner first
    for_each_glint_ascii([](const std::string& s){ std::printf("%s\n", s.c_str()); });
    std::printf("\n             3D Engine v0.3.0\n\n");
    std::printf("GLINT CLI PLATFORM v1.0\n");
    std::printf("=======================\n\n");
    std::printf("Usage:\n");
    std::printf("  glint                         # Launch interactive UI\n");
    std::printf("  glint <command> [options]     # Execute CLI command\n");
    std::printf("  glint help                    # Show detailed command reference\n");
    std::printf("  glint --version               # Print version\n\n");

    std::printf("Core Commands:\n");
    std::printf("  ui                   Launch interactive UI application\n");
    std::printf("  init                 Initialize new Glint project workspace\n");
    std::printf("  render               Render scenes with determinism logging\n");
    std::printf("  inspect              Inspect scenes, manifests, and project files\n");
    std::printf("  validate             Validate project manifests and configurations\n");
    std::printf("  clean                Remove build artifacts and caches\n\n");

    std::printf("Management Commands:\n");
    std::printf("  modules              Manage engine modules (list/enable/disable)\n");
    std::printf("  assets               Synchronize and manage asset packs\n");
    std::printf("  config               View and edit configuration settings\n");
    std::printf("  doctor               Diagnose environment and dependencies\n\n");

    std::printf("Legacy Compatibility:\n");
    std::printf("  ops <file> [options] Execute JSON operations (legacy --ops support)\n\n");

    std::printf("Global Flags:\n");
    std::printf("  --verbosity <level>  Set logging level (quiet|warn|info|debug)\n");
    std::printf("  --project <path>     Path to glint.project.json\n");
    std::printf("  --config <path>      Path to .glint/config.json\n");
    std::printf("  --json               Output structured NDJSON for automation\n\n");

    std::printf("Quick Examples:\n");
    std::printf("  glint init --template pbr my_project\n");
    std::printf("  glint render --input scene.obj --output result.png --width 1920 --height 1080\n");
    std::printf("  glint ops operations.json --render output.png --raytrace --denoise\n");
    std::printf("  glint doctor\n");
    std::printf("  glint modules list\n\n");

    std::printf("Exit Codes:\n");
    std::printf("  0  Success\n");
    std::printf("  2  Schema validation error\n");
    std::printf("  3  File not found\n");
    std::printf("  4  Runtime error\n");
    std::printf("  5  Unknown flag or invalid argument\n");
    std::printf("  6  Dependency error\n");
    std::printf("  7  Determinism validation failed\n\n");

    std::printf("Documentation:\n");
    std::printf("  Complete command reference:     docs/cli_command_reference.md\n");
    std::printf("  Project manifest specification: docs/project_manifest_spec.md\n");
    std::printf("  JSON Ops schema v1.3:           schemas/json_ops_v1.json\n");
    std::printf("  Examples and tutorials:         examples/README.md\n\n");

    std::printf("For command-specific help, use:\n");
    std::printf("  glint <command> --help\n");
}

static inline void emit_welcome_lines(const std::function<void(const std::string&)>& emit)
{
    emit("Type 'help' for console commands and JSON ops.");
    emit("See Help menu (top bar) for interactive guides and controls.");
}
