// Machine Summary Block
// {"file":"cli/src/ndjson_emitter.cpp","purpose":"Implements the NdjsonEmitter stream helper for CLI commands.","depends_on":["glint/cli/ndjson_emitter.h"],"notes":["lightweight_wrapper","flushes_after_emit"]}
// Human Summary
// Wraps RapidJSON usage for NDJSON streaming so commands can output structured events without duplicating boilerplate.

#include "glint/cli/ndjson_emitter.h"

namespace glint::cli {

NdjsonEmitter::NdjsonEmitter(std::ostream& output)
    : m_output(output)
{
}

} // namespace glint::cli
