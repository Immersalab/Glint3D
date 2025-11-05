#include "importer_registry.h"
#include <mutex>

// Factories implemented in plugin sources
std::unique_ptr<IImporter> CreateOBJImporter();
#ifdef USE_ASSIMP
std::unique_ptr<IImporter> CreateAssimpImporter();
#endif

const std::vector<std::unique_ptr<IImporter>>& GetImporters()
{
    static std::vector<std::unique_ptr<IImporter>> s_importers;
    static std::once_flag s_once;
    std::call_once(s_once, []{
        s_importers.emplace_back(CreateOBJImporter());
        (void)0;
        #ifdef USE_ASSIMP
        s_importers.emplace_back(CreateAssimpImporter());
        #endif
    });
    return s_importers;
}

