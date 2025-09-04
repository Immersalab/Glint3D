#include "assimp_loader.h"

#include <limits>
#include <iostream>
#include <filesystem>

#ifdef USE_ASSIMP
  #include <assimp/Importer.hpp>
  #include <assimp/scene.h>
  #include <assimp/postprocess.h>
#endif

bool AssimpImportMesh(const std::string& path,
                      std::vector<glm::vec3>& positions,
                      std::vector<unsigned>& indices,
                      std::vector<glm::vec3>& normals,
                      glm::vec3& minBound,
                      glm::vec3& maxBound,
                      std::string* error,
                      std::vector<glm::vec2>* uvs,
                      std::vector<glm::vec3>* tangents,
                      PBRMaterial* pbrOut,
                      bool flipUV)
{
    positions.clear();
    indices.clear();
    normals.clear();
    minBound = glm::vec3(std::numeric_limits<float>::max());
    maxBound = glm::vec3(std::numeric_limits<float>::lowest());

#ifndef USE_ASSIMP
    if (error) *error = "Assimp disabled. Define USE_ASSIMP and link Assimp to enable.";
    (void)path; // unused
    return false;
#else
    Assimp::Importer importer;
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_PreTransformVertices |
        aiProcess_SortByPType |
        aiProcess_CalcTangentSpace;

    const aiScene* scene = importer.ReadFile(path, flags);
    if (!scene || !scene->mRootNode)
    {
        if (error) *error = importer.GetErrorString();
        return false;
    }

    // After aiProcess_PreTransformVertices, node transforms are baked into meshes
    for (unsigned m = 0; m < scene->mNumMeshes; ++m)
    {
        const aiMesh* mesh = scene->mMeshes[m];
        if (!mesh) continue;

        const unsigned baseIndex = static_cast<unsigned>(positions.size());
        positions.reserve(positions.size() + mesh->mNumVertices);
        normals.reserve(normals.size() + mesh->mNumVertices);
        if (uvs) uvs->reserve((uvs->size()) + mesh->mNumVertices);
        if (tangents) tangents->reserve((tangents->size()) + mesh->mNumVertices);

        for (unsigned v = 0; v < mesh->mNumVertices; ++v)
        {
            aiVector3D p = mesh->mVertices[v];
            positions.emplace_back(p.x, p.y, p.z);
            minBound = glm::min(minBound, glm::vec3(p.x, p.y, p.z));
            maxBound = glm::max(maxBound, glm::vec3(p.x, p.y, p.z));

            if (mesh->HasNormals())
            {
                aiVector3D n = mesh->mNormals[v];
                normals.emplace_back(n.x, n.y, n.z);
            }
            else
            {
                normals.emplace_back(0.0f); // will be fixed later if needed
            }

            if (uvs)
            {
                if (mesh->HasTextureCoords(0))
                {
                    aiVector3D tuv = mesh->mTextureCoords[0][v];
                    float u = tuv.x;
                    float vflip = flipUV ? (1.0f - tuv.y) : tuv.y;
                    uvs->emplace_back(u, vflip);
                }
                else
                {
                    uvs->emplace_back(0.0f, 0.0f);
                }
            }
            if (tangents)
            {
                if (mesh->HasTangentsAndBitangents())
                {
                    aiVector3D t = mesh->mTangents[v];
                    tangents->emplace_back(t.x, t.y, t.z);
                }
                else
                {
                    tangents->emplace_back(0.0f, 0.0f, 1.0f);
                }
            }
        }

        for (unsigned f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue; // ensure triangulated (should be by flags)
            indices.push_back(baseIndex + face.mIndices[0]);
            indices.push_back(baseIndex + face.mIndices[1]);
            indices.push_back(baseIndex + face.mIndices[2]);
        }
    }

    // If no normals were provided by any mesh, generate flat ones per face
    bool needNormals = normals.empty();
    if (needNormals)
    {
        normals.assign(positions.size(), glm::vec3(0.0f));
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            unsigned i0 = indices[i+0], i1 = indices[i+1], i2 = indices[i+2];
            const glm::vec3& v0 = positions[i0];
            const glm::vec3& v1 = positions[i1];
            const glm::vec3& v2 = positions[i2];
            glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            normals[i0] += n; normals[i1] += n; normals[i2] += n;
        }
        for (auto& n : normals) n = glm::normalize(n);
    }

    // Try to fill a basic PBR material from the first material in the scene
    if (pbrOut && scene->mNumMaterials > 0)
    {
        const aiMaterial* mat = scene->mMaterials[0];
        aiColor4D base;
        if (AI_SUCCESS == mat->Get(AI_MATKEY_BASE_COLOR, base))
            pbrOut->baseColorFactor = glm::vec4(base.r, base.g, base.b, base.a);
        float metallic = 1.0f, roughness = 1.0f;
        mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
        mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
        pbrOut->metallicFactor = metallic;
        pbrOut->roughnessFactor = roughness;

        // Resolve texture paths relative to asset directory
        auto resolveTex = [&](aiTextureType type) -> std::string {
            aiString p;
            if (AI_SUCCESS == mat->GetTexture(type, 0, &p))
            {
                std::filesystem::path modelPath(path);
                std::filesystem::path dir = modelPath.has_parent_path() ? modelPath.parent_path() : std::filesystem::path(".");
                std::filesystem::path tex = p.C_Str();
                if (tex.is_absolute()) return tex.string();
                return (dir / tex).string();
            }
            return std::string();
        };

        pbrOut->baseColorTex = resolveTex(aiTextureType_BASE_COLOR);
        if (pbrOut->baseColorTex.empty()) pbrOut->baseColorTex = resolveTex(aiTextureType_DIFFUSE);
        pbrOut->normalTex     = resolveTex(aiTextureType_NORMALS);
        pbrOut->mrTex         = resolveTex(aiTextureType_METALNESS);
        if (pbrOut->mrTex.empty()) pbrOut->mrTex = resolveTex(aiTextureType_DIFFUSE_ROUGHNESS);
        if (pbrOut->mrTex.empty()) pbrOut->mrTex = resolveTex(aiTextureType_UNKNOWN); // some exporters
    }

    return !positions.empty() && !indices.empty();
#endif
}
