#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec2 vUV;
in mat3 vTBN;

// Light types
#define LIGHT_POINT 0
#define LIGHT_DIRECTIONAL 1

// Lights
struct Light { 
    int type;
    vec3 position; 
    vec3 direction; 
    vec3 color; 
    float intensity; 
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
    float shadow = calculateShadow(lightSpaceMatrix * vec4(vWorldPos, 1.0));
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
    // gamma correction
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color, baseColorFactor.a);
}

