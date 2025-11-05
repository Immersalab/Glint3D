#pragma once

#include <string>

namespace PathSecurity {
    /**
     * @brief Result of path validation
     */
    enum class ValidationResult {
        Valid,              // Path is safe and within bounds
        InvalidTraversal,   // Path contains traversal attempts (e.g., ..)
        OutsideRoot,        // Path resolves outside the asset root
        InvalidCharacters,  // Path contains invalid characters
        EmptyPath,          // Path is empty or invalid
        RootNotSet          // No asset root has been configured
    };

    /**
     * @brief Initialize the path security system with an asset root directory
     * @param assetRoot Absolute path to the root directory for all assets
     * @return True if the root was set successfully, false if the path is invalid
     */
    bool setAssetRoot(const std::string& assetRoot);

    /**
     * @brief Get the currently configured asset root
     * @return The asset root path, or empty string if not set
     */
    std::string getAssetRoot();

    /**
     * @brief Validate a path against the asset root and check for traversal attempts
     * @param path The path to validate (can be relative or absolute)
     * @return ValidationResult indicating if the path is safe
     */
    ValidationResult validatePath(const std::string& path);

    /**
     * @brief Normalize and resolve a path relative to the asset root
     * @param path The path to resolve
     * @return The normalized absolute path, or empty string if validation fails
     */
    std::string resolvePath(const std::string& path);

    /**
     * @brief Check if the asset root has been configured
     * @return True if an asset root is set
     */
    bool isAssetRootSet();

    /**
     * @brief Get a human-readable error message for a validation result
     * @param result The validation result
     * @return Error message string
     */
    std::string getErrorMessage(ValidationResult result);

    /**
     * @brief Clear the asset root (for testing purposes)
     */
    void clearAssetRoot();
}