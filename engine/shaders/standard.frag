#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 GouraudLight;  // We only use this if shadingMode == 1
in vec2 UV;

uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// For texturing & shading controls
uniform sampler2D cowTexture;
uniform bool useTexture;
uniform int shadingMode;
uniform vec3 objectColor; // fallback if no texture

// ------------------------------------------------------------------------
// Global ambient
uniform vec4 globalAmbient; // .a is often unused, so we just use .rgb

// Light types
#define LIGHT_POINT 0
#define LIGHT_DIRECTIONAL 1
#define LIGHT_SPOT 2

// Light struct
struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity; // If 0, treat as disabled
    float innerCutoff; // cos(inner)
    float outerCutoff; // cos(outer)
};

#define MAX_LIGHTS 10
uniform int numLights;
uniform Light lights[MAX_LIGHTS];

// ------------------------------------------------------------------------
// Material struct
struct Material {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    float shininess;
    float roughness;
    float metallic;
};
uniform Material material;

// Camera position (for specular reflection in Gouraud)
uniform vec3 viewPos;

// ------------------------------------------------------------------------
// Calculate Shadow
float calculateShadow(vec4 fragPosLightSpace)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // transform to [0,1] range

    // Check if outside light projection
    if (projCoords.z > 1.0)
        return 1.0;

    // Read depth from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Bias to prevent shadow acne
    float bias = 0.005;

    // 1.0 = lit, 0.5 = shadowed
    float shadow = (currentDepth - bias) > closestDepth ? 0.5 : 1.0;
    return shadow;
}

// ------------------------------------------------------------------------
// Main
void main()
{
    // Base color from either texture or fallback color
    vec3 baseColor = useTexture ? texture(cowTexture, UV).rgb : objectColor;

    // Shadow factor
    float shadow = calculateShadow(lightSpaceMatrix * vec4(FragPos, 1.0));

    // Start with ambient term
    vec3 totalLight = globalAmbient.rgb * material.ambient;

    if (shadingMode == 0) {
        // ----- FLAT Shading -----
        vec3 faceNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));

        for (int i = 0; i < numLights; i++) {
            if (lights[i].intensity <= 0.0) continue;
            
            vec3 L;
            if (lights[i].type == LIGHT_POINT) {
                L = normalize(lights[i].position - FragPos);
            } else if (lights[i].type == LIGHT_DIRECTIONAL) {
                L = normalize(-lights[i].direction);
            } else if (lights[i].type == LIGHT_SPOT) {
                L = normalize(lights[i].position - FragPos);
            } else {
                continue;
            }
            
            float diff = max(dot(faceNormal, L), 0.0);

            float atten = 1.0;
            float spotFactor = 1.0;
            if (lights[i].type == LIGHT_POINT) {
                float dist = length(lights[i].position - FragPos);
                atten = 1.0 / max(dist*dist, 1e-4);
            } else if (lights[i].type == LIGHT_SPOT) {
                float dist = length(lights[i].position - FragPos);
                atten = 1.0 / max(dist*dist, 1e-4);
                float theta = dot(normalize(-lights[i].direction), L);
                float inner = lights[i].innerCutoff;
                float outer = lights[i].outerCutoff;
                float t = clamp((theta - outer) / max(inner - outer, 1e-4), 0.0, 1.0);
                spotFactor = t;
            }
            vec3 radiance = lights[i].color * lights[i].intensity * atten * spotFactor;
            totalLight += shadow * material.diffuse * diff * radiance;
        }
    }
    else if (shadingMode == 1) {
        // ----- GOURAUD Shading -----
        totalLight += shadow * GouraudLight;
    }

    // Final color
    vec3 finalColor = baseColor * totalLight;
    FragColor = vec4(finalColor, 1.0);
}
