#pragma once
#include <vector>
#include <memory>
#include "importer.h"

// Returns a static list of built-in importers (OBJ, Assimp if enabled).
const std::vector<std::unique_ptr<IImporter>>& GetImporters();

