#ifndef TEXTURE_H
#define TEXTURE_H

#include "gl_platform.h"
#include <string>

class Texture
{
public:
    Texture();
    ~Texture();

    bool loadFromFile(const std::string& filepath, bool flipY = false);
    void bind(GLuint unit = 0) const;

    // Perf introspection
    int  width() const { return m_width; }
    int  height() const { return m_height; }
    int  channels() const { return m_channels; }

private:
    // Optional: KTX2/Basis loader when KTX2_ENABLED is defined
    bool loadFromKTX2(const std::string& filepath);

    GLuint m_textureID{0};
    int m_width{0}, m_height{0}, m_channels{0};
};

#endif // TEXTURE_H
