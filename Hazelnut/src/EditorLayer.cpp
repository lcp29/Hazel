#include "EditorLayer.h"

#include "../../Hazel/vendor/yaml-cpp/src/tag.h"
#include "Hazel/Scene/SceneSerializer.h"
#include "Hazel/Utils/PlatformUtils.h"
#include "Hazel/Math/Math.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/Renderer/GPUAsset/GPURenderTextureAsset.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Project/GlobalSettingRegistry.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo.h>
#include <fstream>

namespace Hazel
{
    namespace
    {
        constexpr const char* s_ProjectFileFilter = "Hazel Project (*.hproj)\0*.hproj\0";
        constexpr const char* s_SceneFileFilter = "Hazel Scene (*.hazel)\0*.hazel\0";

        EditorLayer::EditorUITexture CreateEditorUITexture(Renderer* renderer,
                                                           RHICommandBuffer* commandBuffer,
                                                           RHISampler* sampler,
                                                           const std::filesystem::path& path)
        {
            EditorLayer::EditorUITexture texture;
            if (!renderer || !commandBuffer || !sampler)
                return texture;

            texture.Image = RHIImage::Factory::CreateFromFile(
                renderer->GetDevice(),
                commandBuffer,
                path,
                true,
                renderer->GetDevice()->GetUniformQueue());

            if (!texture.Image)
                return texture;

            RHIImageViewDesc viewDesc{};
            viewDesc.format = texture.Image->GetDesc().format;
            viewDesc.viewType = RHIImageViewType::Image2D;
            viewDesc.subresourceRange.levelCount = 1;
            viewDesc.subresourceRange.layerCount = 1;
            viewDesc.subresourceRange.planes = RHIImagePlaneFlagBits::Color;

            texture.View = texture.Image->CreateView(viewDesc);
            if (!texture.View)
            {
                texture.Image->Release();
                texture.Image = nullptr;
                return texture;
            }

            texture.ImGuiTexture = Application::Get().GetImGuiLayer()->AddTexture(
                sampler,
                texture.View,
                RHIImageResourceState::ShaderRead);
            return texture;
        }

        void ReleaseEditorUITexture(EditorLayer::EditorUITexture& texture)
        {
            if (texture.ImGuiTexture)
            {
                Application::Get().GetImGuiLayer()->RemoveTexture(texture.ImGuiTexture);
                texture.ImGuiTexture = nullptr;
            }
            if (texture.View)
            {
                texture.View->Release();
                texture.View = nullptr;
            }
            if (texture.Image)
            {
                texture.Image->Release();
                texture.Image = nullptr;
            }
        }

        bool IsPathUnderDirectory(const std::filesystem::path& path, const std::filesystem::path& directory)
        {
            std::error_code errorCode;
            const std::filesystem::path normalizedPath = std::filesystem::weakly_canonical(path, errorCode);
            if (errorCode)
            {
                return false;
            }

            const std::filesystem::path normalizedDirectory = std::filesystem::weakly_canonical(directory, errorCode);
            if (errorCode)
            {
                return false;
            }

            auto dirIt = normalizedDirectory.begin();
            auto pathIt = normalizedPath.begin();
            for (; dirIt != normalizedDirectory.end(); ++dirIt, ++pathIt)
            {
                if (pathIt == normalizedPath.end() || *dirIt != *pathIt)
                {
                    return false;
                }
            }

            return true;
        }

        void WriteProjectMakefile(const std::filesystem::path& makefilePath,
                                  const std::string& projectName,
                                  const std::filesystem::path& outputDllRelativePath)
        {
            std::ofstream fout(makefilePath);
            fout << "PROJECT_NAME := " << projectName << "\n";
            fout << "SCRIPT_DIR := Assets/Scripts\n";
            fout << "BIN_DIR := Assets/Scripts/Binaries\n";
            fout << "OUTPUT := " << outputDllRelativePath.generic_string() << "\n";
            fout << "SCRIPT_CORE := Resources/Scripts/Hazel-ScriptCore.dll\n";
            fout << "CS_FILES := $(shell find $(SCRIPT_DIR) -name '*.cs')\n\n";
            fout << "all: $(OUTPUT)\n\n";
            fout << "$(OUTPUT): $(CS_FILES)\n";
            fout << "\t@mkdir -p $(BIN_DIR)\n";
            fout << "\tmcs -target:library -sdk:4.5 -r:$(SCRIPT_CORE) -out:$(OUTPUT) $(CS_FILES)\n\n";
            fout << "clean:\n";
            fout << "\trm -f $(OUTPUT)\n";
        }
    }

    EditorLayer::EditorLayer(Renderer* renderer)
        : Layer("EditorLayer"), m_Renderer(renderer) {}

