#include "importer.h"
#include "assimp_loader.h"
#include <algorithm>
#include <cctype>

#ifdef USE_ASSIMP

namespace {
static std::string toLowerExt(const std::string& s){
    auto pos = s.find_last_of('.');
    if (pos == std::string::npos) return {};
    std::string e = s.substr(pos);
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return e;
}

static bool extIn(const std::string& ext){
    static const char* exts[] = { ".gltf", ".glb", ".fbx", ".dae", ".ply", ".3ds", ".blend", ".off", ".obj" };
    for (auto* e : exts) if (ext == e) return true;
    return false;
}

class AssimpImporter : public IImporter {
public:
    const char* Name() const override { return "AssimpImporter"; }
    bool CanLoad(const std::string& path) const override {
        return extIn(toLowerExt(path));
    }
    bool Load(const std::string& path, MeshData& out, PBRMaterial* pbrOut, std::string* error, const ImporterOptions& opts) override {
        glm::vec3 minB, maxB; std::string err;
        std::vector<glm::vec2> uvs; std::vector<glm::vec3> tangents;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec3> positions;
        std::vector<unsigned> indices;
        PBRMaterial pbrTmp;
        bool ok = AssimpImportMesh(path, positions, indices, normals, minB, maxB, &err, &uvs, &tangents, &pbrTmp, opts.flipUV);
        if (!ok) {
            if (error) *error = err;
            return false;
        }
        out.positions = std::move(positions);
        out.normals   = std::move(normals);
        out.uvs       = std::move(uvs);
        out.tangents  = std::move(tangents);
        out.indices   = std::move(indices);
        out.minBound  = minB;
        out.maxBound  = maxB;
        if (pbrOut) *pbrOut = std::move(pbrTmp);
        return true;
    }
};
}

std::unique_ptr<IImporter> CreateAssimpImporter(){ return std::make_unique<AssimpImporter>(); }

#endif // USE_ASSIMP

