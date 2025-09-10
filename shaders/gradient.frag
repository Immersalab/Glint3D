#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform vec3 topColor;
uniform vec3 bottomColor;

void main()
{
    // Interpolate between bottom and top color based on Y coordinate
    // TexCoords.y = 0 is bottom, TexCoords.y = 1 is top
    vec3 color = mix(bottomColor, topColor, TexCoords.y);
    FragColor = vec4(color, 1.0);
}