#include "importer.h"
#include "objloader.h"
#include <algorithm>
#include <cctype>

namespace {
static std::string toLowerExt(const std::string& s){
    auto pos = s.find_last_of('.');
    if (pos == std::string::npos) return {};
    std::string e = s.substr(pos);
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return e;
}

class OBJImporter : public IImporter {
public:
    const char* Name() const override { return "OBJImporter"; }
    bool CanLoad(const std::string& path) const override {
        return toLowerExt(path) == ".obj";
    }
    bool Load(const std::string& path, MeshData& out, PBRMaterial* pbrOut, std::string* error, const ImporterOptions& opts) override {
        (void)opts;
        out = MeshData{};
        ObjLoader l; l.load(path.c_str());
        int vc = l.getVertCount();
        int ic = l.getIndexCount();
        const float* pos = l.getPositions();
        const float* nor = l.getNormals();
        const float* uvs = l.getTexcoords();
        const float* tang= l.getTangents();
        const unsigned* idx = l.getFaces();
        if (vc <= 0 || ic <= 0 || !pos || !idx) {
            if (error) *error = "OBJ import failed or produced no geometry.";
            return false;
        }
        out.positions.resize(vc);
        for (int i=0;i<vc;i++) out.positions[i] = glm::vec3(pos[3*i+0], pos[3*i+1], pos[3*i+2]);
        if (nor) { out.normals.resize(vc); for (int i=0;i<vc;i++) out.normals[i] = glm::vec3(nor[3*i+0], nor[3*i+1], nor[3*i+2]); }
        if (uvs) { out.uvs.resize(vc); for (int i=0;i<vc;i++) out.uvs[i] = glm::vec2(uvs[2*i+0], uvs[2*i+1]); }
        if (tang){ out.tangents.resize(vc); for (int i=0;i<vc;i++) out.tangents[i] = glm::vec3(tang[3*i+0], tang[3*i+1], tang[3*i+2]); }
        out.indices.resize(ic);
        for (int i=0;i<ic;i++) out.indices[i] = idx[i];
        out.minBound = l.getMinBounds();
        out.maxBound = l.getMaxBounds();
        if (pbrOut) *pbrOut = PBRMaterial{}; // no textures/factors from plain OBJ path
        return true;
    }
};
}

std::unique_ptr<IImporter> CreateOBJImporter(){ return std::make_unique<OBJImporter>(); }
