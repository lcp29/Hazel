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
            Vulkan = 1
        };

        static API GetAPI() { return s_API; }

        static bool ImageYPositionInverted() { return s_API == API::Vulkan; }

      private:
        static API s_API;
    };
} // namespace Hazel