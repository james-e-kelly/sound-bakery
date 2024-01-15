#include "app.h"

#include "Managers/AppManager.h"
#include "Managers/ProjectManager.h"
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

    m_hasInit = true;

    m_appManager = std::make_unique<AppManager>(this);
    m_appManager->Init(m_executableLocation);

    // Tick
    while (!m_isRequestingExit)
    {
        double deltaTime = m_appManager->GetDeltaTime();

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

        m_appManager->Tick(deltaTime);

        if (m_projectManager)
        {
            m_projectManager->Tick(deltaTime);
        }

        // Rendering
        for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
        {
            subsystem->TickRendering(deltaTime);
        }
    }

    // Exit
    if (m_projectManager)
    {
        m_projectManager->Exit();
    }

    for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
    {
        subsystem->Exit();
    }

    return 0;
}

void App::RequestExit() { m_isRequestingExit = true; }

std::string App::GetResourceFilePath(const char* filename) const
{
    std::filesystem::path executablePath(
        std::filesystem::path(m_executableLocation).parent_path());
    assert(std::filesystem::exists(executablePath));

    std::filesystem::path resourcePath =
        executablePath;  // by default, the exe sits next to its resources

    // We're in a MacOS App
    if (executablePath.filename().string() == std::string("MacOS"))
    {
        resourcePath =
            executablePath.parent_path() / PathHelpers::ResourcesFolder;
        assert(std::filesystem::exists(resourcePath));
    }

    // We're possibly in a debug folder
    if (!std::filesystem::exists(resourcePath / "fonts"))
    {
        resourcePath = executablePath.parent_path();
    }

    return std::filesystem::path(resourcePath / filename).string();
}

void App::OpenProject(const ProjectConfiguration& projectConfiguration)
{
    if (m_projectManager)
    {
        m_projectManager->Exit();
        m_projectManager.reset();
    }

    m_projectManager = std::make_unique<ProjectManager>(this);
    m_projectManager->Init(std::move(projectConfiguration));
}
