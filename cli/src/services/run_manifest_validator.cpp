// Machine Summary Block
// {"file":"cli/src/services/run_manifest_validator.cpp","purpose":"Implements run manifest schema validation and checksum computation.","depends_on":["glint/cli/services/run_manifest_validator.h","rapidjson/document.h","rapidjson/error/en.h","<fstream>","<sstream>","<iomanip>","<algorithm>"],"notes":["schema_validation_v1_0_0","sha256_checksum","required_field_enforcement"]}
// Human Summary
// Validates run.json manifests against the v1.0.0 schema, checking required fields, types, and computing SHA-256 checksums for integrity verification.

#include "glint/cli/services/run_manifest_validator.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace glint::cli::services {

namespace {

// Simple SHA-256 implementation for checksums
// Based on FIPS 180-4 specification
class SHA256 {
public:
    SHA256() { reset(); }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            m_data[m_blocklen++] = data[i];
            if (m_blocklen == 64) {
                transform();
                m_bitlen += 512;
                m_blocklen = 0;
            }
        }
    }

    void update(const std::string& str) {
        update(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
    }

    std::string finalize() {
        uint32_t i = m_blocklen;
        if (m_blocklen < 56) {
            m_data[i++] = 0x80;
            while (i < 56) m_data[i++] = 0x00;
        } else {
            m_data[i++] = 0x80;
            while (i < 64) m_data[i++] = 0x00;
            transform();
            std::memset(m_data, 0, 56);
        }

        m_bitlen += m_blocklen * 8;
        m_data[63] = m_bitlen;
        m_data[62] = m_bitlen >> 8;
        m_data[61] = m_bitlen >> 16;
        m_data[60] = m_bitlen >> 24;
        m_data[59] = m_bitlen >> 32;
        m_data[58] = m_bitlen >> 40;
        m_data[57] = m_bitlen >> 48;
        m_data[56] = m_bitlen >> 56;
        transform();

        std::ostringstream oss;
        for (int i = 0; i < 8; ++i) {
            oss << std::hex << std::setfill('0') << std::setw(8) << m_state[i];
        }
        return oss.str();
    }

private:
    void reset() {
        m_bitlen = 0;
        m_blocklen = 0;
        m_state[0] = 0x6a09e667;
        m_state[1] = 0xbb67ae85;
        m_state[2] = 0x3c6ef372;
        m_state[3] = 0xa54ff53a;
        m_state[4] = 0x510e527f;
        m_state[5] = 0x9b05688c;
        m_state[6] = 0x1f83d9ab;
        m_state[7] = 0x5be0cd19;
    }

    void transform() {
        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        uint32_t m[64];
        for (int i = 0, j = 0; i < 16; ++i, j += 4) {
            m[i] = (m_data[j] << 24) | (m_data[j + 1] << 16) | (m_data[j + 2] << 8) | (m_data[j + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = ror(m[i - 15], 7) ^ ror(m[i - 15], 18) ^ (m[i - 15] >> 3);
            uint32_t s1 = ror(m[i - 2], 17) ^ ror(m[i - 2], 19) ^ (m[i - 2] >> 10);
            m[i] = m[i - 16] + s0 + m[i - 7] + s1;
        }

        uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
        uint32_t e = m_state[4], f = m_state[5], g = m_state[6], h = m_state[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h + S1 + ch + k[i] + m[i];
            uint32_t S0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }

        m_state[0] += a; m_state[1] += b; m_state[2] += c; m_state[3] += d;
        m_state[4] += e; m_state[5] += f; m_state[6] += g; m_state[7] += h;
    }

    static uint32_t ror(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }

    uint8_t m_data[64];
    uint32_t m_blocklen;
    uint64_t m_bitlen;
    uint32_t m_state[8];
};

bool hasField(const rapidjson::Value& obj, const char* name) {
    return obj.HasMember(name);
}

bool isString(const rapidjson::Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsString();
}

bool isNumber(const rapidjson::Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsNumber();
}

bool isObject(const rapidjson::Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsObject();
}

bool isArray(const rapidjson::Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsArray();
}

bool isBool(const rapidjson::Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsBool();
}

void addError(ValidationResult& result, const std::string& field,
              const std::string& message, const std::string& code) {
    result.errors.emplace_back(field, message, code);
    result.valid = false;
}

} // namespace

std::string ValidationResult::getSummary() const {
    if (isValid()) {
        return "Validation passed";
    }

    std::ostringstream oss;
    oss << "Validation failed with " << errors.size() << " error(s):\n";
    for (const auto& err : errors) {
        oss << "  - " << err.field << ": " << err.message << " [" << err.code << "]\n";
    }
    return oss.str();
}

ValidationResult RunManifestValidator::validate(const std::filesystem::path& manifestPath,
                                                bool computeChecksum) {
    ValidationResult result;

    if (!std::filesystem::exists(manifestPath)) {
        addError(result, "file", "Manifest file not found", "file_not_found");
        return result;
    }

    std::ifstream file(manifestPath);
    if (!file) {
        addError(result, "file", "Failed to open manifest file", "io_error");
        return result;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string content = oss.str();

    return validateContent(content, computeChecksum);
}

ValidationResult RunManifestValidator::validateContent(const std::string& jsonContent,
                                                      bool computeChecksum) {
    ValidationResult result;
    result.valid = true;

    // Parse JSON
    rapidjson::Document doc;
    rapidjson::ParseResult parseResult = doc.Parse(jsonContent.c_str());

    if (!parseResult) {
        std::ostringstream oss;
        oss << "JSON parse error: " << rapidjson::GetParseError_En(parseResult.Code())
            << " at offset " << parseResult.Offset();
        addError(result, "json", oss.str(), "parse_error");
        return result;
    }

    if (!doc.IsObject()) {
        addError(result, "root", "Root must be an object", "invalid_type");
        return result;
    }

    // Validate schema version
    validateSchemaVersion(&doc, result);

    // Validate required top-level fields
    validateRequiredFields(&doc, result);

    // Validate field types
    validateFieldTypes(&doc, result);

    // Validate sections
    validateCliSection(&doc, result);
    validatePlatformSection(&doc, result);
    validateEngineSection(&doc, result);
    validateDeterminismSection(&doc, result);
    validateOutputsSection(&doc, result);

    // Compute checksum if requested
    if (computeChecksum) {
        result.checksum = RunManifestValidator::computeChecksum(jsonContent);
    }

    return result;
}

std::string RunManifestValidator::computeChecksum(const std::string& content) {
    SHA256 sha256;
    sha256.update(content);
    return sha256.finalize();
}

bool RunManifestValidator::verifyChecksum(const std::filesystem::path& manifestPath,
                                         const std::string& expectedChecksum) {
    std::ifstream file(manifestPath);
    if (!file) {
        return false;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string content = oss.str();

    std::string actualChecksum = computeChecksum(content);
    return actualChecksum == expectedChecksum;
}

void RunManifestValidator::validateRequiredFields(const void* docPtr, ValidationResult& result) {
    const auto& doc = *static_cast<const rapidjson::Document*>(docPtr);

    const char* requiredFields[] = {
        "schema_version", "run_id", "timestamp_utc", "cli", "platform",
        "engine", "determinism", "outputs"
    };

    for (const char* field : requiredFields) {
        if (!doc.HasMember(field)) {
            addError(result, field, "Required field missing", "missing_field");
        }
    }
}

void RunManifestValidator::validateFieldTypes(const void* docPtr, ValidationResult& result) {
    const auto& doc = *static_cast<const rapidjson::Document*>(docPtr);

    if (!isString(doc, "schema_version")) {
        addError(result, "schema_version", "Must be a string", "invalid_type");
    }
    if (!isString(doc, "run_id")) {
        addError(result, "run_id", "Must be a string", "invalid_type");
    }
    if (!isString(doc, "timestamp_utc")) {
        addError(result, "timestamp_utc", "Must be a string", "invalid_type");
    }
    if (!isObject(doc, "cli")) {
        addError(result, "cli", "Must be an object", "invalid_type");
    }
    if (!isObject(doc, "platform")) {
        addError(result, "platform", "Must be an object", "invalid_type");
    }
    if (!isObject(doc, "engine")) {
        addError(result, "engine", "Must be an object", "invalid_type");
    }
    if (!isObject(doc, "determinism")) {
        addError(result, "determinism", "Must be an object", "invalid_type");
    }
    if (!isObject(doc, "outputs")) {
        addError(result, "outputs", "Must be an object", "invalid_type");
    }
}

void RunManifestValidator::validateSchemaVersion(const void* docPtr, ValidationResult& result) {
    const auto& doc = *static_cast<const rapidjson::Document*>(docPtr);

    if (isString(doc, "schema_version")) {
        std::string version = doc["schema_version"].GetString();
        result.schemaVersion = version;

        // Currently only support 1.0.0
        if (version != "1.0.0") {
            addError(result, "schema_version",
                    "Unsupported schema version (expected 1.0.0, got " + version + ")",
                    "unsupported_version");
        }
    }
}

void RunManifestValidator::validateCliSection(const void* docPtr, ValidationResult& result) {
    const auto& doc = *static_cast<const rapidjson::Document*>(docPtr);

    if (!isObject(doc, "cli")) return;

    const auto& cli = doc["cli"];

    if (!isString(cli, "command")) {
        addError(result, "cli.command", "Must be a string", "invalid_type");
    }
    if (!isArray(cli, "arguments")) {
        addError(result, "cli.arguments", "Must be an array", "invalid_type");
    }
    if (!isNumber(cli, "exit_code")) {
        addError(result, "cli.exit_code", "Must be a number", "invalid_type");
    }
    if (!isBool(cli, "json_mode")) {
        addError(result, "cli.json_mode", "Must be a boolean", "invalid_type");
    }
}

void RunManifestValidator::validatePlatformSection(const void* docPtr, ValidationResult& result) {
    const auto& doc = *static_cast<const rapidjson::Document*>(docPtr);

    if (!isObject(doc, "platform")) return;

    const auto& platform = doc["platform"];

    const char* requiredFields[] = {"os", "cpu", "gpu", "driver_version", "kernel"};
    for (const char* field : requiredFields) {
        if (!isString(platform, field)) {
            std::string fullField = std::string("platform.") + field;
            addError(result, fullField, "Must be a string", "invalid_type");
        }
    }
}

void RunManifestValidator::validateEngineSection(const void* docPtr, ValidationResult& result) {
    const auto& doc = *static_cast<const rapidjson::Document*>(docPtr);

    if (!isObject(doc, "engine")) return;

    const auto& engine = doc["engine"];

    if (!isString(engine, "version")) {
        addError(result, "engine.version", "Must be a string", "invalid_type");
    }
    if (!isArray(engine, "modules")) {
        addError(result, "engine.modules", "Must be an array", "invalid_type");
    }
    if (!isArray(engine, "assets")) {
        addError(result, "engine.assets", "Must be an array", "invalid_type");
    }
}

void RunManifestValidator::validateDeterminismSection(const void* docPtr, ValidationResult& result) {
    const auto& doc = *static_cast<const rapidjson::Document*>(docPtr);

    if (!isObject(doc, "determinism")) return;

    const auto& determinism = doc["determinism"];

    if (!isNumber(determinism, "rng_seed")) {
        addError(result, "determinism.rng_seed", "Must be a number", "invalid_type");
    }
    if (!isArray(determinism, "frame_batch")) {
        addError(result, "determinism.frame_batch", "Must be an array", "invalid_type");
    }
}

void RunManifestValidator::validateOutputsSection(const void* docPtr, ValidationResult& result) {
    const auto& doc = *static_cast<const rapidjson::Document*>(docPtr);

    if (!isObject(doc, "outputs")) return;

    const auto& outputs = doc["outputs"];

    if (!isString(outputs, "render_path")) {
        addError(result, "outputs.render_path", "Must be a string", "invalid_type");
    }
    if (!isArray(outputs, "frames")) {
        addError(result, "outputs.frames", "Must be an array", "invalid_type");
    }
    if (!isArray(outputs, "warnings")) {
        addError(result, "outputs.warnings", "Must be an array", "invalid_type");
    }
}

} // namespace glint::cli::services
