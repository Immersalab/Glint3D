#include "ibl_system.h"
#include "shader.h"
#include "image_io.h"
#include <iostream>
#include <cmath>

namespace {
    // Cube vertices for environment mapping
    float cubeVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // Quad vertices for BRDF LUT generation
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
}

IBLSystem::IBLSystem()
    : m_environmentMap(0)
    , m_irradianceMap(0)
    , m_prefilterMap(0)
    , m_brdfLUT(0)
    , m_captureFramebuffer(0)
    , m_captureRenderbuffer(0)
    , m_equirectToCubemapShader(nullptr)
    , m_irradianceShader(nullptr)
    , m_prefilterShader(nullptr)
    , m_brdfShader(nullptr)
    , m_cubeVAO(0)
    , m_quadVAO(0)
    , m_intensity(1.0f)
    , m_initialized(false)
{
}

IBLSystem::~IBLSystem()
{
    cleanup();
}

bool IBLSystem::init()
{
    if (m_initialized) return true;

    // Setup framebuffer for capturing cubemap faces
    glGenFramebuffers(1, &m_captureFramebuffer);
    glGenRenderbuffers(1, &m_captureRenderbuffer);
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_captureRenderbuffer);

    // Create shaders
    createShaders();
    
    // Setup geometry
    setupCube();
    setupQuad();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    m_initialized = true;
    return true;
}

