//
// Created by helmholtz on 2026/3/23.
//

#include "AssetManager.h"

#include "Hazel/Project/Project.h"
#include "Hazel/Renderer/Renderer.h"

#include <cctype>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <string>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    AssetManager::AssetManager(Project* project, Renderer* renderer)
        : m_Project(project)
          , m_Renderer(renderer)
    {
    }

    void AssetManager::WriteAllMetaFiles() const
    {
    }
} // namespace Hazel
