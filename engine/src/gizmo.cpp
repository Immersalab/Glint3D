#include "gizmo.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

static const char* kVS = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
void main(){
    vColor = aColor;
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
)GLSL";

static const char* kFS = R"GLSL(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main(){ FragColor = vec4(vColor, 1.0); }
)GLSL";

void Gizmo::init(){
    // Triad lines: origin->X, origin->Y, origin->Z
    const GLfloat verts[] = {
        // pos                // color (set initially; we can update if needed)
         0.f, 0.f, 0.f,       1.f, 0.f, 0.f,
         1.f, 0.f, 0.f,       1.f, 0.f, 0.f,
         0.f, 0.f, 0.f,       0.f, 1.f, 0.f,
         0.f, 1.f, 0.f,       0.f, 1.f, 0.f,
         0.f, 0.f, 0.f,       0.f, 0.f, 1.f,
         0.f, 0.f, 1.f,       0.f, 0.f, 1.f,
    };
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    auto compile = [](GLenum type, const char* src){ GLuint s = glCreateShader(type); glShaderSource(s,1,&src,nullptr); glCompileShader(s); return s; };
    GLuint vs = compile(GL_VERTEX_SHADER, kVS);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFS);
    m_prog = glCreateProgram();
    glAttachShader(m_prog, vs);
    glAttachShader(m_prog, fs);
    glLinkProgram(m_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Gizmo::cleanup(){
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_prog) glDeleteProgram(m_prog);
    m_vao = m_vbo = m_prog = 0;
}

void Gizmo::render(const glm::mat4& view,
                   const glm::mat4& proj,
                   const glm::vec3& origin,
                   const glm::mat3& orientation,
                   float scale,
                   GizmoAxis active,
                   GizmoMode /*mode*/){
    glUseProgram(m_prog);
    glm::mat4 R(1.0f);
    R[0] = glm::vec4(orientation[0], 0.0f);
    R[1] = glm::vec4(orientation[1], 0.0f);
    R[2] = glm::vec4(orientation[2], 0.0f);
    glm::mat4 M = glm::translate(glm::mat4(1.0f), origin) * R * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    GLint locM = glGetUniformLocation(m_prog, "uModel");
    GLint locV = glGetUniformLocation(m_prog, "uView");
    GLint locP = glGetUniformLocation(m_prog, "uProj");
    glUniformMatrix4fv(locM,1,GL_FALSE,glm::value_ptr(M));
    glUniformMatrix4fv(locV,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(locP,1,GL_FALSE,glm::value_ptr(proj));

    // To highlight active axis, we draw all axes, then overdraw the active axis thicker via glLineWidth
    // Always on top
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthWasEnabled) glDisable(GL_DEPTH_TEST);

    glBindVertexArray(m_vao);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 6);
    if (active != GizmoAxis::None) {
        // Draw the active segment again with thicker line
        glLineWidth(6.0f);
        switch (active){
            case GizmoAxis::X: glDrawArrays(GL_LINES, 0, 2); break;
            case GizmoAxis::Y: glDrawArrays(GL_LINES, 2, 2); break;
            case GizmoAxis::Z: glDrawArrays(GL_LINES, 4, 2); break;
            default: break;
        }
        glLineWidth(1.0f);
    }
    glBindVertexArray(0);
    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
}

static bool closestPointParamsOnLines(const glm::vec3& r0, const glm::vec3& rd,
                                      const glm::vec3& s0, const glm::vec3& sd,
                                      float& tOut, float& sOut){
    // Solve for t and s minimizing |(r0 + t*rd) - (s0 + s*sd)|
    const float a = glm::dot(rd, rd);
    const float b = glm::dot(rd, sd);
    const float c = glm::dot(sd, sd);
    const glm::vec3 w0 = r0 - s0;
    const float d = glm::dot(rd, w0);
    const float e = glm::dot(sd, w0);
    const float denom = a*c - b*b;
    if (std::abs(denom) < 1e-6f) return false; // nearly parallel
    tOut = (b*e - c*d) / denom;
    sOut = (a*e - b*d) / denom;
    return true;
}

bool Gizmo::pickAxis(const Ray& ray,
                     const glm::vec3& origin,
                     const glm::mat3& orientation,
                     float scale,
                     GizmoAxis& outAxis,
                     float& outS,
                     glm::vec3& outAxisDir) const {
    const float axisLen = scale;             // model scales unit-length axes
    const float hitRadius = 0.15f * scale;   // tolerance

    struct Candidate { GizmoAxis axis; glm::vec3 dir; int offset; };
    Candidate cands[3] = {
        { GizmoAxis::X, glm::normalize(glm::vec3(orientation[0])), 0 },
        { GizmoAxis::Y, glm::normalize(glm::vec3(orientation[1])), 2 },
        { GizmoAxis::Z, glm::normalize(glm::vec3(orientation[2])), 4 },
    };

    bool any = false; float bestDist = 1e9f; GizmoAxis bestAxis = GizmoAxis::None; float bestS = 0.f; glm::vec3 bestDir(0);
    for (auto& c : cands){
        float t=0,s=0; if (!closestPointParamsOnLines(ray.origin, ray.direction, origin, c.dir, t, s)) continue;
        // clamp s to axis segment [0, axisLen]
        float sClamped = glm::clamp(s, 0.0f, axisLen);
        glm::vec3 pRay = ray.origin + t * ray.direction;
        glm::vec3 pAxis = origin + sClamped * c.dir;
        float d = glm::length(pRay - pAxis);
        if (d < hitRadius && d < bestDist){ any = true; bestDist = d; bestAxis = c.axis; bestS = sClamped; bestDir = c.dir; }
    }
    if (!any) return false;
    outAxis = bestAxis; outS = bestS; outAxisDir = bestDir; return true;
}
