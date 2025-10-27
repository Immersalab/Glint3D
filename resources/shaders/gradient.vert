#version 330 core
layout (location = 0) in vec3 aPos;

out vec2 TexCoords;

void main()
{
    // Full screen quad vertex positions
    gl_Position = vec4(aPos, 1.0);
    // Convert from [-1, 1] to [0, 1] for texture coordinates
    TexCoords = (aPos.xy + 1.0) * 0.5;
}