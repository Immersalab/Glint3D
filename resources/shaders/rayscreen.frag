#version 330 core
in  vec2 vUV;
out vec4 FragColor;
uniform sampler2D rayTex;
void main() { FragColor = vec4(texture(rayTex,vUV).rgb, 1.0); }
