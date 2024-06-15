#include "project.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_chef/sound_chef_bank.h"
#include "sound_chef/sound_chef_encoder.h"

std::filesystem::path sbk::editor::ProjectConfiguration::typeFolder(const rttr::type& type) const
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

bool sbk::editor::project::openProject(const std::filesystem::path& projectFile)
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

    createPreviewContainer();

    return true;
}

void sbk::editor::project::saveProject() const
{
    saveSystem();
    saveObjects();
    buildSoundbanks();  // temp for testing
}

void sbk::editor::project::encodeAllMedia() const
{
    std::shared_ptr<concurrencpp::thread_pool_executor> threadPool = sbk::engine::system::get()->background_executor();

    if (sbk::core::object_tracker* const objectTracker = sbk::engine::system::get())
    {
        for (sbk::core::object* const soundObject : objectTracker->getObjectsOfType(sbk::engine::Sound::type()))
        {
            if (sbk::engine::Sound* const sound = soundObject->try_convert_object<sbk::engine::Sound>())
            {
                const std::filesystem::path encodedSoundFile =
                    m_projectConfig.encodedFolder() / (std::to_string(sound->get_database_id()) + ".ogg");
                std::filesystem::create_directories(encodedSoundFile.parent_path());

                const sc_encoder_config encoderConfig =
                    sc_encoder_config_init((ma_encoding_format_ext)ma_encoding_format_vorbis, ma_format_f32,
                                            0, ma_standard_sample_rate_48000, 8);

                std::filesystem::path soundPath = sound->getSoundName();

                if (!std::filesystem::exists(soundPath))
                {
                    soundPath = m_projectConfig.sourceFolder() / soundPath;
                }

                threadPool->post(
                    [sound, encoderConfig, encodedSoundFile, soundPath]
                    {
                        sc_result result = sc_encoder_write_from_file(soundPath.string().c_str(), encodedSoundFile.string().c_str(),
                                                                        &encoderConfig);
                        assert(result == MA_SUCCESS);

                        concurrencpp::resume_on(sbk::engine::system::get()->game_thread_executer());

                        sound->setEncodedSoundName(encodedSoundFile.string());
                    });
            }
        }
    }
}

void sbk::editor::project::loadSounds()
{
    for (const std::filesystem::directory_entry& p :
         std::filesystem::recursive_directory_iterator(m_projectConfig.sourceFolder()))
    {
        if (p.is_regular_file() && p.path().filename().string()[0] != '.')
        {
            const std::filesystem::path filename = p.path().filename();

            if (sbk::core::database* const database = sbk::engine::system::get())
            {
                if (database->try_find(filename.stem().string().c_str()) == nullptr)
                {
                    if (sbk::core::database_object* const createdSound = newDatabaseObject<sbk::engine::Sound>())
                    {
                        createdSound->set_database_name(filename.stem().string().c_str());

                        if (sbk::engine::Sound* const castedSound =
                                sbk::reflection::cast<sbk::engine::Sound*, sbk::core::database_object*>(createdSound))
                        {
                            castedSound->setSoundName(p.path().string());
                        }
                    }
                }
            }
        }
    }
}

void sbk::editor::project::loadSystem()
{
    for (const std::filesystem::directory_entry& p :
         std::filesystem::directory_iterator(m_projectConfig.m_projectFolder))
    {
        if (p.path().extension() == ".yaml")
        {
            YAML::Node node = YAML::LoadFile(p.path().string());
            sbk::core::Serialization::Serializer::loadSystem(sbk::engine::system::get(), node);
        }
    }
}

void sbk::editor::project::loadObjects()
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
                sbk::core::Serialization::Serializer::createAndLoadObject(node);
            }
        }
    }
}

void sbk::editor::project::createPreviewContainer()
{
    m_previewSoundContainer = newDatabaseObject<sbk::engine::SoundContainer>();
    m_previewSoundContainer->set_database_name("Preview Node");
    m_previewSoundContainer->set_editor_hidden(true);
}

