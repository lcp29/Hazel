#pragma once

#include "Scene.h"

namespace Hazel
{
    class SceneSerializer
    {
      public:
        SceneSerializer(const Ref<Scene>& scene)
            // ======== Aster Modify Begin ========
            : m_Scene(scene)
        {}

        void Serialize(const std::string& filepath) const;
        // ======== Aster Modify End ========
        bool Deserialize(const std::string& filepath);

      private:
        Ref<Scene> m_Scene;
    };
} // namespace Hazel