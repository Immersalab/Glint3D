#pragma once
#include <string>
#include <vector>

// Cross-platform file dialog wrapper
class FileDialog 
{
public:
    struct Filter {
        std::string name;      // e.g., "3D Models"
        std::string pattern;   // e.g., "*.obj;*.gltf;*.fbx"
    };
    
    // Open file dialog - returns selected file path or empty string if cancelled
    static std::string openFile(const std::string& title = "Open File", 
                               const std::vector<Filter>& filters = {},
                               const std::string& defaultPath = "");
    
    // Save file dialog - returns selected file path or empty string if cancelled
    static std::string saveFile(const std::string& title = "Save File",
                               const std::vector<Filter>& filters = {},
                               const std::string& defaultPath = "",
                               const std::string& defaultName = "");
    
    // Common filter presets
    static std::vector<Filter> getAssetFilters();  // Combined models + JSON
    static std::vector<Filter> getModelFilters();
    static std::vector<Filter> getJSONFilters();
    static std::vector<Filter> getImageFilters();
    
    // Utility functions for file type detection
    static bool isSceneFile(const std::string& filepath);
    static bool isModelFile(const std::string& filepath);
    
private:
    static std::string platformOpenFile(const std::string& title,
                                       const std::vector<Filter>& filters,
                                       const std::string& defaultPath);
    static std::string platformSaveFile(const std::string& title,
                                       const std::vector<Filter>& filters,
                                       const std::string& defaultPath,
                                       const std::string& defaultName);
};