// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/render_command.h","purpose":"Declares the render command handler for the Glint CLI platform.","exports":["glint::cli::RenderCommand"],"depends_on":["glint/cli/command_dispatcher.h","glint/cli/services/run_manifest_writer.h"],"notes":["determinism_logging","run_manifest_output","offscreen_render_integration"]}
// Human Summary
// Handles the `glint render` command, integrating with the engine's offscreen render system and writing deterministic run manifests to `renders/<name>/run.json`.

#pragma once

#include "glint/cli/command_dispatcher.h"
#include "glint/cli/services/run_manifest_writer.h"

/**
 * @file render_command.h
 * @brief Command handler for `glint render`.
 */

namespace glint::cli {

/**
 * @brief Implements the `glint render` command with determinism logging.
 *
 * This command:
 * - Loads a scene or JSON Ops file
 * - Performs offscreen rendering
 * - Captures complete provenance metadata
 * - Writes `renders/<name>/run.json` with reproducibility data
 * - Supports structured output via `--json` flag
 */
class RenderCommand : public ICommand {
public:
    /**
     * @brief Execute the render command.
     * @param context Execution context with arguments and output emitter.
     * @return Exit code indicating success or failure.
     */
    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    struct RenderOptions {
        std::string outputPath;
        std::string inputPath;
        std::string opsPath;
        int width = 800;
        int height = 600;
        bool denoise = false;
        bool raytrace = false;
        std::string renderName = "default";
        bool writeManifest = true;
    };

    /**
     * @brief Parse command-line arguments into render options.
     * @param args Command arguments.
     * @param options Output options structure.
     * @param errorMessage Output error message if parsing fails.
     * @return Exit code (Success or error code).
     */
    CLIExitCode parseArguments(const std::vector<std::string>& args,
                               RenderOptions& options,
                               std::string& errorMessage) const;

    /**
     * @brief Execute the render operation.
     * @param context Execution context.
     * @param options Parsed render options.
     * @return Exit code indicating success or failure.
     */
    CLIExitCode executeRender(const CommandExecutionContext& context,
                              const RenderOptions& options) const;

    /**
     * @brief Capture platform metadata for the run manifest.
     * @return Platform metadata structure.
     */
    services::PlatformMetadata capturePlatformMetadata() const;

    /**
     * @brief Capture engine metadata (version, modules, assets) for the run manifest.
     * @return Engine metadata structure.
     */
    services::EngineMetadata captureEngineMetadata() const;

    /**
     * @brief Compute determinism metadata (RNG seed, digests, shader hashes).
     * @param options Render options.
     * @return Determinism metadata structure.
     */
    services::DeterminismMetadata captureDeterminismMetadata(const RenderOptions& options) const;
};

} // namespace glint::cli