void IBLSystem::createShaders()
{
    // Equirectangular to cubemap shader
    m_equirectToCubemapShader = new Shader();
    m_equirectToCubemapShader->loadFromStrings(
        // Vertex shader
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 WorldPos;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    WorldPos = aPos;\n"  
        "    gl_Position = projection * view * vec4(WorldPos, 1.0);\n"
        "}\n",
        
        // Fragment shader
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 WorldPos;\n"
        "uniform sampler2D equirectangularMap;\n"
        "const vec2 invAtan = vec2(0.1591, 0.3183);\n"
        "vec2 SampleSphericalMap(vec3 v) {\n"
        "    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));\n"
        "    uv *= invAtan;\n"
        "    uv += 0.5;\n"
        "    return uv;\n"
        "}\n"
        "void main() {\n"		
        "    vec2 uv = SampleSphericalMap(normalize(WorldPos));\n"
        "    vec3 color = texture(equirectangularMap, uv).rgb;\n"
        "    FragColor = vec4(color, 1.0);\n"
        "}\n"
    );

    // Irradiance convolution shader
    m_irradianceShader = new Shader();
    m_irradianceShader->loadFromStrings(
        // Vertex shader
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 WorldPos;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    WorldPos = aPos;\n"
        "    gl_Position = projection * view * vec4(WorldPos, 1.0);\n"
        "}\n",
        
        // Fragment shader
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 WorldPos;\n"
        "uniform samplerCube environmentMap;\n"
        "const float PI = 3.14159265359;\n"
        "void main() {\n"		
        "    vec3 N = normalize(WorldPos);\n"
        "    vec3 irradiance = vec3(0.0);\n"
        "    vec3 up    = vec3(0.0, 1.0, 0.0);\n"
        "    vec3 right = normalize(cross(up, N));\n"
        "    up         = normalize(cross(N, right));\n"
        "    float sampleDelta = 0.025;\n"
        "    float nrSamples = 0.0;\n"
        "    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {\n"
        "        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {\n"
        "            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));\n"
        "            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;\n" 
        "            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);\n"
        "            nrSamples++;\n"
        "        }\n"
        "    }\n"
        "    irradiance = PI * irradiance * (1.0 / float(nrSamples));\n"
        "    FragColor = vec4(irradiance, 1.0);\n"
        "}\n"
    );

    // Prefilter shader
    m_prefilterShader = new Shader();
    m_prefilterShader->loadFromStrings(
        // Vertex shader
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 WorldPos;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    WorldPos = aPos;\n"
        "    gl_Position = projection * view * vec4(WorldPos, 1.0);\n"
        "}\n",
        
        // Fragment shader
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 WorldPos;\n"
        "uniform samplerCube environmentMap;\n"
        "uniform float roughness;\n"
        "const float PI = 3.14159265359;\n"
        "float DistributionGGX(vec3 N, vec3 H, float roughness) {\n"
        "    float a = roughness*roughness;\n"
        "    float a2 = a*a;\n"
        "    float NdotH = max(dot(N, H), 0.0);\n"
        "    float NdotH2 = NdotH*NdotH;\n"
        "    float nom   = a2;\n"
        "    float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n"
        "    denom = PI * denom * denom;\n"
        "    return nom / denom;\n"
        "}\n"
        "float RadicalInverse_VdC(uint bits) {\n"
        "    bits = (bits << 16u) | (bits >> 16u);\n"
        "    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
        "    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
        "    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
        "    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
        "    return float(bits) * 2.3283064365386963e-10;\n"
        "}\n"
        "vec2 Hammersley(uint i, uint N) {\n"
        "    return vec2(float(i)/float(N), RadicalInverse_VdC(i));\n"
        "}\n"
        "vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {\n"
        "    float a = roughness*roughness;\n"
        "    float phi = 2.0 * PI * Xi.x;\n"
        "    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));\n"
        "    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);\n"
        "    vec3 H;\n"
        "    H.x = cos(phi) * sinTheta;\n"
        "    H.y = sin(phi) * sinTheta;\n"
        "    H.z = cosTheta;\n"
        "    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
        "    vec3 tangent   = normalize(cross(up, N));\n"
        "    vec3 bitangent = cross(N, tangent);\n"
        "    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
        "    return normalize(sampleVec);\n"
        "}\n"
        "void main() {\n"		
        "    vec3 N = normalize(WorldPos);\n"
        "    vec3 R = N;\n"
        "    vec3 V = R;\n"
        "    const uint SAMPLE_COUNT = 1024u;\n"
        "    vec3 prefilteredColor = vec3(0.0);\n"
        "    float totalWeight = 0.0;\n"
        "    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {\n"
        "        vec2 Xi = Hammersley(i, SAMPLE_COUNT);\n"
        "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
        "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
        "        float NdotL = max(dot(N, L), 0.0);\n"
        "        if(NdotL > 0.0) {\n"
        "            float D   = DistributionGGX(N, H, roughness);\n"
        "            float NdotH = max(dot(N, H), 0.0);\n"
        "            float HdotV = max(dot(H, V), 0.0);\n"
        "            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;\n"
        "            float resolution = 512.0;\n"
        "            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);\n"
        "            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);\n"
        "            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);\n"
        "            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;\n"
        "            totalWeight      += NdotL;\n"
        "        }\n"
        "    }\n"
        "    prefilteredColor = prefilteredColor / totalWeight;\n"
        "    FragColor = vec4(prefilteredColor, 1.0);\n"
        "}\n"
    );

    // BRDF LUT shader
    m_brdfShader = new Shader();
    m_brdfShader->loadFromStrings(
        // Vertex shader
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoords;\n"
        "out vec2 TexCoords;\n"
        "void main() {\n"
        "    TexCoords = aTexCoords;\n"
        "    gl_Position = vec4(aPos, 1.0);\n"
        "}\n",
        
        // Fragment shader
        "#version 330 core\n"
        "out vec2 FragColor;\n"
        "in vec2 TexCoords;\n"
        "const float PI = 3.14159265359;\n"
        "float RadicalInverse_VdC(uint bits) {\n"
        "    bits = (bits << 16u) | (bits >> 16u);\n"
        "    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
        "    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
        "    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
        "    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
        "    return float(bits) * 2.3283064365386963e-10;\n"
        "}\n"
        "vec2 Hammersley(uint i, uint N) {\n"
        "    return vec2(float(i)/float(N), RadicalInverse_VdC(i));\n"
        "}\n"
        "vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {\n"
        "    float a = roughness*roughness;\n"
        "    float phi = 2.0 * PI * Xi.x;\n"
        "    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));\n"
        "    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);\n"
        "    vec3 H;\n"
        "    H.x = cos(phi) * sinTheta;\n"
        "    H.y = sin(phi) * sinTheta;\n"
        "    H.z = cosTheta;\n"
        "    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n"
        "    vec3 tangent   = normalize(cross(up, N));\n"
        "    vec3 bitangent = cross(N, tangent);\n"
        "    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;\n"
        "    return normalize(sampleVec);\n"
        "}\n"
        "float GeometrySchlickGGX(float NdotV, float roughness) {\n"
        "    float a = roughness;\n"
        "    float k = (a * a) / 2.0;\n"
        "    float nom   = NdotV;\n"
        "    float denom = NdotV * (1.0 - k) + k;\n"
        "    return nom / denom;\n"
        "}\n"
        "float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {\n"
        "    float NdotV = max(dot(N, V), 0.0);\n"
        "    float NdotL = max(dot(N, L), 0.0);\n"
        "    float ggx1 = GeometrySchlickGGX(NdotV, roughness);\n"
        "    float ggx2 = GeometrySchlickGGX(NdotL, roughness);\n"
        "    return ggx1 * ggx2;\n"
        "}\n"
        "vec2 IntegrateBRDF(float NdotV, float roughness) {\n"
        "    vec3 V;\n"
        "    V.x = sqrt(1.0 - NdotV*NdotV);\n"
        "    V.y = 0.0;\n"
        "    V.z = NdotV;\n"
        "    float A = 0.0;\n"
        "    float B = 0.0;\n"
        "    vec3 N = vec3(0.0, 0.0, 1.0);\n"
        "    const uint SAMPLE_COUNT = 1024u;\n"
        "    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {\n"
        "        vec2 Xi = Hammersley(i, SAMPLE_COUNT);\n"
        "        vec3 H = ImportanceSampleGGX(Xi, N, roughness);\n"
        "        vec3 L = normalize(2.0 * dot(V, H) * H - V);\n"
        "        float NdotL = max(L.z, 0.0);\n"
        "        float NdotH = max(H.z, 0.0);\n"
        "        float VdotH = max(dot(V, H), 0.0);\n"
        "        if(NdotL > 0.0) {\n"
        "            float G = GeometrySmith(N, V, L, roughness);\n"
        "            float G_Vis = (G * VdotH) / (NdotH * NdotV);\n"
        "            float Fc = pow(1.0 - VdotH, 5.0);\n"
        "            A += (1.0 - Fc) * G_Vis;\n"
        "            B += Fc * G_Vis;\n"
        "        }\n"
        "    }\n"
        "    A /= float(SAMPLE_COUNT);\n"
        "    B /= float(SAMPLE_COUNT);\n"
        "    return vec2(A, B);\n"
        "}\n"
        "void main() {\n"
        "    vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);\n"
        "    FragColor = integratedBRDF;\n"
        "}\n"
    );
}

