#include "Hazel/ImGui/ImGuiLayer.h"

#include "hzpch.h"

#include <imgui.h>
#include <imgui_internal.h>

#if defined(RHI_USE_VULKAN)
#include "Hazel/RHI/RHI.h"

#include <imgui_impl_vulkan.h>
#endif

#include "Hazel/Core/Application.h"
#include "ImGuizmo.h"
#include "Hazel/Utils/PlatformUtils.h"

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>

namespace Hazel
{
    ImGuiLayer::ImGuiLayer(Renderer* renderer)
        : Layer("ImGuiLayer")
          , m_Renderer(renderer) {}

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
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

        //float fontSize = 18.0f; // *2.0f;
        //io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", fontSize);
        //io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);
        io.FontGlobalScale = SystemSettings::GetSystemDPIScale();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        SetDarkThemeColors();

        Application& app = Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

        // Setup Platform/Renderer bindings
        RHIInstance* instance = m_Renderer->GetInstance();
        RHIAdapter adapter = m_Renderer->GetAdapter();
        RHIDevice* device = m_Renderer->GetDevice();

        // create imgui resource heap, values hard coded here
        RHIResourceHeapDesc heapDesc{};
        heapDesc.maxGroups = 64;
        heapDesc.samplerWithImageCount = 64;

        m_ResourceHeap = device->CreateResourceHeap(heapDesc, true);

#if defined(RHI_USE_VULKAN)
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
        vk::ImageUsageFlags imageUsage = VulkanConvertImageUsages(RHIImageUsageFlagBits::ColorAttachment
            | RHIImageUsageFlagBits::TransferDestination);
        initInfo.PipelineInfoMain.SwapChainImageUsage = static_cast<VkImageUsageFlags>(imageUsage);
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
            VkPipelineRenderingCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO}
        };
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        VkFormat swapchainColorFormat =
            static_cast<VkFormat>(VulkanConvertFormat(m_Renderer->GetSwapchain()->GetFormat()));
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainColorFormat;

        ImGui_ImplVulkan_Init(&initInfo);
#endif
    }

    void ImGuiLayer::OnDetach()
    {
        HZ_PROFILE_FUNCTION();

#if defined(RHI_USE_VULKAN)
        ImGui_ImplVulkan_Shutdown();
#endif
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        operator delete(m_ResourceHeap);
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

#if defined(RHI_USE_VULKAN)
        ImGui_ImplVulkan_NewFrame();
#endif
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void ImGuiLayer::End(RHICommandBuffer* commandBuffer)
    {
        HZ_PROFILE_FUNCTION();

        ImGuiIO& io = ImGui::GetIO();
        Application& app = Application::Get();
        io.DisplaySize =
            ImVec2(static_cast<float>(app.GetWindow().GetWidth()), static_cast<float>(app.GetWindow().GetHeight()));

        // Rendering
        ImGui::Render();
#if defined(RHI_USE_VULKAN)
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer->GetHandle());
#endif
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

    void* ImGuiLayer::AddTexture(RHISampler* sampler, RHIImageView* imageView, RHIImageResourceState imageState)
    {
        if (!sampler || !imageView)
        {
            return nullptr;
        }
#if defined(RHI_USE_VULKAN)
        return ImGui_ImplVulkan_AddTexture(sampler->GetHandle(),
                                           imageView->GetHandle(),
                                           static_cast<VkImageLayout>(VulkanConvertImageResourceState(imageState)));
#endif
        // other not supported
        return nullptr;
    }

    void ImGuiLayer::RemoveTexture(void* textureID)
    {
#if defined(RHI_USE_VULKAN)
        if (textureID)
        {
            ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(textureID));
        }
#endif
        // other not supported
    }

    uint32_t ImGuiLayer::GetActiveWidgetID() const
    {
        return GImGui->ActiveId;
    }
} // namespace Hazel