#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;
uniform bool hasTangents = false;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vWorldPos;
out vec2 vUV;
out mat3 vTBN;

void main() {
    vec3 N = normalize(mat3(transpose(inverse(model))) * aNormal);
    vec3 T;
    if (hasTangents) {
        T = normalize(mat3(model) * aTangent);
        // Orthonormalize T against N
        T = normalize(T - N * dot(N, T));
    } else {
        // Fallback: build arbitrary T perpendicular to N
        vec3 up = abs(N.y) < 0.999 ? vec3(0,1,0) : vec3(1,0,0);
        T = normalize(cross(up, N));
    }
    vec3 B = normalize(cross(N, T));

    vTBN = mat3(T, B, N);
    vUV = aUV;
    vWorldPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(vWorldPos, 1.0);
}
