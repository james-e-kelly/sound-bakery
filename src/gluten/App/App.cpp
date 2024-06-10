#include "App.h"

#include "Subsystems/RendererSubsystem.h"
#include "Subsystems/WidgetSubsystem.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

int App::Run(int argc, char** argv)
{
    m_executableLocation = std::string(argv[0]);

    AddSubsystemClass<RendererSubsystem>();
    AddSubsystemClass<WidgetSubsystem>();

    // PreInit
    for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
    {
        if (int errorCode = subsystem->PreInit(argc, argv); errorCode != 0)
        {
            return errorCode;
        }
    }

    // Init
    for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
    {
        if (int errorCode = subsystem->Init(); errorCode != 0)
        {
            return errorCode;
        }
    }

    m_currentTime = std::chrono::high_resolution_clock::now();
    m_previousTime = std::chrono::high_resolution_clock::now();

    m_hasInit = true;

    // Tick
    while (!m_isRequestingExit)
    {
        m_currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeDiff = std::chrono::duration_cast<std::chrono::duration<double>>(m_currentTime - m_previousTime);
        m_previousTime = m_currentTime;

        double deltaTime = timeDiff.count();

        // PreTick
        for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
        {
            subsystem->PreTick(deltaTime);
        }

        if (m_isRequestingExit)
        {
            break;
        }

        // Tick
        for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
        {
            subsystem->Tick(deltaTime);
        }

        for (auto& manager : m_managers)
        {
            manager->Tick(deltaTime);
        }

        // Rendering
        for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
        {
            subsystem->TickRendering(deltaTime);
        }
    }

    for (auto& manager : m_managers)
    {
        manager->Exit();
    }

    for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
    {
        subsystem->Exit();
    }

    return 0;
}

void App::RequestExit() { m_isRequestingExit = true; }