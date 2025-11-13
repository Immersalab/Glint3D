// Machine Summary Block
// {"file":"cli/include/glint/cli/commands/convert_command.h","purpose":"Declares the convert command handler (stub) for the Glint CLI platform.","exports":["glint::cli::ConvertCommand"],"depends_on":["glint/cli/command_dispatcher.h"],"notes":["stub_implementation","asset_conversion","future_feature"]}
// Human Summary
// Stub implementation for `glint convert` command that handles asset and material format conversions. Full implementation gated pending asset pipeline integration.

#pragma once

#include "glint/cli/command_dispatcher.h"

/**
 * @file convert_command.h
 * @brief Stub command handler for `glint convert`.
 */

namespace glint::cli {

/**
 * @brief Stub implementation of the `glint convert` command.
 *
 * This command will eventually:
 * - Convert between model formats (OBJ, glTF, FBX, PLY, STL)
 * - Transform materials between PBR and legacy Phong
 * - Optimize meshes (decimation, LOD generation, atlas packing)
 * - Convert textures (PNG, JPG, KTX2, EXR, DDS)
 * - Batch process entire asset directories
 *
 * **Current Status**: Stub implementation. Returns RuntimeError with guidance.
 */
class ConvertCommand : public ICommand {
public:
    /**
     * @brief Execute the convert command stub.
     * @param context Execution context with arguments and output emitter.
     * @return RuntimeError exit code with informational message.
     */
    CLIExitCode run(const CommandExecutionContext& context) override;
};

} // namespace glint::cli
