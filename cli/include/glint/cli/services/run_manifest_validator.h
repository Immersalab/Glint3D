// Machine Summary Block
// {"file":"cli/include/glint/cli/services/run_manifest_validator.h","purpose":"Declares validation utilities for run manifest schema compliance and checksums.","exports":["glint::cli::services::RunManifestValidator","glint::cli::services::ValidationResult","glint::cli::services::ValidationError"],"depends_on":["<string>","<vector>","<optional>"],"notes":["schema_validation","checksum_verification","determinism_enforcement"]}
// Human Summary
// Provides schema validation and checksum verification for run.json manifests to ensure reproducibility and detect corruption.

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

/**
 * @file run_manifest_validator.h
 * @brief Schema validation and checksum verification for run manifests.
 */

namespace glint::cli::services {

/// @brief Describes a validation error encountered during manifest validation.
struct ValidationError {
    std::string field;          ///< Field path (e.g., "cli.command", "platform.os")
    std::string message;        ///< Human-readable error description
    std::string code;           ///< Machine-readable error code (e.g., "missing_field", "invalid_type")

    ValidationError(std::string f, std::string m, std::string c)
        : field(std::move(f)), message(std::move(m)), code(std::move(c)) {}
};

/// @brief Result of run manifest validation.
struct ValidationResult {
    bool valid = false;                           ///< True if manifest is valid
    std::vector<ValidationError> errors;          ///< List of validation errors
    std::string schemaVersion;                    ///< Detected schema version
    std::optional<std::string> checksum;          ///< Computed checksum (if requested)

    /// @brief Check if validation passed.
    bool isValid() const { return valid && errors.empty(); }

    /// @brief Get summary error message.
    std::string getSummary() const;
};

/**
 * @brief Validates run manifests against schema and computes checksums.
 *
 * Ensures that run.json files conform to the expected structure and contain
 * all required fields for reproducibility. Computes SHA-256 checksums for
 * content verification.
 */
class RunManifestValidator {
public:
    /**
     * @brief Validate a run manifest file.
     * @param manifestPath Path to run.json file.
     * @param computeChecksum Whether to compute SHA-256 checksum.
     * @return Validation result with errors (if any).
     */
    static ValidationResult validate(const std::filesystem::path& manifestPath,
                                     bool computeChecksum = true);

    /**
     * @brief Validate run manifest JSON content.
     * @param jsonContent Raw JSON string content.
     * @param computeChecksum Whether to compute SHA-256 checksum.
     * @return Validation result with errors (if any).
     */
    static ValidationResult validateContent(const std::string& jsonContent,
                                           bool computeChecksum = true);

    /**
     * @brief Compute SHA-256 checksum of manifest content.
     * @param content Manifest JSON content (normalized).
     * @return Hex-encoded SHA-256 checksum.
     */
    static std::string computeChecksum(const std::string& content);

    /**
     * @brief Verify manifest checksum matches expected value.
     * @param manifestPath Path to run.json file.
     * @param expectedChecksum Expected SHA-256 checksum.
     * @return True if checksum matches.
     */
    static bool verifyChecksum(const std::filesystem::path& manifestPath,
                               const std::string& expectedChecksum);

private:
    /**
     * @brief Validate required fields exist in manifest.
     * @param doc Parsed JSON document.
     * @param result Output validation result.
     */
    static void validateRequiredFields(const void* doc, ValidationResult& result);

    /**
     * @brief Validate field types match schema expectations.
     * @param doc Parsed JSON document.
     * @param result Output validation result.
     */
    static void validateFieldTypes(const void* doc, ValidationResult& result);

    /**
     * @brief Validate schema version is supported.
     * @param doc Parsed JSON document.
     * @param result Output validation result.
     */
    static void validateSchemaVersion(const void* doc, ValidationResult& result);

    /**
     * @brief Validate CLI section structure.
     * @param doc Parsed JSON document.
     * @param result Output validation result.
     */
    static void validateCliSection(const void* doc, ValidationResult& result);

    /**
     * @brief Validate platform section structure.
     * @param doc Parsed JSON document.
     * @param result Output validation result.
     */
    static void validatePlatformSection(const void* doc, ValidationResult& result);

    /**
     * @brief Validate engine section structure.
     * @param doc Parsed JSON document.
     * @param result Output validation result.
     */
    static void validateEngineSection(const void* doc, ValidationResult& result);

    /**
     * @brief Validate determinism section structure.
     * @param doc Parsed JSON document.
     * @param result Output validation result.
     */
    static void validateDeterminismSection(const void* doc, ValidationResult& result);

    /**
     * @brief Validate outputs section structure.
     * @param doc Parsed JSON document.
     * @param result Output validation result.
     */
    static void validateOutputsSection(const void* doc, ValidationResult& result);
};

} // namespace glint::cli::services
