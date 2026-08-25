#pragma once

#include <glm/glm.hpp>

namespace Hazel
{
    class RendererAPI
    {
      public:
        enum class API
        {
            None = 0,
            // ======== Aster Modify Begin ========
            Vulkan = 1
            // ======== Aster Modify End ========
        };

        static API GetAPI() { return s_API; }

        // ======== Aster Modify Begin ========
        static bool ImageYPositionInverted() { return s_API == API::Vulkan; }

        // ======== Aster Modify End ========

      private:
        static API s_API;
    };
} // namespace Hazel