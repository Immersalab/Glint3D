// Machine Summary Block
// {"file":"engine/core/rendering/render_utils.h","purpose":"Provides helpers for locating render output paths and filenames.","exports":["RenderUtils::findProjectRoot","RenderUtils::getDefaultOutputPath","RenderUtils::processOutputPath"],"depends_on":["filesystem","chrono"],"notes":["default_outputs_under_output_renders"]}
// Human Summary
// Utilities for finding the project root and generating render output paths inside output/renders.

#pragma once

#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace RenderUtils {
    // Find project root directory (contains CMakeLists.txt, engine/ folder, etc.)
    inline std::string findProjectRoot() {
        std::filesystem::path current = std::filesystem::current_path();
        
        // Look for project indicators going up the directory tree
        while (current.has_parent_path() && current != current.parent_path()) {
            // Check for project root indicators
            if (std::filesystem::exists(current / "CMakeLists.txt") && 
                std::filesystem::exists(current / "engine") && 
                std::filesystem::exists(current / "examples")) {
                return current.string();
            }
            current = current.parent_path();
        }
        
        // If we can't find the project root, use current directory
        return std::filesystem::current_path().string();
    }
    
    // Get default output directory path (always project root + output/renders)
    inline std::string getDefaultOutputDir() {
        return findProjectRoot() + "/output/renders";
    }
    
    // Generate a unique filename with timestamp
    inline std::string generateTimestampFilename(const std::string& prefix = "render", const std::string& extension = ".png") {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << prefix << "_";
        
        // Use safe localtime function for Windows
        #ifdef _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &time_t);
        ss << std::put_time(&timeinfo, "%Y%m%d_%H%M%S");
        #else
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        #endif
        
        ss << "_" << std::setfill('0') << std::setw(3) << ms.count()
           << extension;
        return ss.str();
    }
    
    // Get default output path with optional custom filename
    inline std::string getDefaultOutputPath(const std::string& filename = "") {
        std::string outputDir = getDefaultOutputDir();
        
        // Ensure output directory exists
        std::filesystem::create_directories(outputDir);
        
        std::string outputFile;
        if (filename.empty()) {
            outputFile = generateTimestampFilename();
        } else {
            outputFile = filename;
        }
        
        return outputDir + "/" + outputFile;
    }
    
    // Process output path - if empty or just filename, use default directory
    inline std::string processOutputPath(const std::string& inputPath) {
        if (inputPath.empty()) {
            return getDefaultOutputPath();
        }
        
        // Check if path contains directory separators
        if (inputPath.find('/') == std::string::npos && inputPath.find('\\') == std::string::npos) {
            // Just a filename, use default directory
            return getDefaultOutputPath(inputPath);
        }
        
        // Full path provided, ensure parent directory exists
        std::filesystem::path p(inputPath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        
        return inputPath;
    }
}
