#include "path_security.h"
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace PathSecurity {
    
    static std::string s_assetRoot;

    bool setAssetRoot(const std::string& assetRoot) {
        if (assetRoot.empty()) {
            return false;
        }

        try {
            // Convert to absolute path and normalize
            std::filesystem::path rootPath = std::filesystem::absolute(assetRoot);
            rootPath = rootPath.lexically_normal();
            
            // Check if the directory exists
            if (!std::filesystem::exists(rootPath) || !std::filesystem::is_directory(rootPath)) {
                return false;
            }
            
            s_assetRoot = rootPath.string();
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    std::string getAssetRoot() {
        return s_assetRoot;
    }

    bool isAssetRootSet() {
        return !s_assetRoot.empty();
    }

    void clearAssetRoot() {
        s_assetRoot.clear();
    }

    ValidationResult validatePath(const std::string& path) {
        if (!isAssetRootSet()) {
            return ValidationResult::RootNotSet;
        }

        if (path.empty()) {
            return ValidationResult::EmptyPath;
        }

        // Check for invalid characters that could be used maliciously
        // Allow normal path characters but reject control characters and some special ones
        for (char c : path) {
            if (c < 32 || c == 127) {  // Control characters
                return ValidationResult::InvalidCharacters;
            }
        }

        try {
            std::filesystem::path inputPath(path);
            std::filesystem::path rootPath(s_assetRoot);
            
            // If path is relative, make it relative to asset root
            std::filesystem::path resolvedPath;
            if (inputPath.is_absolute()) {
                resolvedPath = inputPath;
            } else {
                resolvedPath = rootPath / inputPath;
            }
            
            // Normalize the path to resolve any . and .. components
            resolvedPath = resolvedPath.lexically_normal();
            
            // Check for explicit traversal attempts in the original path
            std::string pathStr = inputPath.string();
            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');  // Normalize separators
            
            // Look for suspicious patterns
            if (pathStr.find("..") != std::string::npos) {
                return ValidationResult::InvalidTraversal;
            }
            
            // Check if the resolved path is within the asset root
            std::filesystem::path normalizedRoot = rootPath.lexically_normal();
            std::string resolvedStr = resolvedPath.string();
            std::string rootStr = normalizedRoot.string();
            
            // Ensure both paths end with a separator for proper prefix checking
            if (!rootStr.empty() && rootStr.back() != '/' && rootStr.back() != '\\') {
                rootStr += std::filesystem::path::preferred_separator;
            }
            if (!resolvedStr.empty() && resolvedStr.back() != '/' && resolvedStr.back() != '\\') {
                resolvedStr += std::filesystem::path::preferred_separator;
            }
            
            // Check if resolved path starts with root path
            if (resolvedStr.length() < rootStr.length() || 
                !std::equal(rootStr.begin(), rootStr.end(), resolvedStr.begin(),
                           [](char a, char b) {
                               return std::tolower(static_cast<unsigned char>(a)) == 
                                      std::tolower(static_cast<unsigned char>(b));
                           })) {
                return ValidationResult::OutsideRoot;
            }
            
            return ValidationResult::Valid;
            
        } catch (const std::exception&) {
            return ValidationResult::InvalidCharacters;
        }
    }

    std::string resolvePath(const std::string& path) {
        ValidationResult result = validatePath(path);
        if (result != ValidationResult::Valid) {
            return "";
        }

        try {
            std::filesystem::path inputPath(path);
            std::filesystem::path rootPath(s_assetRoot);
            
            std::filesystem::path resolvedPath;
            if (inputPath.is_absolute()) {
                resolvedPath = inputPath;
            } else {
                resolvedPath = rootPath / inputPath;
            }
            
            return resolvedPath.lexically_normal().string();
        } catch (const std::exception&) {
            return "";
        }
    }

    std::string getErrorMessage(ValidationResult result) {
        switch (result) {
            case ValidationResult::Valid:
                return "Path is valid";
            case ValidationResult::InvalidTraversal:
                return "Path contains directory traversal attempts (..)";
            case ValidationResult::OutsideRoot:
                return "Path resolves outside the asset root directory";
            case ValidationResult::InvalidCharacters:
                return "Path contains invalid characters";
            case ValidationResult::EmptyPath:
                return "Path is empty or invalid";
            case ValidationResult::RootNotSet:
                return "Asset root directory has not been configured";
            default:
                return "Unknown validation error";
        }
    }
}