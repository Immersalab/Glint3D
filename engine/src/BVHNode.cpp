#include "BVHNode.h"
#include "RayUtils.h"
#include <algorithm>

extern bool rayIntersectsAABB(const Ray& ray, const glm::vec3& minBound, const glm::vec3& maxBound, float& t);

BVHNode::~BVHNode()
{
    delete left;
    delete right;
}

bool BVHNode::intersect(const Ray& ray, const Triangle*& outTri, float& outT, glm::vec3& outNormal) const
{
    float tempT;
    if (!rayIntersectsAABB(ray, boundsMin, boundsMax, tempT))
        return false;

    bool hit = false;
    float closest = outT; // start with incoming outT

    for (auto tri : triangles)
    {
        float localT;
        glm::vec3 localNormal;
        if (tri->intersect(ray, localT, localNormal) && localT < closest)
        {
            closest = localT;
            outTri = tri;
            outNormal = localNormal;
            hit = true;
        }
    }

    if (left)
    {
        const Triangle* triLeft = nullptr;
        glm::vec3 normalLeft;
        float tLeft = closest;
        if (left->intersect(ray, triLeft, tLeft, normalLeft) && tLeft < closest)
        {
            closest = tLeft;
            outTri = triLeft;
            outNormal = normalLeft;
            hit = true;
        }
    }

    if (right)
    {
        const Triangle* triRight = nullptr;
        glm::vec3 normalRight;
        float tRight = closest;
        if (right->intersect(ray, triRight, tRight, normalRight) && tRight < closest)
        {
            closest = tRight;
            outTri = triRight;
            outNormal = normalRight;
            hit = true;
        }
    }

    if (hit)
        outT = closest;

    return hit;
}

bool BVHNode::intersectAny(const Ray& ray, const Triangle*& outTri, float& tOut) const
{
    float t;
    if (!rayIntersectsAABB(ray, boundsMin, boundsMax, t))
        return false;

    bool hit = false;

    if (left)
        hit |= left->intersectAny(ray, outTri, tOut);
    if (right)
        hit |= right->intersectAny(ray, outTri, tOut);

    if (!left && !right)
    {
        for (const auto* tri : triangles)
        {
            float t;
            glm::vec3 normal;
            if (tri->intersect(ray, t, normal))
            {
                outTri = tri;
                tOut = t;
                return true;
            }
        }
    }

    return hit;
}
