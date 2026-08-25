// ======== Aster Modify Begin ========
#include "Hazel/ImGui/ImGuiLayer.h"

#include "hzpch.h"
// ======== Aster Modify End ========

#include <imgui.h>

#include <imgui_internal.h>

// ======== Aster Modify Begin ========
#ifdef RHI_USE_VULKAN
#include "Hazel/RHI/RHI.h"

#include <imgui_impl_vulkan.h>
#endif

#include "Hazel/Core/Application.h"
#include "Hazel/Utils/PlatformUtils.h"
#include "ImGuizmo.h"

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>

// ======== Aster Modify End ========

namespace Hazel
{
    // ======== Aster Modify Begin ========
    ImGuiLayer::ImGuiLayer(Renderer* renderer)
        : Layer("ImGuiLayer")
        , m_Renderer(renderer)
    // ======== Aster Modify End ========
    {}

    void ImGuiLayer::OnAttach()
    {
        HZ_PROFILE_FUNCTION();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
        // ======== Aster Modify Begin ========
        io.FontGlobalScale = SystemSettings::GetSystemDPIScale();
        // ======== Aster Modify End ========

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        SetDarkThemeColors();

        Application& app = Application::Get();
        // ======== Aster Modify Begin ========
        auto window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

        // Setup Platform/Renderer bindings
        Aster::RHIInstance* instance = m_Renderer->GetInstance();
        Aster::RHIAdapter adapter = m_Renderer->GetAdapter();
        Aster::RHIDevice* device = m_Renderer->GetDevice();

        // Reserve descriptors used by editor UI textures.
        Aster::RHIResourceHeapDesc heapDesc{};
        heapDesc.maxGroups = 64;
        heapDesc.samplerWithImageCount = 64;

        m_ResourceHeap = device->CreateResourceHeap(heapDesc, true);

#ifdef RHI_USE_VULKAN
        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = vk::ApiVersion13;
        initInfo.Instance = instance->GetHandle();
        initInfo.PhysicalDevice = adapter.GetHandle();
        initInfo.Device = device->GetHandle();
        initInfo.Allocator = nullptr;
        initInfo.DescriptorPool = m_ResourceHeap->GetHandle();
        initInfo.ImageCount = m_Renderer->GetMaxFramesInFlight();
        initInfo.MinImageCount = m_Renderer->GetMaxFramesInFlight();
        auto uniformQueue = device->GetUniformQueue();
        initInfo.Queue = uniformQueue->GetHandle();
        initInfo.QueueFamily = uniformQueue->GetFamilyIndex();

        initInfo.UseDynamicRendering = true;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        vk::ImageUsageFlags imageUsage = VulkanConvertImageUsages(Aster::RHIImageUsageFlagBits::ColorAttachment
                                                                  | Aster::RHIImageUsageFlagBits::TransferDestination);
        initInfo.PipelineInfoMain.SwapChainImageUsage = static_cast<VkImageUsageFlags>(imageUsage);
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
            VkPipelineRenderingCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO}};
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        auto swapchainColorFormat = static_cast<VkFormat>(VulkanConvertFormat(m_Renderer->GetSwapchain()->GetFormat()));
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainColorFormat;

        ImGui_ImplVulkan_Init(&initInfo);
#endif
    }

    // ======== Aster Modify End ========

    void ImGuiLayer::OnDetach()
    {
        HZ_PROFILE_FUNCTION();

// ======== Aster Modify Begin ========
#if defined(RHI_USE_VULKAN)
        ImGui_ImplVulkan_Shutdown();
#endif
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        operator delete(m_ResourceHeap);
        // ======== Aster Modify End ========
    }

    void ImGuiLayer::OnEvent(Event& e)
    {
        if (m_BlockEvents)
        {
            ImGuiIO& io = ImGui::GetIO();
            e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
            e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }
    }

    void ImGuiLayer::Begin()
    {
        HZ_PROFILE_FUNCTION();

// ======== Aster Modify Begin ========
#if defined(RHI_USE_VULKAN)
        ImGui_ImplVulkan_NewFrame();
#endif
        // ======== Aster Modify End ========
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    // ======== Aster Modify Begin ========
    void ImGuiLayer::End(Aster::RHICommandBuffer* commandBuffer)
    // ======== Aster Modify End ========
    {
        HZ_PROFILE_FUNCTION();

        ImGuiIO& io = ImGui::GetIO();
        Application& app = Application::Get();
        io.DisplaySize =
            // ======== Aster Modify Begin ========
            ImVec2(static_cast<float>(app.GetWindow().GetWidth()), static_cast<float>(app.GetWindow().GetHeight()));
        // ======== Aster Modify End ========

        // Rendering
        ImGui::Render();
// ======== Aster Modify Begin ========
#if defined(RHI_USE_VULKAN)
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer->GetHandle());
#endif
        // ======== Aster Modify End ========
    }

    void ImGuiLayer::SetDarkThemeColors()
    {
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

        // Headers
        colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
        colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
        colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    }

    // ======== Aster Modify Begin ========
    void* ImGuiLayer::AddTexture(Aster::RHISampler* sampler,
                                 Aster::RHIImageView* imageView,
                                 Aster::RHIImageResourceState imageState)
    {
        if (!sampler || !imageView) { return nullptr; }
#ifdef RHI_USE_VULKAN
        return ImGui_ImplVulkan_AddTexture(sampler->GetHandle(),
                                           imageView->GetHandle(),
                                           static_cast<VkImageLayout>(VulkanConvertImageResourceState(imageState)));
#else
        return nullptr;
#endif
    }

    void ImGuiLayer::RemoveTexture(void* textureID)
    {
#ifdef RHI_USE_VULKAN
        if (textureID) { ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(textureID)); }
#endif
        // other not supported
    }

    // ======== Aster Modify End ========

    uint32_t ImGuiLayer::GetActiveWidgetID() const { return GImGui->ActiveId; }
} // namespace Hazel
