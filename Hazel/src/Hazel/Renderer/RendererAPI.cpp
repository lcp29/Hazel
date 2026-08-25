// ======== Aster Modify Begin ========
#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/RHI/RHIBase.h"
#include "hzpch.h"

// ======== Aster Modify End ========

namespace Hazel
{
// ======== Aster Modify Begin ========
#ifdef RHI_USE_VULKAN
    RendererAPI::API RendererAPI::s_API = API::Vulkan;
#endif
    // ======== Aster Modify End ========
} // namespace Hazel