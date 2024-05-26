#include "project.h"

#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"

std::filesystem::path SB::Editor::ProjectConfiguration::typeFolder(const rttr::type& type) const
{
    const std::filesystem::path rootObjectFolder = objectFolder();

    if (!type.is_valid())
    {
        return rootObjectFolder;
    }

    const rttr::string_view typeName = type.get_name();
    const std::string typeNameString = typeName.to_string();

    const std::size_t lastColonCharacterPos = typeNameString.find_last_of(':') + 1;

    if (lastColonCharacterPos == std::string::npos || lastColonCharacterPos >= typeNameString.size())
    {
        return rootObjectFolder;
    }

    return rootObjectFolder / typeNameString.substr(lastColonCharacterPos, std::string::npos);
}

bool SB::Editor::Project::openProject(const std::filesystem::path& projectFile)
{
    if (projectFile.empty())
    {
        return false;
    }

    if (!std::filesystem::exists(projectFile))
    {
        return false;
    }

    YAML::Node projectYAML = YAML::LoadFile(projectFile.string());

    m_projectConfig = ProjectConfiguration(projectFile);

    loadObjects();
    loadSystem();
    loadSounds();

    SB::Engine::System::get()->onLoaded();

    return true;
}

void SB::Editor::Project::saveProject() const
{
    saveSystem();
    saveObjects();
    buildSoundbanks();  // temp for testing
}

void SB::Editor::Project::loadSounds()
{
    for (const std::filesystem::directory_entry& p :
         std::filesystem::recursive_directory_iterator(m_projectConfig.sourceFolder()))
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

void SB::Editor::Project::loadSystem()
{
    for (const std::filesystem::directory_entry& p :
         std::filesystem::directory_iterator(m_projectConfig.m_projectFolder))
    {
        if (p.path().extension() == ".yaml")
        {
            YAML::Node node = YAML::LoadFile(p.path().string());
            SB::Core::Serialization::Serializer::loadSystem(SB::Engine::System::get(), node);
        }
    }
}

void SB::Editor::Project::loadObjects()
{
    std::vector<std::filesystem::path> loadPaths{m_projectConfig.objectFolder()};

    for (const std::filesystem::path& path : loadPaths)
    {
        std::filesystem::create_directories(path);

        for (const std::filesystem::directory_entry& p : std::filesystem::recursive_directory_iterator(path))
        {
            if (p.is_regular_file())
            {
                YAML::Node node = YAML::LoadFile(p.path().string());
                SB::Core::Serialization::Serializer::createAndLoadObject(node);
            }
        }
    }
}

void SB::Editor::Project::buildSoundbanks() const
{
    if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
    {
        const std::unordered_set<SB::Core::Object*> soundbankObjects =
            objectTracker->getObjectsOfCategory(SB_CATEGORY_BANK);

        for (auto& soundbankObject : soundbankObjects)
        {
            if (SB::Engine::Soundbank* const soundbank = soundbankObject->tryConvertObject<SB::Engine::Soundbank>())
            {
                YAML::Emitter soundbankEmitter;
                SB::Core::Serialization::Serializer::packageSoundbank(soundbank, soundbankEmitter);

                const std::filesystem::path filePath =
                    m_projectConfig.objectFolder() / m_projectConfig.getIdFilename(soundbank, std::string(".bank"));

                saveYAML(soundbankEmitter, filePath);
            }
        }
    }
}

void SB::Editor::Project::saveSystem() const
{
    YAML::Emitter systemYaml;
    SB::Core::Serialization::Serializer::saveSystem(SB::Engine::System::get(), systemYaml);
    saveYAML(systemYaml, m_projectConfig.m_projectFolder / "System.yaml");
}

void SB::Editor::Project::saveObjects() const
{
    if (SB::Core::Database* const database = SB::Engine::System::getDatabase())
    {
        for (const std::weak_ptr<SB::Core::DatabaseObject>& object : database->getAll())
        {
            if (const std::shared_ptr<SB::Core::DatabaseObject> sharedObject = object.lock())
            {
                if (sharedObject->getEditorHidden())
                {
                    continue;
                }

                YAML::Emitter yaml;
                SB::Core::Serialization::Serializer::saveObject(sharedObject.get(), yaml);

                const std::filesystem::path filePath = m_projectConfig.typeFolder(sharedObject->getType()) / m_projectConfig.getIdFilename(sharedObject.get());
                std::filesystem::create_directories(filePath.parent_path());

                saveYAML(yaml, filePath);
            }
        }
    }
}

void SB::Editor::Project::saveYAML(const YAML::Emitter& emitter, const std::filesystem::path& filePath) const
{
    std::ofstream outputStream(filePath);
    outputStream << emitter.c_str();
}