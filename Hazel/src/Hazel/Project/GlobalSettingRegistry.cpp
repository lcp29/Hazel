//
// Created by helmholtz on 2026/3/22.
//

#include "GlobalSettingRegistry.h"

namespace Hazel
{
    GlobalSettingRegistry GlobalSettings;

    YAML::Node GlobalSettingRegistry::Serialize() const
    {
        YAML::Node node(YAML::NodeType::Map);
        for (const auto& [key, value] : m_Registries)
        {
            node[key] = value;
        }

        return node;
    }

    void GlobalSettingRegistry::Deserialize(const YAML::Node& node)
    {
        m_Registries.clear();

        if (!node || !node.IsMap())
        {
            return;
        }

        for (const auto& entry : node)
        {
            if (!entry.first.IsScalar() || !entry.second.IsScalar())
            {
                continue;
            }

            m_Registries[entry.first.as<std::string>()] = entry.second.as<std::string>();
        }
    }
} // namespace Hazel