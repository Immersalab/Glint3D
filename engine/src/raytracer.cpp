#include "raytracer.h"
#include <cmath>
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>
#include "brdf.h"

Raytracer::Raytracer()
    : lightPos(glm::vec3(-2.0f, 4.0f, -3.0f)),
    lightColor(glm::vec3(1.0f, 1.0f, 1.0f)),
    bvhRoot(nullptr)
{}

// Build BVH
static BVHNode* buildBVH(std::vector<const Triangle*>& tris, int depth = 0)
{
    if (tris.empty())
        return nullptr;

    BVHNode* node = new BVHNode();

    glm::vec3 minBox(FLT_MAX), maxBox(-FLT_MAX);
    for (auto tri : tris)
    {
        for (const glm::vec3& v : { tri->v0, tri->v1, tri->v2 })
        {
            minBox = glm::min(minBox, v);
            maxBox = glm::max(maxBox, v);
        }
    }
    node->boundsMin = minBox;
    node->boundsMax = maxBox;

    if (tris.size() <= 4)
    {
        node->triangles = tris;
        return node;
    }

    int axis = depth % 3;
    std::sort(tris.begin(), tris.end(), [axis](const Triangle* a, const Triangle* b)
        {
            float centerA = (a->v0[axis] + a->v1[axis] + a->v2[axis]) / 3.0f;
            float centerB = (b->v0[axis] + b->v1[axis] + b->v2[axis]) / 3.0f;
            return centerA < centerB;
        });

    size_t mid = tris.size() / 2;
    std::vector<const Triangle*> leftTris(tris.begin(), tris.begin() + mid);
    std::vector<const Triangle*> rightTris(tris.begin() + mid, tris.end());

    node->left = buildBVH(leftTris, depth + 1);
    node->right = buildBVH(rightTris, depth + 1);

    return node;
}

// --- Simplified Ray Tracer ---
glm::vec3 Raytracer::traceRay(const Ray& ray, const Light& lights, int depth) const
{
    if (depth > 2)
        return glm::vec3(0.0f);

    float tMin = FLT_MAX;
    glm::vec3 hitNormal;
    const Triangle* hitObject = nullptr;

    // BVH or brute-force
    if (bvhRoot)
        bvhRoot->intersect(ray, hitObject, tMin, hitNormal);
    else
    {
        for (const auto& tri : triangles)
        {
            float t;
            glm::vec3 normal;
            if (tri.intersect(ray, t, normal) && t < tMin)
            {
                tMin = t;
                hitNormal = normal;
                hitObject = &tri;
            }
        }
    }

    if (!hitObject)
        return glm::vec3(0.05f); // slightly dark background

    glm::vec3 hitPoint = ray.origin + tMin * ray.direction;
    glm::vec3 viewDir = glm::normalize(-ray.direction);
    glm::vec3 normal = glm::normalize(hitNormal);

    const Material& mat = hitObject->material;

    glm::vec3 color(0.0f);

    // Global ambient
    color += mat.ambient * glm::vec3(lights.m_globalAmbient);

    // For each light
    for (const auto& light : lights.m_lights)
    {
        if (!light.enabled)
            continue;

        glm::vec3 lightDir = glm::normalize(light.position - hitPoint);
        float lightDist = glm::length(light.position - hitPoint);

        // Shadow ray
        Ray shadowRay(hitPoint + normal * 0.001f, lightDir);
        bool inShadow = false;
        float shadowT;

        if (bvhRoot)
        {
            const Triangle* blocker = nullptr;
            if (bvhRoot->intersectAny(shadowRay, blocker, shadowT) && shadowT < lightDist)
                inShadow = true;
        }
        else
        {
            for (const auto& tri : triangles)
            {
                float t;
                glm::vec3 dummyNormal;
                if (tri.intersect(shadowRay, t, dummyNormal) && t < lightDist)
                {
                    inShadow = true;
                    break;
                }
            }
        }

        if (!inShadow)
        {
            // Cook–Torrance BRDF (Beckmann D, Cook–Torrance G, Schlick F)
            // Use material diffuse as baseColor/albedo; metallic drives specular vs diffuse split
            glm::vec3 brdf = brdf::cookTorrance(normal, viewDir, lightDir, mat.diffuse, mat.roughness, mat.metallic);
            float NdotL = glm::max(glm::dot(normal, lightDir), 0.0f);

            // Scale by light radiance (color * intensity) and Lambert cosine term
            color += brdf * (NdotL * light.intensity) * light.color;
        }
    }

    // Reflective contribution
    if (hitObject->reflectivity > 0.0f)
    {
        glm::vec3 reflectedDir = glm::reflect(ray.direction, normal);
        Ray reflectedRay(hitPoint + normal * 0.001f, glm::normalize(reflectedDir));
        glm::vec3 reflectedColor = traceRay(reflectedRay, lights, depth + 1);

        color = glm::mix(color, reflectedColor, hitObject->reflectivity);
    }

    // Leave color in linear space; screen shader applies tone mapping and gamma
    return glm::clamp(color, 0.0f, 1.0f);
}

