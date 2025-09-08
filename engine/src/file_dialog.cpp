#include "file_dialog.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#endif

#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cctype>

std::string FileDialog::openFile(const std::string& title, 
                                const std::vector<Filter>& filters,
                                const std::string& defaultPath) 
{
    return platformOpenFile(title, filters, defaultPath);
}

std::string FileDialog::saveFile(const std::string& title,
                                const std::vector<Filter>& filters,
                                const std::string& defaultPath,
                                const std::string& defaultName)
{
    return platformSaveFile(title, filters, defaultPath, defaultName);
}

std::vector<FileDialog::Filter> FileDialog::getAssetFilters()
{
    return {
        {"All Assets", "*.obj;*.gltf;*.glb;*.fbx;*.dae;*.ply;*.stl;*.3ds;*.json"},
        {"3D Models", "*.obj;*.gltf;*.glb;*.fbx;*.dae;*.ply;*.stl;*.3ds"},
        {"Scene Files", "*.json"},
        {"Wavefront OBJ", "*.obj"},
        {"glTF Files", "*.gltf;*.glb"},
        {"FBX Files", "*.fbx"},
        {"COLLADA", "*.dae"},
        {"Stanford PLY", "*.ply"},
        {"STL Files", "*.stl"},
        {"3DS Files", "*.3ds"},
        {"JSON Files", "*.json"},
        {"All Files", "*.*"}
    };
}

std::vector<FileDialog::Filter> FileDialog::getModelFilters() 
{
    return {
        {"All 3D Models", "*.obj;*.gltf;*.glb;*.fbx;*.dae;*.ply;*.stl;*.3ds"},
        {"Wavefront OBJ", "*.obj"},
        {"glTF Files", "*.gltf;*.glb"},
        {"FBX Files", "*.fbx"},
        {"COLLADA", "*.dae"},
        {"Stanford PLY", "*.ply"},
        {"STL Files", "*.stl"},
        {"3DS Files", "*.3ds"},
        {"All Files", "*.*"}
    };
}

std::vector<FileDialog::Filter> FileDialog::getJSONFilters()
{
    return {
        {"JSON Scene Files", "*.json"},
        {"All Files", "*.*"}
    };
}

std::vector<FileDialog::Filter> FileDialog::getImageFilters()
{
    return {
        {"Image Files", "*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.hdr;*.exr"},
        {"PNG Files", "*.png"},
        {"JPEG Files", "*.jpg;*.jpeg"},
        {"HDR Files", "*.hdr"},
        {"EXR Files", "*.exr"},
        {"All Files", "*.*"}
    };
}

bool FileDialog::isSceneFile(const std::string& filepath)
{
    if (filepath.empty()) return false;
    
    // Extract file extension
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) return false;
    
    std::string extension = filepath.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == "json";
}

bool FileDialog::isModelFile(const std::string& filepath)
{
    if (filepath.empty()) return false;
    
    // Extract file extension
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) return false;
    
    std::string extension = filepath.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Check against supported model formats
    return extension == "obj" || extension == "gltf" || extension == "glb" || 
           extension == "fbx" || extension == "dae" || extension == "ply" || 
           extension == "stl" || extension == "3ds";
}

#ifdef _WIN32

static std::string convertToFilterString(const std::vector<FileDialog::Filter>& filters)
{
    std::ostringstream oss;
    for (const auto& filter : filters) {
        oss << filter.name << '\0' << filter.pattern << '\0';
    }
    oss << '\0'; // Double null terminator
    return oss.str();
}

std::string FileDialog::platformOpenFile(const std::string& title,
                                        const std::vector<Filter>& filters,
                                        const std::string& defaultPath)
{
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr; // Could pass GLFW window handle if needed
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    std::string filterStr;
    if (!filters.empty()) {
        filterStr = convertToFilterString(filters);
        ofn.lpstrFilter = filterStr.c_str();
        ofn.nFilterIndex = 1;
    }
    
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    
    if (!defaultPath.empty() && std::filesystem::exists(defaultPath)) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }
    
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }
    
    return ""; // Cancelled or error
}

std::string FileDialog::platformSaveFile(const std::string& title,
                                        const std::vector<Filter>& filters,
                                        const std::string& defaultPath,
                                        const std::string& defaultName)
{
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    // Copy default name to buffer
    if (!defaultName.empty() && defaultName.length() < sizeof(szFile) - 1) {
        strcpy_s(szFile, defaultName.c_str());
    }
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    std::string filterStr;
    if (!filters.empty()) {
        filterStr = convertToFilterString(filters);
        ofn.lpstrFilter = filterStr.c_str();
        ofn.nFilterIndex = 1;
    }
    
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    
    if (!defaultPath.empty() && std::filesystem::exists(defaultPath)) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }
    
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    
    if (GetSaveFileNameA(&ofn)) {
        return std::string(szFile);
    }
    
    return ""; // Cancelled or error
}

#else

// Linux/macOS placeholder - would need zenity/kdialog for Linux or native Cocoa for macOS
std::string FileDialog::platformOpenFile(const std::string& title,
                                        const std::vector<Filter>& filters,
                                        const std::string& defaultPath)
{
    // TODO: Implement for Linux (zenity) and macOS (Cocoa)
    // For now, return empty (fallback to CLI/drag-drop)
    return "";
}

std::string FileDialog::platformSaveFile(const std::string& title,
                                        const std::vector<Filter>& filters,
                                        const std::string& defaultPath,
                                        const std::string& defaultName)
{
    // TODO: Implement for Linux (zenity) and macOS (Cocoa)  
    return "";
}

#endif