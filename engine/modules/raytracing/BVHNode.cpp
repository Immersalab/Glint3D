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

    // If this is a leaf node, check triangles
    if (!left && !right)
    {
        for (const auto* tri : triangles)
        {
            float localT;
            glm::vec3 normal;
            if (tri->intersect(ray, localT, normal))
            {
                outTri = tri;
                tOut = localT;
                return true;
            }
        }
        return false;
    }

    // Check children - return immediately if any child finds an intersection
    if (left && left->intersectAny(ray, outTri, tOut))
        return true;
    if (right && right->intersectAny(ray, outTri, tOut))
        return true;

    return false;
}
