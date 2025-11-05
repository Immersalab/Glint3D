#include "brdf.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float PI = 3.14159265358979323846f;
    inline float saturate(float x) { return std::max(0.0f, std::min(1.0f, x)); }

    // Beckmann normal distribution function (NDF)
    // alpha is surface slope parameter (use roughness^2 as an approximation)
    inline float D_Beckmann(float NdotH, float alpha)
    {
        NdotH = saturate(NdotH);
        const float cos2 = NdotH * NdotH;
        if (cos2 <= 0.0f) return 0.0f;
        const float tan2 = (1.0f - cos2) / cos2;
        const float a2 = alpha * alpha;
        const float denom = PI * a2 * cos2 * cos2;
        if (denom <= 0.0f) return 0.0f;
        return std::exp(-tan2 / a2) / denom;
    }

    // Cook–Torrance geometric attenuation term (also called Smith's for V and L in min form)
    inline float G_CookTorrance(float NdotL, float NdotV, float NdotH, float VdotH)
    {
        if (VdotH <= 0.0f) return 0.0f;
        const float g1 = (2.0f * NdotH * NdotV) / VdotH;
        const float g2 = (2.0f * NdotH * NdotL) / VdotH;
        return std::min(1.0f, std::min(g1, g2));
    }

    inline glm::vec3 Fresnel_Schlick(float cosTheta, const glm::vec3& F0)
    {
        cosTheta = saturate(cosTheta);
        return F0 + (glm::vec3(1.0f) - F0) * std::pow(1.0f - cosTheta, 5.0f);
    }
}

namespace brdf
{
    glm::vec3 cookTorrance(
        const glm::vec3& N,
        const glm::vec3& V,
        const glm::vec3& L,
        const glm::vec3& baseColor,
        float roughness,
        float metallic)
    {
        const glm::vec3 Nn = glm::normalize(N);
        const glm::vec3 Vn = glm::normalize(V);
        const glm::vec3 Ln = glm::normalize(L);
        const glm::vec3 H = glm::normalize(Vn + Ln);

        const float NdotL = std::max(0.0f, glm::dot(Nn, Ln));
        const float NdotV = std::max(0.0f, glm::dot(Nn, Vn));
        if (NdotL <= 0.0f || NdotV <= 0.0f)
            return glm::vec3(0.0f);

        const float NdotH = std::max(0.0f, glm::dot(Nn, H));
        const float VdotH = std::max(0.0f, glm::dot(Vn, H));

        // Map artist roughness to microfacet alpha; clamp to avoid singularities
        const float r = std::max(0.001f, roughness);
        const float alpha = r * r; // Beckmann often uses alpha = roughness^2

        // Dielectric base reflectance ~0.04; metals take baseColor as F0
        const glm::vec3 dielectricF0(0.04f);
        const glm::vec3 F0 = glm::mix(dielectricF0, baseColor, saturate(metallic));

        const float D = D_Beckmann(NdotH, alpha);
        const float G = G_CookTorrance(NdotL, NdotV, NdotH, VdotH);
        const glm::vec3 F = Fresnel_Schlick(VdotH, F0);

        const float denom = std::max(1e-6f, 4.0f * NdotL * NdotV);
        const glm::vec3 spec = (D * G / denom) * F;

        // Energy conservation: reduce diffuse by average Fresnel, and by (1 - metallic)
        const float F_avg = (F.x + F.y + F.z) * (1.0f / 3.0f);
        const float kd = (1.0f - saturate(metallic)) * (1.0f - F_avg);
        const glm::vec3 diffuse = kd * baseColor * (1.0f / PI);

        // Return full BRDF (diffuse + specular), caller multiplies by NdotL and light radiance
        return diffuse + spec;
    }
}
