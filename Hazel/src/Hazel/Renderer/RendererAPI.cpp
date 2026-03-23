#include "hzpch.h"

#include "Hazel/RHI/RHIBase.h"
#include "Hazel/Renderer/RendererAPI.h"


namespace Hazel
{
#ifdef RHI_USE_VULKAN
    RendererAPI::API RendererAPI::s_API = RendererAPI::API::Vulkan;
#endif
}