void sbk::editor::project::buildSoundbanks() const
{
    if (sbk::core::object_tracker* const objectTracker = sbk::engine::system::get())
    {
        const std::unordered_set<sbk::core::object*> soundbankObjects =
            objectTracker->getObjectsOfCategory(SB_CATEGORY_BANK);

        for (auto& soundbankObject : soundbankObjects)
        {
            if (sbk::engine::Soundbank* const soundbank = soundbankObject->try_convert_object<sbk::engine::Soundbank>())
            {
                /*YAML::Emitter soundbankEmitter;
                SB::Core::Serialization::Serializer::packageSoundbank(soundbank, soundbankEmitter);

                const std::filesystem::path filePath =
                    m_projectConfig.objectFolder() / m_projectConfig.getIdFilename(soundbank, std::string(".bank"));

                saveYAML(soundbankEmitter, filePath);*/

                std::vector<sbk::engine::NodeBase*> nodesToSave;

                for (auto& event : soundbank->GetEvents())
                {
                    if (event.lookup())
                    {
                        for (auto& action : event->m_actions)
                        {
                            if (!action.m_destination.lookup())
                            {
                                continue;
                            }

                            sbk::engine::NodeBase* const nodeBase =
                                action.m_destination->try_convert_object<sbk::engine::NodeBase>();

                            assert(nodeBase);

                            nodesToSave.push_back(nodeBase);
                            nodeBase->gatherAllDescendants(nodesToSave);
                            nodeBase->gatherAllParents(nodesToSave);
                        }
                    }
                }

                if (nodesToSave.empty())
                {
                    continue;
                }

                std::vector<sbk::engine::Sound*> soundsToSave;
                std::vector<std::string> ecodedSoundPathsToSaveStrings;
                std::vector<const char*> encodedSoundPathsToSave;
                std::vector<ma_encoding_format> encodingFormats;

                for (auto& node : nodesToSave)
                {
                    if (node->getType() == sbk::engine::SoundContainer::type())
                    {
                        if (sbk::engine::SoundContainer* const soundContainer =
                                node->try_convert_object<sbk::engine::SoundContainer>())
                        {
                            if (sbk::engine::Sound* const sound = soundContainer->getSound())
                            {
                                soundsToSave.push_back(sound);
                                ecodedSoundPathsToSaveStrings.push_back(
                                    (m_projectConfig.encodedFolder() / sound->getEncodedSoundName()).string());
                                encodedSoundPathsToSave.push_back(ecodedSoundPathsToSaveStrings.back().c_str());
                                encodingFormats.push_back(ma_encoding_format_unknown);
                            }
                        }
                    }
                }

                sc_bank bank;
                sc_result initresult = sc_bank_init((m_projectConfig.buildFolder() / (std::string(soundbank->get_database_name()) + ".bnk")).string().c_str(), &bank);
                assert(initresult == MA_SUCCESS);

                sc_result buildResult = sc_bank_build(&bank, encodedSoundPathsToSave.data(), encodingFormats.data(), encodedSoundPathsToSave.size());
                assert(buildResult == MA_SUCCESS);

                sc_bank_uninit(&bank);
            }
        }
    }
}

void sbk::editor::project::saveSystem() const
{
    YAML::Emitter systemYaml;
    sbk::core::Serialization::Serializer::saveSystem(sbk::engine::system::get(), systemYaml);
    saveYAML(systemYaml, m_projectConfig.m_projectFolder / "system.yaml");
}

void sbk::editor::project::saveObjects() const
{
    if (sbk::core::database* const database = sbk::engine::system::get())
    {
        for (const std::weak_ptr<sbk::core::database_object>& object : database->get_all())
        {
            if (const std::shared_ptr<sbk::core::database_object> sharedObject = object.lock())
            {
                if (sharedObject->get_editor_hidden())
                {
                    continue;
                }

                YAML::Emitter yaml;
                sbk::core::Serialization::Serializer::saveObject(sharedObject.get(), yaml);

                const std::filesystem::path filePath = m_projectConfig.typeFolder(sharedObject->getType()) /
                                                       m_projectConfig.getIdFilename(sharedObject.get());
                std::filesystem::create_directories(filePath.parent_path());

                saveYAML(yaml, filePath);
            }
        }
    }
}

void sbk::editor::project::saveYAML(const YAML::Emitter& emitter, const std::filesystem::path& filePath) const
{
    std::ofstream outputStream(filePath);
    outputStream << emitter.c_str();
}