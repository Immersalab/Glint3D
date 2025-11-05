#pragma once
#include <string>
#include <memory>
#include "mesh_loader.h"   // MeshData
#include "pbr_material.h"

struct ImporterOptions {
    bool flipUV = true; // flip V coordinate on load
};

class IImporter {
public:
    virtual ~IImporter() = default;
    virtual const char* Name() const = 0;
    virtual bool CanLoad(const std::string& path) const = 0;
    virtual bool Load(const std::string& path,
                      MeshData& out,
                      PBRMaterial* pbrOut,
                      std::string* error,
                      const ImporterOptions& opts) = 0;
};

