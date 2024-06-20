#include "app.h"

#include "subsystems/renderer_subsystem.h"
#include "subsystems/widget_subsystem.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

using namespace gluten;

int app::run(int argc, char** argv)
{
    m_executableLocation = std::string(argv[0]);

    add_subsystem_class<renderer_subsystem>();
    add_subsystem_class<widget_subsystem>();

    // PreInit
    for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
    {
        if (int errorCode = subsystem->pre_init(argc, argv); errorCode != 0)
        {
            return errorCode;
        }
    }

    // Init
    for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
    {
        if (int errorCode = subsystem->init(); errorCode != 0)
        {
            return errorCode;
        }
    }

    m_currentTime  = std::chrono::high_resolution_clock::now();
    m_previousTime = std::chrono::high_resolution_clock::now();

    m_hasInit = true;

    // Tick
    while (!m_isRequestingExit)
    {
        m_currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeDiff =
            std::chrono::duration_cast<std::chrono::duration<double>>(m_currentTime - m_previousTime);
        m_previousTime = m_currentTime;

        double deltaTime = timeDiff.count();

        // PreTick
        for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->pre_tick(deltaTime);
        }

        if (m_isRequestingExit)
        {
            break;
        }

        // Tick
        for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->tick(deltaTime);
        }

        for (auto& manager : m_managers)
        {
            manager->Tick(deltaTime);
        }

        // Rendering
        for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->tick_rendering(deltaTime);
        }
    }

    for (auto& manager : m_managers)
    {
        manager->Exit();
    }

    for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
    {
        subsystem->exit();
    }

    return 0;
}

void app::request_exit() { m_isRequestingExit = true; }