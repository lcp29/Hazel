#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/LayerStack.h"
// ======== Aster Modify Begin ========
#include "Hazel/Core/Window.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/Event.h"
#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/Renderer/Renderer.h"
// ======== Aster Modify End ========

int main(int argc, char** argv);

namespace Hazel
{
    struct ApplicationCommandLineArgs
    {
        int Count = 0;
        char** Args = nullptr;

        const char* operator[](int index) const
        {
            HZ_CORE_ASSERT(index < Count);
            return Args[index];
        }
    };

    struct ApplicationSpecification
    {
        std::string Name = "Hazel Application";
        std::string WorkingDirectory;
        ApplicationCommandLineArgs CommandLineArgs;
    };

    class Application
    {
      public:
        Application(const ApplicationSpecification& specification);
        virtual ~Application();

        void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);

        Window& GetWindow() { return *m_Window; }

        // ======== Aster Modify Begin ========
        GraphicsContext* GetGraphicsContext() const { return m_GraphicsContext.get(); }

        Renderer* GetRenderer() const { return m_Renderer.get(); }

        // ======== Aster Modify End ========

        ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

        // ======== Aster Modify Begin ========
        void Close();

        // ======== Aster Modify End ========

        static Application& Get() { return *s_Instance; }

        const ApplicationSpecification& GetSpecification() const { return m_Specification; }

        void SubmitToMainThread(const std::function<void()>& function);

        // ======== Aster Modify Begin ========
      protected:
        // ======== Aster Modify End ========
        void Run();
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

        void ExecuteMainThreadQueue();

        ApplicationSpecification m_Specification;
        Scope<Window> m_Window;
        // ======== Aster Modify Begin ========
        Scope<GraphicsContext> m_GraphicsContext;
        Scope<Renderer> m_Renderer;
        // ======== Aster Modify End ========
        ImGuiLayer* m_ImGuiLayer;
        bool m_Running = true;
        bool m_Minimized = false;
        LayerStack m_LayerStack;
        float m_LastFrameTime = 0.0f;

        std::vector<std::function<void()>> m_MainThreadQueue;
        std::mutex m_MainThreadQueueMutex;

        static Application* s_Instance;
        friend int ::main(int argc, char** argv);
    };

    // To be defined in CLIENT
    Application* CreateApplication(ApplicationCommandLineArgs args);
} // namespace Hazel