#pragma once
#include <string>

enum class ToneMappingMode {
    Linear,
    Reinhard,
    ACES,
    Filmic
};

struct RenderSettings {
    // Random seed for deterministic rendering
    uint32_t seed = 0;
    
    // Tone mapping mode
    ToneMappingMode toneMapping = ToneMappingMode::Linear;
    
    // Exposure adjustment (EV stops)
    float exposure = 0.0f;
    
    // Gamma correction
    float gamma = 2.2f;
    
    // Helper functions
    static ToneMappingMode parseToneMapping(const std::string& str);
    static std::string toneMappingToString(ToneMappingMode mode);
    static bool isValidToneMapping(const std::string& str);
};