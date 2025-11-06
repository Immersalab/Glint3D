// Machine Summary Block
// {"file":"cli/include/glint/cli/ndjson_emitter.h","purpose":"Declares a helper for writing deterministic NDJSON events.","exports":["glint::cli::NdjsonEmitter"],"depends_on":["rapidjson/stringbuffer.h","rapidjson/writer.h","<ostream>"],"notes":["stateless_builder","json_streaming","structured_output_contract"]}
// Human Summary
// Lightweight wrapper around RapidJSON that streams newline-delimited JSON events to an output stream for CLI commands.

#pragma once

#include <functional>
#include <ostream>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

/**
 * @file ndjson_emitter.h
 * @brief Streaming helper that writes newline-delimited JSON events.
 */

namespace glint::cli {

/**
 * @brief Emits NDJSON objects using RapidJSON writers.
 *
 * Commands supply a lambda that receives a `rapidjson::Writer` instance. The emitter
 * wraps the lambda with `StartObject`/`EndObject` calls and flushes the serialized
 * buffer to the configured output stream, guaranteeing one JSON object per line.
 */
class NdjsonEmitter {
public:
    /// @brief Construct an emitter that writes to the provided output stream.
    explicit NdjsonEmitter(std::ostream& output);

    /**
     * @brief Emit a single NDJSON object.
     * @param builder Lambda that writes key/value pairs using the provided writer.
     *
     * The lambda must only emit JSON object members; `StartObject`/`EndObject` are
     * managed by the emitter. The resulting JSON is written followed by a newline.
     */
    template <typename WriterFn>
    void emit(WriterFn&& builder) const
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        builder(writer);
        writer.EndObject();
        m_output << buffer.GetString() << '\n';
        m_output.flush();
    }

private:
    std::ostream& m_output;
};

} // namespace glint::cli
