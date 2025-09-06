#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec2 vUV;
in mat3 vTBN;

// Light types
#define LIGHT_POINT 0
#define LIGHT_DIRECTIONAL 1
#define LIGHT_SPOT 2

// Lights
struct Light { 
    int type;
    vec3 position; 
    vec3 direction; 
    vec3 color; 
    float intensity; 
    float innerCutoff; // cos(inner)
    float outerCutoff; // cos(outer)
};
#define MAX_LIGHTS 10
uniform int numLights;
uniform Light lights[MAX_LIGHTS];
uniform vec3 viewPos;

// PBR inputs
uniform vec4 baseColorFactor; // rgba
uniform float metallicFactor;
uniform float roughnessFactor;
uniform bool  hasBaseColorMap;
uniform bool  hasNormalMap;
uniform bool  hasMRMap;
uniform sampler2D baseColorTex;
uniform sampler2D normalTex;
uniform sampler2D mrTex; // glTF convention: G=roughness, B=metallic

uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// Post-processing uniforms
uniform float exposure;
uniform float gamma;
uniform int toneMappingMode; // 0=Linear, 1=Reinhard, 2=Filmic, 3=ACES

// Shadow
float calculateShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 1.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.005;
    return (currentDepth - bias) > closestDepth ? 0.5 : 1.0;
}

// Helpers
const float PI = 3.14159265359;
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float DistributionGGX(vec3 N, vec3 H, float rough) {
    float a = rough*rough; float a2 = a*a;
    float NdotH = max(dot(N,H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 1e-4);
}
float GeometrySchlickGGX(float NdotV, float rough) {
    float r = rough + 1.0;
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, rough);
    float ggx2 = GeometrySchlickGGX(NdotL, rough);
    return ggx1 * ggx2;
}

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
    // Apply exposure first - use exp2 for better performance
    color *= exp2(exposure);
    
    // Optimized tone mapping with fewer branches
    vec3 reinhard = color / (1.0 + color);
    vec3 x = max(vec3(0.0), color - 0.004);
    vec3 filmic = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    vec3 aces = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
    
    // Use step functions instead of branches for better GPU performance
    float useReinhard = step(0.5, float(toneMappingMode == 1));
    float useFilmic = step(0.5, float(toneMappingMode == 2));
    float useAces = step(0.5, float(toneMappingMode == 3));
    
    color = mix(color, reinhard, useReinhard);
    color = mix(color, filmic, useFilmic);
    color = mix(color, aces, useAces);
    
    return color;
}

void main() {
    // Sample inputs
    vec3 albedo = baseColorFactor.rgb;
    if (hasBaseColorMap) albedo = pow(texture(baseColorTex, vUV).rgb, vec3(2.2)); // assume sRGB
    float metallic = metallicFactor;
    float roughness = clamp(roughnessFactor, 0.04, 1.0);
    if (hasMRMap) {
        vec3 mrs = texture(mrTex, vUV).rgb; // R=occlusion (optional), G=roughness, B=metallic (glTF)
        roughness = clamp(mrs.g, 0.04, 1.0);
        metallic = mrs.b;
    }

    // Normal mapping
    vec3 N = normalize(vTBN[2]);
    if (hasNormalMap) {
        vec3 n = texture(normalTex, vUV).xyz * 2.0 - 1.0;
        N = normalize(vTBN * n);
    }

    vec3 V = normalize(viewPos - vWorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    // Disable shadowing until we implement a proper shadow map path
    float shadow = 1.0;
    for (int i=0;i<numLights;i++) {
        if (lights[i].intensity <= 0.0) continue;
        
        vec3 L;
        vec3 radiance;
        
        if (lights[i].type == LIGHT_POINT) {
            // Point light calculation
            L = normalize(lights[i].position - vWorldPos);
            float dist = length(lights[i].position - vWorldPos);
            float atten = 1.0 / (dist*dist);
            radiance = lights[i].color * lights[i].intensity * atten;
        } else if (lights[i].type == LIGHT_DIRECTIONAL) {
            // Directional light calculation
            L = normalize(-lights[i].direction);  // Light direction points towards the surface
            radiance = lights[i].color * lights[i].intensity;  // No attenuation for directional lights
        } else if (lights[i].type == LIGHT_SPOT) {
            // Spot light calculation
            L = normalize(lights[i].position - vWorldPos);
            float dist = length(lights[i].position - vWorldPos);
            float atten = 1.0 / (dist*dist);
            float theta = dot(normalize(-lights[i].direction), L);
            float inner = lights[i].innerCutoff;
            float outer = lights[i].outerCutoff;
            float t = clamp((theta - outer) / max(inner - outer, 1e-4), 0.0, 1.0);
            radiance = lights[i].color * lights[i].intensity * atten * t;
        } else {
            continue; // Skip unknown light types
        }
        
        vec3 H = normalize(V + L);

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 nominator = NDF * G * F;
        float denom = 4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 1e-4;
        vec3 specular = nominator / denom;

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float NdotL = max(dot(N,L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadow;
    }

    // No IBL; simple ambient term
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;
    
    // Apply tone mapping and exposure
    color = applyToneMapping(color);
    
    // Apply gamma correction
    color = pow(color, vec3(1.0/gamma));
    FragColor = vec4(color, baseColorFactor.a);
}
