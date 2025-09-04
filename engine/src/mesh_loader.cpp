#include "mesh_loader.h"
#include "importer_registry.h"
#include <algorithm>

bool LoadMeshFromFile(const std::string& path, MeshData& out, PBRMaterial* pbrOut, std::string* error)
{
    out = MeshData{};
    const auto& importers = GetImporters();
    ImporterOptions opts; // defaults
    std::string lastErr;
    for (const auto& imp : importers) {
        if (!imp || !imp->CanLoad(path)) continue;
        if (imp->Load(path, out, pbrOut, &lastErr, opts)) {
            return true;
        }
    }
    if (error) *error = lastErr.empty() ? std::string("No importer could load the file.") : lastErr;
    return false;
}
