#include "image_io.h"

#include <algorithm>
#include <cctype>
#include <string>

// stb_image for LDR and Radiance .hdr
#include "stb_image.h"

// TinyEXR for .exr (optional)
#ifdef EXR_ENABLED
#define TINYEXR_USE_MINIZ 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"
#endif

namespace {

inline std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

inline bool HasExtension(const std::string& path, const char* ext) {
    std::string p = ToLower(path);
    std::string e = ToLower(std::string(ext));
    if (p.size() < e.size()) return false;
    return p.compare(p.size() - e.size(), e.size(), e) == 0;
}

template <typename T>
void FlipY(T* data, int width, int height, int channels) {
    const int stride = width * channels;
    for (int y = 0; y < height / 2; ++y) {
        T* rowTop = data + y * stride;
        T* rowBottom = data + (height - 1 - y) * stride;
        for (int x = 0; x < stride; ++x) {
            std::swap(rowTop[x], rowBottom[x]);
        }
    }
}

} // namespace

namespace ImageIO {

bool LoadImageFloat(const std::string& path, ImageDataFloat& out, bool flipY) {
    out = {};

    // .exr via TinyEXR (returns RGBA float32 always)
    if (HasExtension(path, ".exr")) {
#ifndef EXR_ENABLED
        // EXR disabled at build time
        return false;
#else
        int width = 0, height = 0;
        const char* err = nullptr;
        float* rgba = nullptr;
        int ret = LoadEXR(&rgba, &width, &height, path.c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
            if (err) FreeEXRErrorMessage(err);
            return false;
        }

        out.width = width;
        out.height = height;
        out.channels = 4;
        out.pixels.assign(rgba, rgba + (static_cast<size_t>(width) * height * 4));
        free(rgba);

        if (flipY) {
            FlipY(out.pixels.data(), out.width, out.height, out.channels);
        }
        return true;
#endif
    }

    // .hdr via stb_image (float32 RGB(A))
    if (HasExtension(path, ".hdr")) {
        stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
        int w = 0, h = 0, n = 0;
        float* data = stbi_loadf(path.c_str(), &w, &h, &n, 0);
        if (!data) return false;

        out.width = w;
        out.height = h;
        out.channels = n;
        out.pixels.assign(data, data + (static_cast<size_t>(w) * h * n));
        stbi_image_free(data);
        return true;
    }

    // Fallback: attempt float load via stb (for other formats, rarely used)
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    int w = 0, h = 0, n = 0;
    float* data = stbi_loadf(path.c_str(), &w, &h, &n, 0);
    if (!data) return false;
    out.width = w;
    out.height = h;
    out.channels = n;
    out.pixels.assign(data, data + (static_cast<size_t>(w) * h * n));
    stbi_image_free(data);
    return true;
}

bool LoadImage8(const std::string& path, ImageData8& out, bool flipY, int desiredChannels) {
    out = {};
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    int w = 0, h = 0, n = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, desiredChannels);
    if (!data) return false;
    out.width = w;
    out.height = h;
    out.channels = desiredChannels > 0 ? desiredChannels : n;
    out.pixels.assign(data, data + (static_cast<size_t>(w) * h * out.channels));
    stbi_image_free(data);
    return true;
}

} // namespace ImageIO