GLuint IBLSystem::loadHDRTexture(const std::string& path)
{
    ImageIO::ImageDataFloat img;
    if (!ImageIO::LoadImageFloat(path, img, /*flipY=*/true)) {
        std::cerr << "Failed to load HDR/EXR image: " << path << std::endl;
        return 0;
    }

    GLuint hdrTexture;
    glGenTextures(1, &hdrTexture);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    if (img.channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, img.width, img.height, 0, GL_RGB, GL_FLOAT, img.pixels.data());
    } else {
        // Default to RGBA for 4 or other channel counts (TinyEXR returns 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, img.width, img.height, 0, GL_RGBA, GL_FLOAT, img.pixels.data());
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return hdrTexture;
}

bool IBLSystem::loadHDREnvironment(const std::string& hdrPath)
{
    if (!m_initialized) {
        std::cerr << "IBL System not initialized" << std::endl;
        return false;
    }

    // Save current viewport
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Load HDR equirectangular map
    GLuint hdrTexture = loadHDRTexture(hdrPath);
    if (hdrTexture == 0) {
        // Restore viewport before returning
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
        return false;
    }

    // Create environment cubemap
    glGenTextures(1, &m_environmentMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_environmentMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Convert HDR equirectangular map to cubemap
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    m_equirectToCubemapShader->use();
    m_equirectToCubemapShader->setInt("equirectangularMap", 0);
    m_equirectToCubemapShader->setMat4("projection", captureProjection);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFramebuffer);
    
    for (unsigned int i = 0; i < 6; ++i) {
        m_equirectToCubemapShader->setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_environmentMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Generate mipmaps for the environment map
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_environmentMap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Cleanup HDR texture
    glDeleteTextures(1, &hdrTexture);
    
    // Restore original viewport
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    
    return true;
}

void IBLSystem::generateIrradianceMap()
{
    // Save current viewport
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    
    glGenTextures(1, &m_irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_irradianceMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    m_irradianceShader->use();
    m_irradianceShader->setInt("environmentMap", 0);
    m_irradianceShader->setMat4("projection", glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_environmentMap);

    glViewport(0, 0, 32, 32);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };
    
    for (unsigned int i = 0; i < 6; ++i) {
        m_irradianceShader->setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Restore original viewport
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

void IBLSystem::generatePrefilterMap()
{
    // Save current viewport
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    
    glGenTextures(1, &m_prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_prefilterMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    m_prefilterShader->use();
    m_prefilterShader->setInt("environmentMap", 0);
    m_prefilterShader->setMat4("projection", glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_environmentMap);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFramebuffer);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, m_captureRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        m_prefilterShader->setFloat("roughness", roughness);
        
        glm::mat4 captureViews[] = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };
        
        for (unsigned int i = 0; i < 6; ++i) {
            m_prefilterShader->setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Restore original viewport
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

void IBLSystem::generateBRDFLUT()
{
    // Save current viewport
    GLint prevViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    
    glGenTextures(1, &m_brdfLUT);
    glBindTexture(GL_TEXTURE_2D, m_brdfLUT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_brdfLUT, 0);

    glViewport(0, 0, 512, 512);
    m_brdfShader->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Restore original viewport
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

void IBLSystem::bindIBLTextures() const
{
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_irradianceMap);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_prefilterMap);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, m_brdfLUT);
}

void IBLSystem::setupCube()
{
    GLuint cubeVBO;
    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &cubeVBO);
    
    glBindVertexArray(m_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void IBLSystem::setupQuad()
{
    GLuint quadVBO;
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    
    glBindVertexArray(0);
}

void IBLSystem::renderCube()
{
    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void IBLSystem::renderQuad()
{
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void IBLSystem::cleanup()
{
    if (m_environmentMap != 0) {
        glDeleteTextures(1, &m_environmentMap);
        m_environmentMap = 0;
    }
    if (m_irradianceMap != 0) {
        glDeleteTextures(1, &m_irradianceMap);
        m_irradianceMap = 0;
    }
    if (m_prefilterMap != 0) {
        glDeleteTextures(1, &m_prefilterMap);
        m_prefilterMap = 0;
    }
    if (m_brdfLUT != 0) {
        glDeleteTextures(1, &m_brdfLUT);
        m_brdfLUT = 0;
    }
    if (m_captureFramebuffer != 0) {
        glDeleteFramebuffers(1, &m_captureFramebuffer);
        m_captureFramebuffer = 0;
    }
    if (m_captureRenderbuffer != 0) {
        glDeleteRenderbuffers(1, &m_captureRenderbuffer);
        m_captureRenderbuffer = 0;
    }
    if (m_cubeVAO != 0) {
        glDeleteVertexArrays(1, &m_cubeVAO);
        m_cubeVAO = 0;
    }
    if (m_quadVAO != 0) {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    
    delete m_equirectToCubemapShader;
    delete m_irradianceShader;
    delete m_prefilterShader;
    delete m_brdfShader;
    
    m_equirectToCubemapShader = nullptr;
    m_irradianceShader = nullptr;
    m_prefilterShader = nullptr;
    m_brdfShader = nullptr;
    
    m_initialized = false;
}
