// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/ops_command.h","purpose":"Declares ops command for JSON operations execution (replaces legacy --ops flag).","exports":["glint::cli::OpsCommand"],"depends_on":["glint/cli/command_dispatcher.h"],"notes":["json_ops_v1","headless_rendering","replaces_legacy_ops_flag"]}
// Human Summary
// Implements the 'glint ops' command for executing JSON operations files, replacing the legacy --ops flag syntax.

#pragma once

#include "glint/cli/command_dispatcher.h"

/**
 * @file ops_command.h
 * @brief JSON operations command implementation
 */

namespace glint::cli {

/**
 * @brief Execute JSON operations files (replaces legacy --ops flag)
 *
 * This command provides the new syntax for JSON operations:
 *   glint ops <file.json> [options]
 *
 * Replaces legacy syntax:
 *   glint --ops <file.json> [options]
 *
 * @par Options
 * - --render [<output.png>] - Render to PNG after applying ops
 * - --selection-overlay - Render selected-object wireframe overlay in offscreen output
 * - --w <width> - Output width (default: 1024)
 * - --h <height> - Output height (default: 1024)
 * - --denoise - Enable denoising if available
 * - --raytrace - Force raytracing mode
 * - --samples <n> - MSAA sample count (default: 1)
 * - --refl-spp <n> - Reflection samples per pixel (default: 8)
 * - --asset-root <dir> - Restrict file access to directory
 * - --strict-schema - Validate operations against schema
 * - --schema-version <v> - Schema version (default: v1.3)
 * - --seed <n> - Random seed for deterministic rendering (default: 0)
 * - --tone <mode> - Tone mapping: linear, reinhard, aces, filmic (default: linear)
 * - --exposure <f> - Exposure adjustment in EV stops (default: 0.0)
 * - --gamma <f> - Gamma correction value (default: 2.2)
 *
 * @par Exit Codes
 * - 0: Success
 * - 2: Schema validation error
 * - 3: File not found
 * - 4: Runtime/render failure
 * - 5: Invalid argument
 */
class OpsCommand : public ICommand {
public:
    /**
     * @brief Execute JSON operations from file
     * @param context Command execution context with parsed arguments
     * @return CLIExitCode indicating success or specific failure mode
     */
    CLIExitCode run(const CommandExecutionContext& context) override;

private:
    struct OpsOptions {
        std::string opsFile;
        std::string outputFile;
        std::string assetRoot;
        std::string schemaVersion = "v1.3";

        int outputWidth = 1024;
        int outputHeight = 1024;
        int samples = 1;
        int reflectionSpp = 8;

        uint32_t seed = 0;
        std::string toneMapping = "linear";
        float exposure = 0.0f;
        float gamma = 2.2f;

        bool enableDenoise = false;
        bool forceRaytrace = false;
        bool strictSchema = false;
        bool shouldRender = false;
        bool selectionOverlay = false;
    };

    /**
     * @brief Parse ops command specific arguments
     * @param args Argument vector
     * @param options Output options structure
     * @param errorMsg Output error message on failure
     * @return true if parsing succeeded
     */
    static bool parseOpsArguments(const std::vector<std::string>& args,
                                  OpsOptions& options,
                                  std::string& errorMsg);

    /**
     * @brief Execute the ops file and optionally render
     * @param options Parsed command options
     * @param context Execution context for logging
     * @return CLIExitCode indicating result
     */
    static CLIExitCode executeOps(const OpsOptions& options,
                                  const CommandExecutionContext& context);
};

} // namespace glint::cli
