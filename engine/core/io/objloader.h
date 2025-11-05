#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Face { unsigned int a, b, c; };

class ObjLoader
{
public:
    ObjLoader();

    void load(const char* filename);
    // Populate from raw arrays (triangulated). If normals is empty, they will be computed.
    void setFromRaw(const std::vector<glm::vec3>& positions,
                    const std::vector<unsigned>& indices, // 3*n entries
                    const std::vector<glm::vec3>& normals = {},
                    const std::vector<glm::vec2>& uvs = {},
                    const std::vector<glm::vec3>& tangents = {});

    void reset();

    int                  getVertCount()  const;
    int                  getIndexCount() const;

    const float* getPositions()  const;  // 3×getVertCount() floats
    const unsigned int* getFaces()      const;  // 3×getIndexCount()/3 uints
    const float* getNormals()    const;  // optional (same size as pos)
    const float* getTexcoords()  const;  // optional (2xgetVertCount())
    const float* getTangents()   const;  // optional (3xgetVertCount())
    bool         hasTexcoords()  const { return !Texcoords.empty(); }
    bool         hasTangents()   const { return !Tangents.empty(); }

    glm::vec3            getMinBounds()  const;
    glm::vec3            getMaxBounds()  const;

    // Diagnostics helpers
    // Returns true if normals were provided by the source (not auto-computed here)
    bool                 hadNormalsFromSource() const { return m_normalsProvidedFromSource; }
    // Recompute vertex normals using angle-weighted faces (more robust smoothing)
    void                 computeNormalsAngleWeighted();
    // Flip triangle winding and invert vertex normals
    void                 flipWindingAndNormals();

private:
    void computeNormals();                       // helper
    void computeTangents();                      // requires positions + normals + texcoords

    std::vector<glm::vec3> Positions;
    std::vector<Face>      Faces;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> Texcoords;
    std::vector<glm::vec3> Tangents;

    glm::vec3 minBound, maxBound;

    bool m_normalsProvidedFromSource = false;
};
