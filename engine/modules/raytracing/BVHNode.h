// Machine Summary Block
// {"file":"engine/modules/raytracing/BVHNode.h","purpose":"Defines nodes used in the raytracer bounding volume hierarchy.","exports":["BVHNode"],"depends_on":["triangle.h","ray.h","glm/glm.hpp"],"notes":["binary_tree_layout","owns_child_memory"]}
// Human Summary
// Bounding volume hierarchy node storing triangles and split references for ray queries.

#pragma once
/// @file BVHNode.h
/// @brief Declares the acceleration structure node used by the raytracer BVH.

#include <vector>
#include "triangle.h"
#include "ray.h"
#include <glm/glm.hpp>

/// @brief Bounding volume hierarchy node containing triangle references and child links.
class BVHNode
{
public:
    glm::vec3 boundsMin{0.0f}; ///< Minimum corner of the axis-aligned bounding box.
    glm::vec3 boundsMax{0.0f}; ///< Maximum corner of the axis-aligned bounding box.
    std::vector<const Triangle*> triangles; ///< Triangle references stored in this node.
    BVHNode* left = nullptr;  ///< Pointer to left child.
    BVHNode* right = nullptr; ///< Pointer to right child.

    /// @brief Releases child nodes recursively.
    ~BVHNode();

    /// @brief Performs a closest-hit intersection test against triangles in the tree.
    /// @param ray Ray to test.
    /// @param outTri Receives the triangle that was hit.
    /// @param outT Receives the distance to the intersection.
    /// @param outNormal Receives the interpolated normal.
    /// @return True if the ray intersects any triangle in the subtree.
    bool intersect(const Ray& ray, const Triangle*& outTri, float& outT, glm::vec3& outNormal) const;

    /// @brief Performs a boolean intersection test returning on the first hit encountered.
    /// @param ray Ray to test.
    /// @param outTri Receives the triangle that was hit.
    /// @param tOut Receives the distance to the hit.
    /// @return True if any triangle is intersected by the ray.
    bool intersectAny(const Ray& ray, const Triangle*& outTri, float& tOut) const;
};
