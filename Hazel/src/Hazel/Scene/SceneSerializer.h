#pragma once

#include "Scene.h"

namespace Hazel
{
    class SceneSerializer
    {
    public:
        SceneSerializer(const Ref<Scene>& scene)
            : m_Scene(scene)
        {
        }

        void Serialize(const std::string& filepath) const;
        bool Deserialize(const std::string& filepath);

    private:
        Ref<Scene> m_Scene;
    };
} // namespace Hazel