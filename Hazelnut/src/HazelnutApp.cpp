// ======== Aster Modify Begin ========
#include "EditorLayer.h"

#include <Hazel.h>
// ======== Aster Modify End ========
#include <Hazel/Core/EntryPoint.h>

namespace Hazel
{
    class Hazelnut : public Application
    {
      public:
        Hazelnut(const ApplicationSpecification& spec)
            : Application(spec)
        // ======== Aster Modify Begin ========
        { PushLayer(new EditorLayer(m_Renderer.get())); }
        // ======== Aster Modify End ========
    };

    Application* CreateApplication(ApplicationCommandLineArgs args)
    {
        ApplicationSpecification spec;
        spec.Name = "Hazelnut";
        spec.CommandLineArgs = args;

        return new Hazelnut(spec);
    }
} // namespace Hazel