    void EditorLayer::OnAttach()
    {
        HZ_PROFILE_FUNCTION();

        // Get device and queue from the graphics context
        auto* device = m_Renderer->GetDevice();
        auto* graphicsQueue = device->GetQueue(RHIQueueType::Graphics);

        // Create a temporary command pool and buffer for uploading editor UI textures
        RHICommandPoolDesc uploadPoolDesc{};
        uploadPoolDesc.queueType = RHIQueueType::Graphics;
        uploadPoolDesc.transient = true;
        uploadPoolDesc.allowCommandBufferReset = false;
        RHICommandPool* uploadPool = device->CreateCommandPool(uploadPoolDesc);

        RHICommandBufferDesc uploadBufferDesc{};
        RHICommandBuffer* uploadCommandBuffer = uploadPool->CreateCommandBuffer(uploadBufferDesc);

        uploadCommandBuffer->Begin(true);

        RHISamplerDesc uiSamplerDesc{};
        uiSamplerDesc.minFilter = RHISamplerFilter::Linear;
        uiSamplerDesc.magFilter = RHISamplerFilter::Linear;
        uiSamplerDesc.mipFilter = RHISamplerFilter::Linear;
        uiSamplerDesc.addressModeU = RHISamplerAddressMode::ClampToEdge;
        uiSamplerDesc.addressModeV = RHISamplerAddressMode::ClampToEdge;
        uiSamplerDesc.addressModeW = RHISamplerAddressMode::ClampToEdge;
        m_UISampler = device->CreateSampler(uiSamplerDesc);

        RHISamplerDesc checkerboardSamplerDesc{};
        checkerboardSamplerDesc.minFilter = RHISamplerFilter::Nearest;
        checkerboardSamplerDesc.magFilter = RHISamplerFilter::Nearest;
        checkerboardSamplerDesc.mipFilter = RHISamplerFilter::Nearest;
        checkerboardSamplerDesc.addressModeU = RHISamplerAddressMode::Repeat;
        checkerboardSamplerDesc.addressModeV = RHISamplerAddressMode::Repeat;
        checkerboardSamplerDesc.addressModeW = RHISamplerAddressMode::Repeat;
        m_CheckerboardSampler = device->CreateSampler(checkerboardSamplerDesc);

        m_IconPlay = CreateEditorUITexture(m_Renderer,
                                           uploadCommandBuffer,
                                           m_UISampler,
                                           "Resources/Icons/PlayButton.png");
        m_IconPause = CreateEditorUITexture(m_Renderer,
                                            uploadCommandBuffer,
                                            m_UISampler,
                                            "Resources/Icons/PauseButton.png");
        m_IconStep = CreateEditorUITexture(m_Renderer,
                                           uploadCommandBuffer,
                                           m_UISampler,
                                           "Resources/Icons/StepButton.png");
        m_IconStop = CreateEditorUITexture(m_Renderer,
                                           uploadCommandBuffer,
                                           m_UISampler,
                                           "Resources/Icons/StopButton.png");
        m_ContentBrowserDirectoryIcon = CreateEditorUITexture(m_Renderer,
                                                              uploadCommandBuffer,
                                                              m_UISampler,
                                                              "Resources/Icons/ContentBrowser/DirectoryIcon.png");
        m_ContentBrowserFileIcon = CreateEditorUITexture(m_Renderer,
                                                         uploadCommandBuffer,
                                                         m_UISampler,
                                                         "Resources/Icons/ContentBrowser/FileIcon.png");
        m_CheckerboardTexture = CreateEditorUITexture(m_Renderer,
                                                      uploadCommandBuffer,
                                                      m_CheckerboardSampler,
                                                      "assets/textures/Checkerboard.png");

        uploadCommandBuffer->End();

        RHIQueueSubmitDesc submitDesc{};
        submitDesc.commandBuffers.push_back(uploadCommandBuffer);
        graphicsQueue->Submit(submitDesc);
        device->WaitIdle();

        uploadPool->Release();

        RecreateObjectIDRenderData();
        RecreateDefaultRenderTextureData();

        // Create an empty scene for the editor
        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
        m_SceneHierarchyPanel.SetContext(m_EditorScene);
        m_PropertyPanel.SetContext(m_EditorScene);

        auto commandLineArgs = Application::Get().GetSpecification().CommandLineArgs;
        if (commandLineArgs.Count > 1)
        {
            auto projectFilePath = commandLineArgs[1];
            OpenProject(projectFilePath);
        }
    }

