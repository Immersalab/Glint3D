#include "Texture.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <fstream>
#include <filesystem>
#ifdef KTX2_ENABLED
#  include <ktx.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static std::string toLowerExt(const std::string& path)
{
    auto pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = path.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return ext;
}

Texture::Texture() : m_textureID(0) {}

Texture::~Texture()
{
    if (m_textureID)
        glDeleteTextures(1, &m_textureID);
}

bool Texture::loadFromFile(const std::string& filepath, bool flipY)
{
    // Prefer a sibling .ktx2 when present, or direct .ktx2 request
    std::error_code ec;
    std::filesystem::path inPath(filepath);
    const std::string ext = toLowerExt(filepath);
    const bool requestIsKTX2 = (ext == ".ktx2");
    std::filesystem::path ktx2Path = requestIsKTX2 ? inPath : inPath;
    if (!requestIsKTX2)
    {
        ktx2Path.replace_extension(".ktx2");
    }
    const bool ktx2Exists = std::filesystem::exists(ktx2Path, ec) && !ec;
    if (requestIsKTX2 || ktx2Exists)
    {
        if (loadFromKTX2(ktx2Path.string()))
        {
            return true;
        }
        // If KTX2 load failed (e.g., not enabled), fall through to STB
        std::cerr << "[Texture] KTX2 load failed or unsupported for '" << ktx2Path.string() << "'. Falling back to STB." << std::endl;
    }

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return false;
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // store dims for perf HUD
    m_width = width; m_height = height; m_channels = channels;

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

bool Texture::loadFromKTX2(const std::string& filepath)
{
#ifdef KTX2_ENABLED
    ktxTexture2* k2 = nullptr;
    KTX_error_code rc = ktxTexture2_CreateFromNamedFile(filepath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &k2);
    if (rc != KTX_SUCCESS || !k2)
    {
        std::cerr << "[Texture] ktxTexture2_CreateFromNamedFile failed for '" << filepath << "' rc=" << rc << std::endl;
        return false;
    }

    ktxTexture* kt = reinterpret_cast<ktxTexture*>(k2);
    GLuint tex = 0; GLenum target = 0; GLenum glerr = 0;
    rc = ktxTexture_GLUpload(kt, &tex, &target, &glerr);
    if (rc != KTX_SUCCESS)
    {
        std::cerr << "[Texture] ktxTexture_GLUpload failed rc=" << rc << " glerr=" << glerr << std::endl;
        ktxTexture_Destroy(kt);
        return false;
    }

    m_textureID = tex;
    // Set defaults (filtering/wrap) on bound texture target
    glBindTexture(target, m_textureID);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(target, 0);

    // Dimensions
    m_width  = static_cast<int>(ktxTexture_GetWidth(kt));
    m_height = static_cast<int>(ktxTexture_GetHeight(kt));
    // Channels: approximate; compressed formats vary. Use 4 as a reasonable default.
    m_channels = 4;

    ktxTexture_Destroy(kt);
    return true;
#else
    (void)filepath;
    return false;
#endif
}

void Texture::bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

