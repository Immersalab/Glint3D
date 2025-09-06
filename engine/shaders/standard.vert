#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

// Matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Shading mode: 0=Flat, 1=Gouraud
uniform int shadingMode;

// Outputs to the fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec3 GouraudLight;
out vec2 UV;

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

// Material for Gouraud specular
struct Material {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    float shininess;
    float roughness;
    float metallic;
};

uniform Material material;
uniform vec3 viewPos; // Camera position in world space

void main()
{
    // Compute position in world space
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // Normal transformation via normal matrix
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Simple placeholder UV mapping
    UV = aPos.xy * 0.5 + 0.5;

    // Default to black unless we're in Gouraud shading mode
    GouraudLight = vec3(0.0);

    // Only compute Gouraud if shadingMode == 1
    if (shadingMode == 1) {
        vec3 normal = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);

        for (int i = 0; i < numLights; i++) {
            if (lights[i].intensity <= 0.0) continue;
            
            vec3 lightDir;
            if (lights[i].type == LIGHT_POINT) {
                lightDir = normalize(lights[i].position - FragPos);
            } else if (lights[i].type == LIGHT_DIRECTIONAL) {
                lightDir = normalize(-lights[i].direction);
            } else if (lights[i].type == LIGHT_SPOT) {
                // Spot: direction points out from light; compare with vector to fragment
                lightDir = normalize(lights[i].position - FragPos);
            } else {
                continue;
            }

            float spotFactor = 1.0;
            float atten = 1.0;
            if (lights[i].type == LIGHT_POINT) {
                float dist = length(lights[i].position - FragPos);
                atten = 1.0 / max(dist*dist, 1e-4);
            } else if (lights[i].type == LIGHT_SPOT) {
                // Attenuation + cone falloff
                float dist = length(lights[i].position - FragPos);
                atten = 1.0 / max(dist*dist, 1e-4);
                float theta = dot(normalize(-lights[i].direction), lightDir);
                float inner = lights[i].innerCutoff;
                float outer = lights[i].outerCutoff;
                float t = clamp((theta - outer) / max(inner - outer, 1e-4), 0.0, 1.0);
                spotFactor = t;
            } else {
                continue;
            }

            // Diffuse
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 radiance = lights[i].color * lights[i].intensity * atten * spotFactor;
            vec3 diffuse = material.diffuse * diff * radiance;

            // Specular (Phong reflection model but computed at vertex)
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
            vec3 specular = material.specular * spec * radiance;

            GouraudLight += diffuse + specular;
        }
    }

    // Final position for rasterization
    gl_Position = projection * view * worldPos;
}
