#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/RHI/RHIBase.h"
#include "hzpch.h"

namespace Hazel
{
#ifdef RHI_USE_VULKAN
    RendererAPI::API RendererAPI::s_API = RendererAPI::API::Vulkan;
#endif
} // namespace Hazel