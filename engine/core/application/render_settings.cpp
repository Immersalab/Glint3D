#include "render_settings.h"
#include <algorithm>

ToneMappingMode RenderSettings::parseToneMapping(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "linear") return ToneMappingMode::Linear;
    if (lower == "reinhard") return ToneMappingMode::Reinhard;
    if (lower == "aces") return ToneMappingMode::ACES;
    if (lower == "filmic") return ToneMappingMode::Filmic;
    
    return ToneMappingMode::Linear; // default fallback
}

std::string RenderSettings::toneMappingToString(ToneMappingMode mode) {
    switch (mode) {
        case ToneMappingMode::Linear: return "linear";
        case ToneMappingMode::Reinhard: return "reinhard";
        case ToneMappingMode::ACES: return "aces";
        case ToneMappingMode::Filmic: return "filmic";
        default: return "linear";
    }
}

bool RenderSettings::isValidToneMapping(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    return lower == "linear" || lower == "reinhard" || lower == "aces" || lower == "filmic";
}