#version 330 core
out vec4 FragColor;
in vec2 vUV;
uniform vec3 topColor;
uniform vec3 bottomColor;
void main()
{
    // vUV.y: 0 at bottom, 1 at top
    float t = clamp(vUV.y, 0.0, 1.0);
    vec3 col = mix(bottomColor, topColor, t);
    FragColor = vec4(col, 1.0);
}