    void EditorLayer::OnDetach()
    {
        HZ_PROFILE_FUNCTION();

        if (HasOpenProject())
        {
            if (m_SceneState == SceneState::Play && m_ActiveScene)
            {
                OnSceneStop();
            }
            Project::GetActive()->GetAssetManager()->ClearLoadedAssets();
            ScriptEngine::Shutdown();
            Project::CloseActive();
        }

        ReleaseEditorUITexture(m_IconPlay);
        ReleaseEditorUITexture(m_IconPause);
        ReleaseEditorUITexture(m_IconStep);
        ReleaseEditorUITexture(m_IconStop);
        ReleaseEditorUITexture(m_ContentBrowserDirectoryIcon);
        ReleaseEditorUITexture(m_ContentBrowserFileIcon);
        ReleaseEditorUITexture(m_CheckerboardTexture);

        if (m_UISampler)
        {
            m_UISampler->Release();
            m_UISampler = nullptr;
        }
        if (m_CheckerboardSampler)
        {
            m_CheckerboardSampler->Release();
            m_CheckerboardSampler = nullptr;
        }
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        HZ_PROFILE_FUNCTION();

        if (!HasOpenProject() || !m_ActiveScene || !m_ContentBrowserPanel)
        {
            return;
        }

        m_ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x),
                                        static_cast<uint32_t>(m_ViewportSize.y));

        auto renderTexture = m_Renderer->GetDefaultRenderTexture();
        if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
            (renderTexture->GetDesc().width != static_cast<uint32_t>(m_ViewportSize.x) ||
             renderTexture->GetDesc().height != static_cast<uint32_t>(m_ViewportSize.y)))
        {
            m_Renderer->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x),
                                         static_cast<uint32_t>(m_ViewportSize.y));
            RecreateObjectIDRenderData();
            RecreateDefaultRenderTextureData();
        }

        switch (m_SceneState)
        {
            case SceneState::Edit:
            {
                if (m_ViewportFocused)
                {
                    m_ActiveScene->GetViewportCameraController().OnUpdate(ts);
                }
                else
                {
                    m_ActiveScene->GetViewportCameraController().StopControlling();
                }

                m_ActiveScene->OnUpdateEditor(ts);
                break;
            }
            case SceneState::Play:
            {
                m_ActiveScene->OnUpdateRuntime(ts);
                break;
            }
        }

        // pull render scene update
        auto payload = m_ActiveScene->GetRenderSceneUpdatePayloads();
        m_Renderer->GetRenderScene()->Update(payload);
        m_Renderer->GetRenderScene()->SortRenderObjectShader();
        m_Renderer->SetCameras(m_ActiveScene->GetAllCameras());

        // render frame buffer
        m_Renderer->Render();

        // transition the default render texture to shader read for ImGui rendering
        auto* image = m_DefaultRenderTextureUIData[m_Renderer->GetCurrentFrameInFlightIndex()].Image;
        image->Transition(
            m_Renderer->GetCurrentFrameData().commandBuffer,
            image->GetCurrentState(),
            RHIImageResourceState::ShaderRead);

        // separate object id map pass
        OnObjectIDMapRender();

        auto [mx, my] = ImGui::GetMousePos();

        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;

        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
        my = viewportSize.y - my;

        int mouseX = static_cast<int>(mx);
        int mouseY = static_cast<int>(my);

        uint64_t currentFrameIndex = m_Renderer->GetCurrentFrameIndex();
        if (mouseX >= 0 && mouseY >= 0 && mouseX < static_cast<int>(viewportSize.x) &&
            mouseY < static_cast<int>(viewportSize.y) &&
            currentFrameIndex > 0)
        {
            size_t pixelIndex = mouseY * static_cast<size_t>(viewportSize.x) + mouseX;

            int pixelData =
                *(static_cast<uint32_t*>(static_cast<GPURenderBufferAsset*>(
                      m_ObjectIDRenderTextureBuffer->asset)->GetBuffer()->Map()) + pixelIndex);
            m_HoveredEntity = pixelData == -1
                                  ? Entity()
                                  : Entity(static_cast<entt::entity>(pixelData), m_ActiveScene.get());
        }

        OnOverlayRender();
    }

    void EditorLayer::RecreateDefaultRenderTextureData()
    {
        for (auto& data : m_DefaultRenderTextureUIData)
        {
            if (data.ImGuiTexture)
            {
                Application::Get().GetImGuiLayer()->RemoveTexture(data.ImGuiTexture);
            }
        }
        m_DefaultRenderTextureUIData.clear();
        GPURenderTextureAsset* defaultRenderTexture = m_Renderer->GetDefaultRenderTexture();
        auto images = defaultRenderTexture->GetAllImages();
        auto imageViews = defaultRenderTexture->GetAllDefaultImageViews();
        for (int i = 0; i < images.size(); i++)
        {
            void* texture = Application::Get().GetImGuiLayer()->AddTexture(
                m_UISampler,
                imageViews[i],
                RHIImageResourceState::ShaderRead);
            m_DefaultRenderTextureUIData.push_back({images[i], imageViews[i], texture});
        }
    }

    void EditorLayer::RecreateObjectIDRenderData()
    {
        m_Renderer->GetDevice()->WaitIdle();

        if (m_ObjectIDRenderTexture)
        {
            m_ObjectIDRenderTexture->Destroy();
            m_ObjectIDRenderTexture = nullptr;
        }

        if (m_ObjectIDDepthRenderTexture)
        {
            m_ObjectIDDepthRenderTexture->Destroy();
            m_ObjectIDDepthRenderTexture = nullptr;
        }

        if (m_ObjectIDRenderTextureBuffer)
        {
            m_ObjectIDRenderTextureBuffer->Destroy();
            m_ObjectIDRenderTextureBuffer = nullptr;
        }

        RenderTextureDesc objectIDRenderTextureDesc{};
        objectIDRenderTextureDesc.width = static_cast<uint32_t>(m_ViewportSize.x);
        objectIDRenderTextureDesc.height = static_cast<uint32_t>(m_ViewportSize.y);
        objectIDRenderTextureDesc.format = RHIFormat::R32SInt;
        objectIDRenderTextureDesc.perFrame = true;
        objectIDRenderTextureDesc.useMipmap = false;
        objectIDRenderTextureDesc.usages = RHIImageUsageFlagBits::TransferSource;

        m_ObjectIDRenderTexture = std::make_unique<GPUAssetHandle>(
            m_Renderer->ResolveGPURenderTexture(objectIDRenderTextureDesc));

        objectIDRenderTextureDesc.format = RHIFormat::D32SFloatS8Uint;
        m_ObjectIDDepthRenderTexture = std::make_unique<GPUAssetHandle>(
            m_Renderer->ResolveGPURenderTexture(objectIDRenderTextureDesc));

        RenderBufferDesc objectIDRenderBufferDesc{};
        objectIDRenderBufferDesc.perFrame = true;
        objectIDRenderBufferDesc.size = objectIDRenderTextureDesc.width * objectIDRenderTextureDesc.height * 4;
        objectIDRenderBufferDesc.usages = RHIBufferUsageFlagBits::TransferDestination;
        objectIDRenderBufferDesc.cpuAccess = RHIBufferCpuAccess::Read;
        objectIDRenderBufferDesc.mapOnCreate = true;
        objectIDRenderBufferDesc.hostCoherent = true;
        objectIDRenderBufferDesc.allowGpuAddress = false;
        objectIDRenderBufferDesc.deviceMemory = true;

        m_ObjectIDRenderTextureBuffer = std::make_unique<GPUAssetHandle>(
            m_Renderer->ResolveGPURenderBuffer(objectIDRenderBufferDesc));
    }

    void EditorLayer::OnObjectIDMapRender()
    {
        auto* commandBuffer = m_Renderer->GetCurrentFrameData().commandBuffer;
        auto* objectIDImage =
            static_cast<GPURenderTextureAsset*>(m_ObjectIDRenderTexture->asset)->GetImage();
        auto* objectIDImageBuffer =
            static_cast<GPURenderBufferAsset*>(m_ObjectIDRenderTextureBuffer->asset)->GetBuffer();
        auto* depthImage =
            static_cast<GPURenderTextureAsset*>(m_ObjectIDDepthRenderTexture->asset)->GetImage();

        objectIDImage->Transition(commandBuffer,
                                  objectIDImage->GetCurrentState(),
                                  RHIImageResourceState::ColorAttachment);

        depthImage->Transition(commandBuffer,
                               depthImage->GetCurrentState(),
                               RHIImageResourceState::DepthStencilAttachment);

        static uint64_t objectIDMaterialID = 15999967383665241091ull;
        auto viewportCamera = m_ActiveScene->GetSceneViewportCamera();

        RHIRenderingAttachmentDesc colorAttachmentDesc{};
        colorAttachmentDesc.imageView = static_cast<GPURenderTextureAsset*>(m_ObjectIDRenderTexture->asset)->
            GetDefaultImageView();
        colorAttachmentDesc.loadOp = RHIRenderingLoadOp::Clear;
        colorAttachmentDesc.storeOp = RHIRenderingStoreOp::Store;
        colorAttachmentDesc.clearColorValue.int32 = {-1, 0, 0, 0};
        colorAttachmentDesc.state = RHIImageResourceState::ColorAttachment;

        RHIRenderingAttachmentDesc depthStencilDesc{};
        depthStencilDesc.imageView = static_cast<GPURenderTextureAsset*>(m_ObjectIDDepthRenderTexture->asset)->
            GetDefaultImageView();
        depthStencilDesc.loadOp = RHIRenderingLoadOp::Clear;
        depthStencilDesc.storeOp = RHIRenderingStoreOp::Store;
        depthStencilDesc.clearDepthStencilValue.depth = 1.0f;
        depthStencilDesc.clearDepthStencilValue.stencil = 0;
        depthStencilDesc.state = RHIImageResourceState::DepthStencilAttachment;

        m_Renderer->RunGraphicsPass(
            commandBuffer,
            objectIDMaterialID,
            &viewportCamera,
            {colorAttachmentDesc},
            {RHIColorBlendAttachmentDesc{}},
            &depthStencilDesc,
            {0, 0},
            {static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y)});

        objectIDImage->Transition(commandBuffer,
                                  objectIDImage->GetCurrentState(),
                                  RHIImageResourceState::TransferSource);

        commandBuffer->CopyImageToBuffer(objectIDImage,
                                         objectIDImageBuffer,
                                         0,
                                         {
                                             static_cast<uint32_t>(m_ViewportSize.x),
                                             static_cast<uint32_t>(m_ViewportSize.y)
                                         },
                                         {0, 0, 0},
                                         {objectIDImage->GetDesc().width, objectIDImage->GetDesc().height, 1},
                                         {
                                             0, 0, 1, RHIImagePlaneFlagBits::Color
                                         });
    }

    void EditorLayer::ResetProjectContext()
    {
        if (m_SceneState == SceneState::Play && m_ActiveScene)
        {
            OnSceneStop();
        }

        if (Project::HasActive())
        {
            ScriptEngine::Shutdown();
            Project::CloseActive();
        }

        m_ContentBrowserPanel.reset();
        m_EditorScenePath.clear();
        m_HoveredEntity = {};
        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
        m_SceneHierarchyPanel.SetContext(m_EditorScene);
        m_PropertyPanel.SetContext(m_EditorScene);
        m_PropertyPanel.SetSelectedMetaPath({});
        m_LastSceneHierarchySelectionVersion = m_SceneHierarchyPanel.GetSelectionVersion();
        m_LastContentBrowserSelectionVersion = 0;
    }

    bool EditorLayer::HasOpenProject() const
    {
        return Project::HasActive();
    }

    void EditorLayer::DrawProjectSelectionScreen()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 windowSize(420.0f, 180.0f);
        ImGui::SetNextWindowPos(
            ImVec2(viewport->GetCenter().x - windowSize.x * 0.5f, viewport->GetCenter().y - windowSize.y * 0.5f));
        ImGui::SetNextWindowSize(windowSize);

        ImGui::Begin("Project",
                     nullptr,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);
        ImGui::TextUnformatted("No project is currently open.");
        ImGui::Spacing();
        ImGui::TextWrapped("Create a new Hazel project or open an existing .hproj file.");
        ImGui::Dummy(ImVec2(0.0f, 16.0f));

        const float buttonWidth = 160.0f;
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((availableWidth - buttonWidth * 2.0f - 12.0f) * 0.5f);
        if (ImGui::Button("New Project", ImVec2(buttonWidth, 0.0f)))
        {
            NewProject();
        }

        ImGui::SameLine();
        if (ImGui::Button("Open Project", ImVec2(buttonWidth, 0.0f)))
        {
            OpenProject();
        }

        ImGui::End();
    }

    void EditorLayer::OnImGuiRender()
    {
        HZ_PROFILE_FUNCTION();

        if (!HasOpenProject())
        {
            DrawProjectSelectionScreen();
            return;
        }

        // Note: Switch this to true to enable dockspace
        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        style.WindowMinSize.x = minWinSizeX;

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Project..."))
                    NewProject();

                if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
                    OpenProject();

                ImGui::Separator();

                if (ImGui::MenuItem("Save Project"))
                    SaveProject();

                ImGui::Separator();

                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                    NewScene();

                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                    SaveScene();

                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                    SaveSceneAs();

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                    Application::Get().Close();

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Script"))
            {
                if (ImGui::MenuItem("Reload assembly", "Ctrl+R"))
                    ScriptEngine::ReloadAssembly();

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        m_SceneHierarchyPanel.OnImGuiRender();
        if (m_ContentBrowserPanel)
        {
            m_ContentBrowserPanel->OnImGuiRender();
        }
        if (m_SceneHierarchyPanel.GetSelectionVersion() != m_LastSceneHierarchySelectionVersion)
        {
            m_LastSceneHierarchySelectionVersion = m_SceneHierarchyPanel.GetSelectionVersion();
            m_PropertyPanel.SetSelectedEntity(m_SceneHierarchyPanel.GetSelectedEntity());
        }
        if (m_ContentBrowserPanel &&
            m_ContentBrowserPanel->GetSelectionVersion() != m_LastContentBrowserSelectionVersion)
        {
            m_LastContentBrowserSelectionVersion = m_ContentBrowserPanel->GetSelectionVersion();
            m_PropertyPanel.SetSelectedMetaPath(m_ContentBrowserPanel->GetSelectedMetaPath());
        }
        m_PropertyPanel.OnImGuiRender();

        ImGui::Begin("Stats");

#if 0
        std::string name = "None";
        if (m_HoveredEntity)
            name = m_HoveredEntity.GetComponent<TagComponent>().tag;
        ImGui::Text("Hovered Entity: %s", name.c_str());
#endif

        // auto stats = Renderer2D::GetStats();
        // ImGui::Text("Renderer2D Stats:");
        // ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        // ImGui::Text("Quads: %d", stats.QuadCount);
        // ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        // ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
        //
        // ImGui::End();
        //
        // ImGui::Begin("Settings");
        // ImGui::Checkbox("Show physics colliders", &m_ShowPhysicsColliders);
        //
        // ImGui::Image((ImTextureID)s_Font->GetAtlasTexture()->GetRendererID(), {512, 512}, {0, 1}, {1, 0});
        //
        //
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin("Viewport");
        auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
        m_ViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};

        if (!m_DefaultRenderTextureUIData.empty())
        {
            void* textureID = m_DefaultRenderTextureUIData[m_Renderer->GetCurrentFrameInFlightIndex()].ImGuiTexture;
#ifdef RHI_USE_VULKAN
            ImGui::Image(textureID, ImVec2{m_ViewportSize.x, m_ViewportSize.y}, ImVec2{0, 1}, ImVec2{1, 0});
#else
            ImGui::Image(textureID, ImVec2{m_ViewportSize.x, m_ViewportSize.y}, ImVec2{0, 0}, ImVec2{1, 1});
#endif
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const wchar_t* path = static_cast<const wchar_t*>(payload->Data);
                OpenScene(path);
            }
            ImGui::EndDragDropTarget();
        }

        // Editor camera
        const glm::mat4& cameraProjection = m_ActiveScene->GetViewportCamera().GetProjection();
        glm::mat4 cameraView = m_ActiveScene->GetSceneViewportCamera().transform.GetView();

        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity && m_GizmoType != -1)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            ImGuizmo::SetRect(m_ViewportBounds[0].x,
                              m_ViewportBounds[0].y,
                              m_ViewportBounds[1].x - m_ViewportBounds[0].x,
                              m_ViewportBounds[1].y - m_ViewportBounds[0].y);

            // Camera

            // Runtime camera from entity
            // auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
            // const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
            // const glm::mat4& cameraProjection = camera.GetProjection();
            // glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());

            // Entity transform
            auto& tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetTransform();

            // Snapping
            bool snap = Input::IsKeyPressed(Key::LeftControl);
            float snapValue = GlobalSettings.Get(GizmoTranslateSnapString, DefaultGizmoTranslateSnap);
            // Snap to 45 degrees for rotation
            if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                snapValue = GlobalSettings.Get(GizmoRotateSnapDegreesString, DefaultGizmoRotateSnapDegrees);

            float snapValues[3] = {snapValue, snapValue, snapValue};

            ImGuizmo::Manipulate(glm::value_ptr(cameraView),
                                 glm::value_ptr(cameraProjection),
                                 static_cast<ImGuizmo::OPERATION>(m_GizmoType),
                                 ImGuizmo::LOCAL,
                                 glm::value_ptr(transform),
                                 nullptr,
                                 snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                Math::DecomposeTransform(transform, translation, rotation, scale);

                selectedEntity.SetTransform(translation, rotation, scale);
            }
        }

        glm::mat3 cameraSpaceWorldFrame = glm::mat3(cameraView);

        glm::vec3 worldFrameX = glm::normalize(cameraSpaceWorldFrame[0]);
        glm::vec3 worldFrameY = glm::normalize(cameraSpaceWorldFrame[1]);
        glm::vec3 worldFrameZ = glm::normalize(cameraSpaceWorldFrame[2]);

        auto projectAxis = [](const glm::vec3& axis) {
            return ImVec2(axis.x, -axis.y);
        };

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 gizmoOrigin = {m_ViewportBounds[1].x - 100, m_ViewportBounds[0].y + 100};

        const std::tuple<glm::vec3, ImU32, const char*> axes[] = {
            {worldFrameX, IM_COL32(255, 0, 0, 255), "X"},
            {worldFrameY, IM_COL32(0, 255, 0, 255), "Y"},
            {worldFrameZ, IM_COL32(0, 0, 255, 255), "Z"}
        };

        for (const auto& [axis, color, label] : axes)
        {
            ImVec2 axisEnd = gizmoOrigin + projectAxis(axis) * 70;
            drawList->AddLine(gizmoOrigin, axisEnd, color, 2);
            drawList->AddCircle(axisEnd, 4, color);
            drawList->AddText(axisEnd, color, label);
        }

        ImGui::End();
        ImGui::PopStyleVar();

        UIToolbar();

        ImGui::End();
    }

    void EditorLayer::UIToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto& colors = ImGui::GetStyle().Colors;
        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##toolbar",
                     nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        bool toolbarEnabled = (bool)m_ActiveScene;

        ImVec4 tintColor = ImVec4(1, 1, 1, 1);
        if (!toolbarEnabled)
            tintColor.w = 0.5f;

        float size = ImGui::GetWindowHeight() - 4.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

        bool hasPlayButton = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play;
        bool hasPauseButton = m_SceneState != SceneState::Edit;

        if (hasPlayButton)
        {
            void* icon = m_SceneState == SceneState::Edit
                             ? m_IconPlay.ImGuiTexture
                             : m_IconStop.ImGuiTexture;
            if (ImGui::ImageButton("PlayButton",
                                   icon,
                                   ImVec2(size, size),
                                   ImVec2(0, 0),
                                   ImVec2(1, 1),
                                   ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
                                   tintColor) && toolbarEnabled)
            {
                if (m_SceneState == SceneState::Edit)
                    OnScenePlay();
                else if (m_SceneState == SceneState::Play)
                    OnSceneStop();
            }
        }

        if (hasPauseButton)
        {
            bool isPaused = m_ActiveScene->IsPaused();
            ImGui::SameLine();
            {
                if (ImGui::ImageButton("PauseButton",
                                       m_IconPause.ImGuiTexture,
                                       ImVec2(size, size),
                                       ImVec2(0, 0),
                                       ImVec2(1, 1),
                                       ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
                                       tintColor) && toolbarEnabled)
                {
                    m_ActiveScene->SetPaused(!isPaused);
                }
            }

            // Step button
            if (isPaused)
            {
                ImGui::SameLine();
                {
                    if (ImGui::ImageButton("StepButton",
                                           m_IconStep.ImGuiTexture,
                                           ImVec2(size, size),
                                           ImVec2(0, 0),
                                           ImVec2(1, 1),
                                           ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
                                           tintColor) && toolbarEnabled)
                    {
                        m_ActiveScene->Step();
                    }
                }
            }
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& e)
    {
        //m_CameraController.OnEvent(e);
        if (m_SceneState == SceneState::Edit)
        {
            //m_EditorCamera.OnEvent(e);
        }

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(HZ_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        // Shortcuts
        if (e.IsRepeat())
            return false;

        bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

        switch (e.GetKeyCode())
        {
            case Key::N:
            {
                if (control && HasOpenProject())
                    NewScene();

                break;
            }
            case Key::O:
            {
                if (control)
                    OpenProject();

                break;
            }
            case Key::S:
            {
                if (control && HasOpenProject())
                {
                    if (shift)
                        SaveSceneAs();
                    else
                        SaveScene();
                }

                break;
            }

            // Scene Commands
            case Key::D:
            {
                if (control && HasOpenProject())
                    OnDuplicateEntity();

                break;
            }

            // Gizmos
            case Key::Q:
            {
                if (m_ActiveScene && m_ActiveScene->GetViewportCameraController().IsControlling())
                    break;
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = -1;
                break;
            }
            case Key::W:
            {
                if (m_ActiveScene && m_ActiveScene->GetViewportCameraController().IsControlling())
                    break;
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            }
            case Key::E:
            {
                if (m_ActiveScene && m_ActiveScene->GetViewportCameraController().IsControlling())
                    break;
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            }
            case Key::R:
            {
                if (control && HasOpenProject())
                {
                    ScriptEngine::ReloadAssembly();
                }
                else
                {
                    if (m_ActiveScene && m_ActiveScene->GetViewportCameraController().IsControlling())
                        break;
                    if (!ImGuizmo::IsUsing())
                        m_GizmoType = ImGuizmo::OPERATION::SCALE;
                }
                break;
            }
            case Key::Delete:
            {
                if (Application::Get().GetImGuiLayer()->GetActiveWidgetID() == 0)
                {
                    Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
                    if (selectedEntity)
                    {
                        m_SceneHierarchyPanel.SetSelectedEntity({});
                        m_ActiveScene->DestroyEntity(selectedEntity);
                    }
                }
                break;
            }
        }

        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        if (e.GetMouseButton() == Mouse::ButtonLeft)
        {
            if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
                m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
        }
        return false;
    }

    void EditorLayer::OnOverlayRender()
    {
        // draw selected entity outline
        if (Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity()) {}
    }

    void EditorLayer::NewProject()
    {
        if (HasOpenProject())
        {
            ResetProjectContext();
        }

        std::string filepath = FileDialogs::SaveFile(s_ProjectFileFilter);
        if (filepath.empty())
        {
            return;
        }

        std::filesystem::path projectFilePath = filepath;
        if (projectFilePath.extension() != ".hproj")
        {
            projectFilePath.replace_extension(".hproj");
        }

        const std::filesystem::path projectDirectory = projectFilePath.parent_path();
        const std::string projectName = projectFilePath.stem().string();
        const std::filesystem::path assetDirectory = projectDirectory / "Assets";
        const std::filesystem::path scenesDirectory = assetDirectory / "Scenes";
        const std::filesystem::path scriptsDirectory = assetDirectory / "Scripts";
        const std::filesystem::path binariesDirectory = scriptsDirectory / "Binaries";
        const std::filesystem::path defaultSceneRelativePath = std::filesystem::path("Scenes") / "Untitled.hazel";
        const std::filesystem::path defaultScenePath = assetDirectory / defaultSceneRelativePath;
        const std::filesystem::path scriptModuleRelativePath =
            std::filesystem::path("Scripts") / "Binaries" / (projectName + ".dll");

        std::filesystem::create_directories(scenesDirectory);
        std::filesystem::create_directories(binariesDirectory);

        Ref<Project> project = Project::New();
        auto& config = project->GetConfig();
        config.Name = projectName;
        config.AssetDirectory = "Assets";
        config.StartScene = defaultSceneRelativePath;
        config.ScriptModulePath = scriptModuleRelativePath;

        if (!Project::SaveActive(projectFilePath))
        {
            return;
        }

        Ref<Scene> defaultScene = CreateRef<Scene>();
        SerializeScene(defaultScene, defaultScenePath);
        WriteProjectMakefile(projectDirectory / "Makefile",
                             projectName,
                             assetDirectory.filename() / scriptModuleRelativePath);

        Project::CloseActive();
        OpenProject(projectFilePath);
    }

    void EditorLayer::OpenProject(const std::filesystem::path& path)
    {
        if (!path.empty() && HasOpenProject())
        {
            ResetProjectContext();
        }

        Ref<Project> project = Project::Load(path, m_Renderer);
        if (!project)
        {
            return;
        }

        ScriptEngine::Init();
        m_ContentBrowserPanel = CreateScope<ContentBrowserPanel>(
            m_ContentBrowserDirectoryIcon.ImGuiTexture,
            m_ContentBrowserFileIcon.ImGuiTexture);
        m_LastContentBrowserSelectionVersion = m_ContentBrowserPanel->GetSelectionVersion();

        const auto& startSceneRelativePath = project->GetConfig().StartScene;
        const std::filesystem::path startScenePath = Project::GetAssetFileSystemPath(startSceneRelativePath);
        if (!startSceneRelativePath.empty() && std::filesystem::exists(startScenePath))
        {
            OpenScene(startScenePath);
        }
        else
        {
            NewScene();
        }

        RecreateObjectIDRenderData();
        RecreateDefaultRenderTextureData();
    }

    bool EditorLayer::OpenProject()
    {
        std::string filepath = FileDialogs::OpenFile(s_ProjectFileFilter);
        if (filepath.empty())
            return false;

        OpenProject(filepath);
        return HasOpenProject();
    }

    void EditorLayer::SaveProject()
    {
        if (!HasOpenProject())
        {
            return;
        }

        Project::SaveActive(Project::GetProjectFilePath());
    }

    void EditorLayer::NewScene()
    {
        if (!HasOpenProject())
        {
            return;
        }

        if (m_SceneState != SceneState::Edit)
        {
            OnSceneStop();
        }

        Project::GetActive()->GetAssetManager()->ClearLoadedAssets();
        m_Renderer->ClearRenderScene();

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
        m_SceneHierarchyPanel.SetContext(m_EditorScene);
        m_PropertyPanel.SetContext(m_EditorScene);
        m_EditorScenePath.clear();
        m_HoveredEntity = {};
    }

    void EditorLayer::OpenScene()
    {
        if (!HasOpenProject())
        {
            return;
        }

        std::string filepath = FileDialogs::OpenFile(s_SceneFileFilter);
        if (!filepath.empty())
            OpenScene(filepath);
    }

    void EditorLayer::OpenScene(const std::filesystem::path& path)
    {
        if (!HasOpenProject())
        {
            return;
        }

        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        if (path.extension().string() != ".hazel")
        {
            HZ_WARN("Could not load {0} - not a scene file", path.filename().string());
            return;
        }

        Project::GetActive()->GetAssetManager()->ClearLoadedAssets();
        m_Renderer->ClearRenderScene();

        Ref<Scene> newScene = CreateRef<Scene>();
        SceneSerializer serializer(newScene);
        if (serializer.Deserialize(path.string()))
        {
            m_EditorScene = newScene;
            m_ActiveScene = m_EditorScene;
            m_SceneHierarchyPanel.SetContext(m_EditorScene);
            m_PropertyPanel.SetContext(m_EditorScene);
            m_EditorScenePath = path;
            m_HoveredEntity = {};
        }
        auto initialRenderScenePayload = newScene->GetInitialRenderSceneUpdatePayloads();
        m_Renderer->GetRenderScene()->Update(std::move(initialRenderScenePayload));
    }

    void EditorLayer::SaveScene()
    {
        if (!HasOpenProject() || !m_ActiveScene)
        {
            return;
        }

        if (!m_EditorScenePath.empty())
            SerializeScene(m_ActiveScene, m_EditorScenePath);
        else
            SaveSceneAs();
    }

    void EditorLayer::SaveSceneAs()
    {
        if (!HasOpenProject() || !m_ActiveScene)
        {
            return;
        }

        std::string filepath = FileDialogs::SaveFile(s_SceneFileFilter);
        if (!filepath.empty())
        {
            std::filesystem::path scenePath = filepath;
            if (scenePath.extension() != ".hazel")
            {
                scenePath.replace_extension(".hazel");
            }

            SerializeScene(m_ActiveScene, scenePath);
            m_EditorScenePath = scenePath;

            const std::filesystem::path assetDirectory = Project::GetAssetDirectory();
            if (IsPathUnderDirectory(scenePath, assetDirectory))
            {
                Project::GetActive()->GetConfig().StartScene = std::filesystem::relative(scenePath, assetDirectory);
                SaveProject();
            }
        }
    }

    void EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& path)
    {
        SceneSerializer serializer(scene);
        serializer.Serialize(path.string());
    }

    void EditorLayer::OnScenePlay()
    {
        m_SceneState = SceneState::Play;

        m_ActiveScene = Scene::Copy(m_EditorScene);
        m_ActiveScene->OnRuntimeStart();

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
        m_PropertyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnSceneStop()
    {
        HZ_CORE_ASSERT(m_SceneState == SceneState::Play);

        m_ActiveScene->OnRuntimeStop();

        m_SceneState = SceneState::Edit;

        m_ActiveScene = m_EditorScene;

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
        m_PropertyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnScenePause()
    {
        if (m_SceneState == SceneState::Edit)
            return;

        m_ActiveScene->SetPaused(true);
    }

    void EditorLayer::OnDuplicateEntity()
    {
        if (m_SceneState != SceneState::Edit)
            return;

        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity)
        {
            Entity newEntity = m_EditorScene->DuplicateEntity(selectedEntity);
            m_SceneHierarchyPanel.SetSelectedEntity(newEntity);
        }
    }
}