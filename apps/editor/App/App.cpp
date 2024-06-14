#include "App.h"

#include "Managers/AppManager.h"
#include "Managers/ProjectManager.h"
#include "Subsystems/RendererSubsystem.h"
#include "Subsystems/WidgetSubsystem.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

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

    m_hasInit = true;

    m_appManager = std::make_unique<AppManager>(this);
    m_appManager->Init(m_executableLocation);

    // Tick
    while (!m_isRequestingExit)
    {
        double deltaTime = m_appManager->GetDeltaTime();

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

        m_appManager->Tick(deltaTime);

        if (m_projectManager)
        {
            m_projectManager->Tick(deltaTime);
        }

        // Rendering
        for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->tick_rendering(deltaTime);
        }
    }

    // Exit
    if (m_projectManager)
    {
        m_projectManager->Exit();
    }

    for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
    {
        subsystem->exit();
    }

    return 0;
}

void app::request_exit() { m_isRequestingExit = true; }

void app::OpenProject(const std::filesystem::path& projectFile)
{
    if (m_projectManager)
    {
        m_projectManager->Exit();
        m_projectManager.reset();
    }

    m_projectManager = std::make_unique<ProjectManager>(this);
    m_projectManager->Init(projectFile);
}
