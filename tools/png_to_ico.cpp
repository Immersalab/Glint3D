#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "../engine/Libraries/include/stb/stb_image.h"

// Simple ICO file format structures
#pragma pack(push, 1)
struct ICONDIR {
    uint16_t idReserved;
    uint16_t idType;
    uint16_t idCount;
};

struct ICONDIRENTRY {
    uint8_t bWidth;
    uint8_t bHeight;
    uint8_t bColorCount;
    uint8_t bReserved;
    uint16_t wPlanes;
    uint16_t wBitCount;
    uint32_t dwBytesInRes;
    uint32_t dwImageOffset;
};

struct BITMAPINFOHEADER {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

bool convertPngToIco(const char* pngPath, const char* icoPath) {
    int width, height, channels;
    unsigned char* data = stbi_load(pngPath, &width, &height, &channels, 4); // Force RGBA
    
    if (!data) {
        std::cerr << "Failed to load PNG: " << pngPath << std::endl;
        return false;
    }
    
    // Only support square icons up to 256x256
    if (width != height || width > 256) {
        std::cerr << "Icon must be square and <= 256x256" << std::endl;
        stbi_image_free(data);
        return false;
    }
    
    std::ofstream ico(icoPath, std::ios::binary);
    if (!ico) {
        std::cerr << "Failed to create ICO file: " << icoPath << std::endl;
        stbi_image_free(data);
        return false;
    }
    
    // Calculate sizes
    uint32_t bitmapSize = sizeof(BITMAPINFOHEADER) + (width * height * 4) + (width * height / 8);
    uint32_t totalSize = sizeof(ICONDIR) + sizeof(ICONDIRENTRY) + bitmapSize;
    
    // Write ICO header
    ICONDIR iconDir = {0};
    iconDir.idReserved = 0;
    iconDir.idType = 1; // ICO format
    iconDir.idCount = 1;
    ico.write(reinterpret_cast<const char*>(&iconDir), sizeof(iconDir));
    
    // Write directory entry
    ICONDIRENTRY iconEntry = {0};
    iconEntry.bWidth = (width == 256) ? 0 : width;
    iconEntry.bHeight = (height == 256) ? 0 : height;
    iconEntry.bColorCount = 0; // True color
    iconEntry.bReserved = 0;
    iconEntry.wPlanes = 1;
    iconEntry.wBitCount = 32;
    iconEntry.dwBytesInRes = bitmapSize;
    iconEntry.dwImageOffset = sizeof(ICONDIR) + sizeof(ICONDIRENTRY);
    ico.write(reinterpret_cast<const char*>(&iconEntry), sizeof(iconEntry));
    
    // Write bitmap info header
    BITMAPINFOHEADER bmpHeader = {0};
    bmpHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpHeader.biWidth = width;
    bmpHeader.biHeight = height * 2; // Icon height is doubled (color + mask)
    bmpHeader.biPlanes = 1;
    bmpHeader.biBitCount = 32;
    bmpHeader.biCompression = 0; // BI_RGB
    bmpHeader.biSizeImage = width * height * 4;
    ico.write(reinterpret_cast<const char*>(&bmpHeader), sizeof(bmpHeader));
    
    // Write pixel data (bottom-up, BGRA format)
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            unsigned char pixel[4] = {
                data[idx + 2], // B
                data[idx + 1], // G
                data[idx + 0], // R
                data[idx + 3]  // A
            };
            ico.write(reinterpret_cast<const char*>(pixel), 4);
        }
    }
    
    // Write AND mask (all zeros since we have alpha channel)
    int maskSize = width * height / 8;
    std::vector<unsigned char> mask(maskSize, 0);
    ico.write(reinterpret_cast<const char*>(mask.data()), maskSize);
    
    ico.close();
    stbi_image_free(data);
    
    std::cout << "Successfully converted " << pngPath << " to " << icoPath << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.png> <output.ico>" << std::endl;
        return 1;
    }
    
    return convertPngToIco(argv[1], argv[2]) ? 0 : 1;
}