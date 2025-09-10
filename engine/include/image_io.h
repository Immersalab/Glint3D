#pragma once

#include <string>
#include <vector>

namespace ImageIO {

struct ImageDataFloat {
    int width = 0;
    int height = 0;
    int channels = 0; // 3 or 4 expected for HDR/EXR
    std::vector<float> pixels; // size = width * height * channels
};

struct ImageData8 {
    int width = 0;
    int height = 0;
    int channels = 0; // 1..4
    std::vector<unsigned char> pixels; // size = width * height * channels
};

// Loads HDR float images.
// Supports: .hdr via stb_image, .exr via TinyEXR.
// If flipY is true, vertically flips the image after load.
bool LoadImageFloat(const std::string& path, ImageDataFloat& out, bool flipY = false);

// Loads LDR 8-bit images via stb_image. Common formats: png, jpg, bmp, tga, etc.
// If flipY is true, vertically flips the image after load.
bool LoadImage8(const std::string& path, ImageData8& out, bool flipY = false, int desiredChannels = 0);

} // namespace ImageIO