void Raytracer::renderImage(std::vector<glm::vec3>& out,
    int W, int H,
    glm::vec3 camPos,
    glm::vec3 camFront,
    glm::vec3 camUp,
    float fovDeg,
    const Light& lights)
{
    const float aspect = float(W) / float(H);
    const float scale = tan(glm::radians(fovDeg * 0.5f));

    glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    glm::vec3 up = glm::normalize(glm::cross(right, camFront));

    glm::vec3 imageCenter = camFront;
    glm::vec3 imageRight = right * aspect * scale;
    glm::vec3 imageUp = up * scale;

    // Use proper OpenMP parallelization with atomic progress reporting
#pragma omp parallel for schedule(dynamic, 8)
    for (int y = 0; y < H; ++y)
    {
        // Thread-safe progress reporting
        if (y % 50 == 0) {
            #pragma omp critical
            {
                std::cout << "[DEBUG] Tracing row " << y << " of " << H << "\n";
            }
        }

        for (int x = 0; x < W; ++x)
        {
            float u = (x + 0.5f) / W * 2.0f - 1.0f;
            float v = 1.0f - (y + 0.5f) / H * 2.0f;

            glm::vec3 dir = glm::normalize(imageCenter + u * imageRight + v * imageUp);
            Ray r(camPos, dir);
            
            // Calculate output index correctly for flipped image
            int outputIndex = (H - 1 - y) * W + x;
            out[outputIndex] = traceRay(r, lights, 0);
        }
    }

    std::cout << "[DEBUG] renderImage() finished!\n";
}

void Raytracer::loadModel(const ObjLoader& obj, const glm::mat4& M, float refl, const Material& mat)
{
    const float* pos = obj.getPositions();
    const unsigned int* idx = obj.getFaces();
    const size_t Nv = obj.getVertCount();
    const size_t Nt = obj.getIndexCount() / 3;

    std::vector<glm::vec3> wPos(Nv);
    for (size_t i = 0; i < Nv; ++i)
    {
        glm::vec4 p(pos[i * 3 + 0],
            pos[i * 3 + 1],
            pos[i * 3 + 2], 1.0f);
        wPos[i] = glm::vec3(M * p);
    }

    for (size_t i = 0; i < Nt; ++i)
    {
        glm::vec3 v0 = wPos[idx[i * 3 + 0]];
        glm::vec3 v1 = wPos[idx[i * 3 + 1]];
        glm::vec3 v2 = wPos[idx[i * 3 + 2]];
        triangles.emplace_back(v0, v1, v2, refl, mat);
    }

    // Build BVH
    std::vector<const Triangle*> triPtrs;
    for (const auto& tri : triangles)
        triPtrs.push_back(&tri);

    bvhRoot = buildBVH(triPtrs);
}
