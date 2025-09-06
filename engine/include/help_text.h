// Centralized, art-free help and intro utilities
#pragma once

#include <cstdio>
#include <functional>
#include <string>

// Emit the GLINT 3D ASCII banner (no sparkle). Reusable via callback.
static inline void for_each_glint_ascii(const std::function<void(const std::string&)>& emit)
{
    static const char* kGlintAscii[] = {
        "   _____ _      _____ _   _ _______ ____  _____",
        "  / ____| |    |_   _| \\ | |__   __|___ \\|  __ \\",
        " | |  __| |      | | |  \\| |  | |    __) | |  | |",
        " | | |_ | |      | | | . ` |  | |   |__ <| |  | |",
        " | |__| | |____ _| |_| |\\  |  | |   ___) | |__| |",
        "  \\_____|______|_____|_| \\_|  |_|  |____/|_____/"
    };
    for (const char* line : kGlintAscii) emit(std::string(line));
}

static inline void print_cli_help()
{
    // Print ASCII banner first
    for_each_glint_ascii([](const std::string& s){ std::printf("%s\n", s.c_str()); });
    std::printf("\n             3D Engine v0.3.0\n\n");
    std::printf("Usage:\n");
    std::printf("  glint                          # Launch UI\n");
    std::printf("  glint --ops <file>             # Apply JSON ops headlessly\n");
    std::printf("  glint --ops <file> --render [<out.png>] [--w W --h H] [--denoise] [--raytrace]\n");
    std::printf("\nOptions:\n");
    std::printf("  --help                Show this help\n");
    std::printf("  --version             Print version\n");
    std::printf("  --ops <file>          JSON ops file to apply\n");
    std::printf("  --render [<png>]      Output PNG path for headless render (defaults to renders/ folder)\n");
    std::printf("  --w <int>             Output image width (default 1024)\n");
    std::printf("  --h <int>             Output image height (default 1024)\n");
    std::printf("  --denoise             Enable denoiser if available\n");
    std::printf("  --raytrace            Force raytracing mode for rendering\n");
    std::printf("  --strict-schema       Validate operations against schema strictly\n");
    std::printf("  --schema-version <v>  Schema version to validate against (default v1.3)\n");
    std::printf("  --log <level>         Set log level: quiet, warn, info, debug (default info)\n");
    std::printf("\nJSON Operations v1.3 (Core Operations):\n");
    std::printf("  Object:     load, duplicate, remove/delete, select, transform\n");
    std::printf("  Camera:     set_camera, set_camera_preset, orbit_camera, frame_object\n");
    std::printf("  Lighting:   add_light (point/directional/spot)\n");
    std::printf("  Materials:  set_material, set_background, exposure, tone_map\n");
    std::printf("  Rendering:  render_image\n");
    std::printf("\nExit Codes:\n");
    std::printf("  0  Success\n");
    std::printf("  2  Schema validation error (when using --strict-schema)\n");
    std::printf("  3  File not found\n");
    std::printf("  4  Runtime/render failure\n");
    std::printf("  5  Unknown flag or invalid argument\n");
    std::printf("\nExamples:\n");
    std::printf("  glint --ops examples/json-ops/duplicate-test.json --render output.png\n");
    std::printf("  glint --ops examples/json-ops/camera-preset-test.json --render --w 800 --h 600\n");
    std::printf("  glint --ops test.json --strict-schema --log debug --render result.png\n");
    std::printf("\nDocumentation:\n");
    std::printf("  See examples/README.md for operation details and examples/json-ops/ for samples\n");
    std::printf("  Schema validation: schemas/json_ops_v1.json\n");
}

static inline void emit_welcome_lines(const std::function<void(const std::string&)>& emit)
{
    emit("Glint 3D Engine v0.3.0");
    emit("");
    emit("Welcome to Glint3D! Type 'help' for console commands and JSON ops.");
    emit("See Help menu (top bar) for interactive guides and controls.");
}
