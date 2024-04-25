#include "AppManager.h"

#include "App/App.h"
#include "Subsystems/WidgetSubsystem.h"
#include "Widgets/SplashWidget.h"
#include "imgui.h"
#include "nfd.h"  // native file dialog

#include "yaml-cpp/yaml.h"

static std::chrono::high_resolution_clock::time_point currentTime =
    std::chrono::high_resolution_clock::now();
static std::chrono::high_resolution_clock::time_point prevTime = currentTime;

void AppManager::Init(const std::string& executablePath)
{
    SplashWidget* splashWidget = GetApp()
                                     ->GetSubsystemByClass<WidgetSubsystem>()
                                     ->AddWidgetClass<SplashWidget>();

    splashWidget->m_OnDestroy.AddRaw(this, &AppManager::OnSplashWidgetDestroy);
    splashWidget->ShowSplashScreen();  // this is not instant. It will show asap

    // Start the clock
    currentTime = std::chrono::high_resolution_clock::now();
    prevTime    = currentTime;
}

double AppManager::GetDeltaTime() const
{
    currentTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> timeDiff =
        std::chrono::duration_cast<std::chrono::duration<double>>(currentTime -
                                                                  prevTime);
    double deltaTime = timeDiff.count();

    prevTime = currentTime;

    return deltaTime;
}

void AppManager::CreateNewProject()
{
    nfdchar_t* outPath = NULL;

retry:
    nfdresult_t pickFolderResult = NFD_PickFolder(
        std::filesystem::current_path().string().c_str(), &outPath);

    switch (pickFolderResult)
    {
        case NFD_OKAY:
            break;

        case NFD_CANCEL:
            return;
            break;

        case NFD_ERROR:
        default:
            goto retry;
            break;
    }

    // Root folder
    const std::filesystem::path rootFolderPath(
        outPath);  // for now, whatever the folder was will be our project name

    // Source folder
    const std::filesystem::path sourceFolder(rootFolderPath / "Source");
    const std::filesystem::path sfxFolder(sourceFolder / "Sounds");
    const std::filesystem::path dxFolder(sourceFolder / "Dialogue");
    const std::filesystem::path mxFolder(sourceFolder / "Music");

    // Objects folder
    const std::filesystem::path objectsFolder(rootFolderPath / "Objects");

    // Events folder
    const std::filesystem::path eventsFolder(rootFolderPath / "Events");

    // Soundbanks folder
    const std::filesystem::path soundbanksFolder(rootFolderPath / "SoundBanks");

    // Project info
    const std::string projectName(rootFolderPath.filename().string());
    const std::filesystem::path projectFile(
        rootFolderPath / (projectName + std::string(".atlas")).c_str());

    // Create the folders
    std::filesystem::create_directory(sourceFolder);
    std::filesystem::create_directory(sfxFolder);
    std::filesystem::create_directory(dxFolder);
    std::filesystem::create_directory(mxFolder);
    std::filesystem::create_directory(objectsFolder);
    std::filesystem::create_directory(eventsFolder);
    std::filesystem::create_directory(soundbanksFolder);

    // Write the yaml project file
    YAML::Emitter projectYAML;

    // Top file message
    projectYAML << YAML::Comment("Atlas Project File") << YAML::Newline;
    projectYAML << YAML::Comment("Project files are designed to be "
                                 "configurable to suit the developers' needs")
                << YAML::Newline;
    projectYAML << YAML::Comment(
                       "The following properties are editable, meaning the "
                       "folder structure can change to suit the project")
                << YAML::Newline;

    // Project Info
    {
        projectYAML << YAML::BeginMap;
        projectYAML << YAML::Key << "Project" << YAML::Value << projectName;
        projectYAML
            << YAML::Key << "Source Folder" << YAML::Value
            << std::filesystem::relative(sourceFolder, rootFolderPath).string();
        {
            projectYAML << YAML::Key << "Source Folders" << YAML::Value
                        << YAML::BeginMap;
            projectYAML
                << YAML::Key << "Sound Folder" << YAML::Value
                << std::filesystem::relative(sfxFolder, sourceFolder).string();
            projectYAML
                << YAML::Key << "Dialogue Folder" << YAML::Value
                << std::filesystem::relative(dxFolder, sourceFolder).string();
            projectYAML
                << YAML::Key << "Music Folder" << YAML::Value
                << std::filesystem::relative(mxFolder, sourceFolder).string();
            projectYAML << YAML::EndMap;
        }

        projectYAML << YAML::Key << "Objects Folder" << YAML::Value
                    << std::filesystem::relative(objectsFolder, rootFolderPath)
                           .string();
        projectYAML
            << YAML::Key << "Events Folder" << YAML::Value
            << std::filesystem::relative(eventsFolder, rootFolderPath).string();
        projectYAML << YAML::Key << "Soundbanks Folder" << YAML::Value
                    << std::filesystem::relative(soundbanksFolder,
                                                 rootFolderPath)
                           .string();
        projectYAML << YAML::EndMap;
    }

    std::ofstream outputStream(projectFile.c_str(), std::ios_base::trunc);
    assert(outputStream.is_open());

    outputStream << projectYAML.c_str();
    outputStream.close();

    GetApp()->OpenProject(projectFile);
}

void AppManager::OpenProject()
{
    nfdchar_t* outPath = NULL;
retry:
    nfdresult_t result = NFD_OpenDialog("atlas", NULL, &outPath);

    if (result == NFD_ERROR)
        goto retry;
    if (result == NFD_CANCEL)
        return;

    assert(std::filesystem::exists(outPath));
    YAML::Node projectYAML = YAML::LoadFile(outPath);

    const std::filesystem::path projectFile(outPath);

    GetApp()->OpenProject(projectFile);
}

void AppManager::OnSplashWidgetDestroy(Widget* widget) {}
