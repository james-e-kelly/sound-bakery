#include "ProjectManager.h"

#include "App/App.h"
#include "Subsystems/WidgetSubsystem.h"
#include "Widgets/DetailsWidget.h"
#include "Widgets/PlayControlsWidget.h"
#include "Widgets/ProjectExplorerWidget.h"

#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

void ProjectManager::Init(const ProjectConfiguration&& projectConfig)
{
    m_projectConfiguration = projectConfig;

    std::filesystem::path currentWorkingDir = std::filesystem::current_path();
    std::filesystem::current_path(m_projectConfiguration.m_sourceFolder);

    // Widgets
    GetApp()
        ->GetSubsystemByClass<WidgetSubsystem>()
        ->AddWidgetClassToRoot<ProjectExplorerWidget>();
    GetApp()
        ->GetSubsystemByClass<WidgetSubsystem>()
        ->AddWidgetClassToRoot<PlayerWidget>();
    GetApp()
        ->GetSubsystemByClass<WidgetSubsystem>()
        ->AddWidgetClassToRoot<DetailsWidget>();

    std::filesystem::current_path(currentWorkingDir);

    // Sound Bakery
    SB::Engine::System::create();

    SB::Engine::System::get()->init();

    m_previewSoundContainer =
        SB::Reflection::cast<SB::Engine::SoundContainer*, SB::Core::Object*>(
            SB::Engine::Factory::createObjectFromType(
                SB::Engine::SoundContainer::type()));
    m_previewSoundContainer->setDatabaseName("Preview Node");

    if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
    {
        objectTracker->untrackObject(m_previewSoundContainer);
    }

    if (SB::Core::Database* const database = SB::Engine::System::getDatabase())
    {
        m_previewSoundContainerResource = database->removeUnsafe(m_previewSoundContainer);
    }

    LoadProject();

    SB::Engine::System::get()->onLoaded();
}

void ProjectManager::Tick(double deltaTime)
{
    SB::Engine::System::get()->update();
}

void ProjectManager::Exit() 
{ 
    m_previewSoundContainerResource.reset();

    SB::Engine::System::get()->destroy(); 
}

void ProjectManager::SaveYAMLToFile(YAML::Emitter& yaml,
                                    std::ofstream& stream,
                                    std::filesystem::path file) const
{
    if (yaml.good() && yaml.size() > 0)
    {
        stream.open(file, std::ios_base::trunc);
        assert(stream.is_open());
        stream << yaml.c_str();
        stream.close();
    }
    else
    {
        std::cout << yaml.GetLastError() << std::endl;
    }
}

void ProjectManager::SaveObjectToFile(YAML::Emitter& yaml,
                                      std::ofstream& stream,
                                      SB_ID objectID,
                                      std::filesystem::path file,
                                      std::string_view fileExtension) const
{
    if (yaml.good() && yaml.size() > 0)
    {
        std::filesystem::path nodeFile =
            file / (std::to_string(objectID) + fileExtension.data());

        std::filesystem::create_directories(nodeFile.parent_path());

        stream.open(nodeFile, std::ios_base::trunc);
        assert(stream.is_open());
        stream << yaml.c_str();
        stream.close();
    }
    else
    {
        std::cout << yaml.GetLastError() << std::endl;
    }
}

void ProjectManager::SaveProject() const
{
    std::ofstream outputStream;

    YAML::Emitter systemYaml;
    SB::Core::Serialization::Serializer::saveSystem(SB::Engine::System::get(),
                                                    systemYaml);
    SaveYAMLToFile(systemYaml, outputStream,
                   m_projectConfiguration.m_projectFolder / "System.yaml");

    if (SB::Core::Database* const database = SB::Engine::System::getDatabase())
    {
        for (const std::weak_ptr<SB::Core::DatabaseObject>& object : database->getAll())
        {
            if (std::shared_ptr<SB::Core::DatabaseObject> sharedObject = object.lock())
            {
                YAML::Emitter yaml;
                SB::Core::Serialization::Serializer::saveObject(sharedObject.get(), yaml);
                SaveObjectToFile(yaml, outputStream, sharedObject->getDatabaseID(),
                                 m_projectConfiguration.m_objectsFolder /
                                     SB::Util::TypeHelper::getFolderNameForObjectType(sharedObject->getType()),
                                 ".yaml");
            }
        }
    }

    // Test output
    if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
    {
        const std::unordered_set<SB::Core::Object*> soundbankObjects = objectTracker->getObjectsOfCategory(SB_CATEGORY_BANK);

        for (auto& soundbankObject : soundbankObjects)
        {
            if (SB::Engine::Soundbank* const soundbank = soundbankObject->tryConvertObject<SB::Engine::Soundbank>())
            {
                YAML::Emitter soundbankEmitter;
                SB::Core::Serialization::Serializer::packageSoundbank(soundbank, soundbankEmitter);

                SaveObjectToFile(soundbankEmitter, outputStream, soundbank->getDatabaseID(),
                                 m_projectConfiguration.m_objectsFolder, ".bank");
            }
        }
    }
}

void ProjectManager::LoadProject()
{
    bool foundSystemFile = false;

    // LOAD OBJECTS

    std::vector<std::filesystem::path> loadPaths{
        m_projectConfiguration.m_objectsFolder};

    for (const std::filesystem::path& path : loadPaths)
    {
        std::filesystem::create_directories(path);

        for (const std::filesystem::directory_entry& p :
             std::filesystem::recursive_directory_iterator(path))
        {
            if (p.is_regular_file())
            {
                YAML::Node node = YAML::LoadFile(p.path().string());
                SB::Core::Serialization::Serializer::createAndLoadObject(node);
            }
        }
    }

    // LOAD SYSTEM

    for (const std::filesystem::directory_entry& p :
         std::filesystem::directory_iterator(
             m_projectConfiguration.m_projectFolder))
    {
        if (p.path().extension() == ".yaml")
        {
            YAML::Node node = YAML::LoadFile(p.path().string());
            SB::Core::Serialization::Serializer::loadSystem(
                SB::Engine::System::get(), node);
            foundSystemFile = true;
        }
    }

    // LOAD SOUNDS

    for (const std::filesystem::directory_entry& p :
         std::filesystem::recursive_directory_iterator(
             m_projectConfiguration.m_sourceFolder))
    {
        if (p.is_regular_file() && p.path().filename().string()[0] != '.')
        {
            const std::filesystem::path filename = p.path().filename();

            if (SB::Core::Database* const database = SB::Engine::System::getDatabase())
            {
                if (database->tryFind(filename.string().c_str()) == nullptr)
                {
                    if (SB::Core::DatabaseObject* const createdSound = newDatabaseObject<SB::Engine::Sound>())
                    {
                        createdSound->setDatabaseName(filename.string().c_str());

                        if (SB::Engine::Sound* const castedSound =
                                SB::Reflection::cast<SB::Engine::Sound*, SB::Core::DatabaseObject*>(createdSound))
                        {
                            castedSound->setSoundName(p.path().string());
                        }
                    }
                }
            }
        }
    }
}
