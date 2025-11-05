// Machine Summary Block
// {"file":"engine/modules/gizmos/gizmo.h","purpose":"Declares gizmo overlay helpers for manipulating scene objects.","exports":["Gizmo","GizmoMode","GizmoAxis"],"depends_on":["gl_platform.h","glm/glm.hpp","ray.h"],"notes":["requires_GL_context","supports_axis_picking"]}
// Human Summary
// Scene gizmo interface used to render translation/rotation/scale handles and process hit tests.

#pragma once
/// @file gizmo.h
/// @brief Interactive gizmo overlay used for scene object manipulation and axis picking.

#ifndef GLINT_ENABLE_GIZMOS
#define GLINT_ENABLE_GIZMOS 1
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include "ray.h"

/// @brief Enumerates manipulator modes the gizmo can present.
enum class GizmoMode { Translate = 0, Rotate = 1, Scale = 2 };

/// @brief Identifies the active axis (if any) under the cursor.
enum class GizmoAxis { None = 0, X = 1, Y = 2, Z = 3 };

#if GLINT_ENABLE_GIZMOS
#include "gl_platform.h"

/// @brief Renders an interactive axis triad and supports hit testing for object manipulation.
class Gizmo {
public:
    /// @brief Constructs an uninitialized gizmo instance.
    Gizmo() = default;

    /// @brief Creates GPU buffers and shader resources required for rendering.
    void init();

    /// @brief Releases GPU resources allocated by init().
    void cleanup();

    /// @brief Renders the gizmo oriented around the provided transform.
    /// @param view Current camera view matrix.
    /// @param proj Current camera projection matrix.
    /// @param origin World-space origin of the gizmo.
    /// @param orientation Column-major matrix containing world X/Y/Z axes.
    /// @param scale Scale factor applied to the gizmo axes.
    /// @param active Axis currently highlighted.
    /// @param mode Operation mode influencing axis colors.
    void render(const glm::mat4& view,
                const glm::mat4& proj,
                const glm::vec3& origin,
                const glm::mat3& orientation,
                float scale,
                GizmoAxis active,
                GizmoMode mode);

    /// @brief Tests whether a ray intersects an axis handle.
    /// @param ray World-space ray to test.
    /// @param origin Gizmo origin used for rendering.
    /// @param orientation Column-major matrix containing world axes.
    /// @param scale Current axis scale in world units.
    /// @param outAxis Receives the axis that was hit.
    /// @param outS Receives the parametric distance along the axis.
    /// @param outAxisDir Receives the axis direction in world space.
    /// @return True if the ray intersects an axis within tolerance.
    bool pickAxis(const Ray& ray,
                  const glm::vec3& origin,
                  const glm::mat3& orientation,
                  float scale,
                  GizmoAxis& outAxis,
                  float& outS,
                  glm::vec3& outAxisDir) const;

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_prog = 0;
};

#else

/// @brief No-op gizmo implementation used when gizmo support is disabled.
class Gizmo {
public:
    Gizmo() = default;
    void init() {}
    void cleanup() {}
    void render(const glm::mat4&, const glm::mat4&, const glm::vec3&, const glm::mat3&, float, GizmoAxis, GizmoMode) {}
    bool pickAxis(const Ray&, const glm::vec3&, const glm::mat3&, float, GizmoAxis& axis, float& param, glm::vec3& dir) const {
        axis = GizmoAxis::None;
        param = 0.0f;
        dir = glm::vec3(0.0f);
        return false;
    }
};

#endif // GLINT_ENABLE_GIZMOS
