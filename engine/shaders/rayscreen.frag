#version 330 core
in  vec2 vUV;
out vec4 FragColor;
uniform sampler2D rayTex;

// Post-processing uniforms
uniform float exposure;
uniform float gamma;
uniform int toneMappingMode; // 0=Linear, 1=Reinhard, 2=Filmic, 3=ACES

// Tone mapping functions
vec3 reinhardToneMapping(vec3 color) {
    return color / (1.0 + color);
}

vec3 filmicToneMapping(vec3 color) {
    vec3 x = max(vec3(0.0), color - 0.004);
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

vec3 acesToneMapping(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 applyToneMapping(vec3 color) {
    // Apply exposure first
    color *= pow(2.0, exposure);
    
    // Apply tone mapping
    if (toneMappingMode == 1) {
        color = reinhardToneMapping(color);
    } else if (toneMappingMode == 2) {
        color = filmicToneMapping(color);
    } else if (toneMappingMode == 3) {
        color = acesToneMapping(color);
    }
    // Mode 0 (Linear) doesn't apply tone mapping
    
    return color;
}

void main() { 
    vec3 color = texture(rayTex, vUV).rgb;
    
    // Apply tone mapping and exposure
    color = applyToneMapping(color);
    
    // Apply gamma correction
    color = pow(color, vec3(1.0/gamma));
    
    FragColor = vec4(color, 1.0); 
}
