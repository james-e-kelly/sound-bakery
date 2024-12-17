#include "project.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"
#include "sound_chef/sound_chef_bank.h"
#include "sound_chef/sound_chef_encoder.h"

bool sbk::editor::project::open_project(const std::filesystem::path& projectFile)
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

    m_projectConfig = project_configuration(projectFile);

    loadObjects();
    loadSystem();
    loadSounds();

    createPreviewContainer();

    return true;
}

void sbk::editor::project::save_project() const
{
    saveSystem();
    saveObjects();
    build_soundbanks();  // temp for testing
}

void sbk::editor::project::encode_all_media() const
{
    std::shared_ptr<concurrencpp::thread_pool_executor> threadPool = sbk::engine::system::get()->background_executor();

    if (sbk::core::object_tracker* const objectTracker = sbk::engine::system::get())
    {
        for (sbk::core::object* const soundObject : objectTracker->get_objects_of_type(sbk::engine::sound::type()))
        {
            if (sbk::engine::sound* const sound = soundObject->try_convert_object<sbk::engine::sound>())
            {
                const std::filesystem::path encodedSoundFile =
                    m_projectConfig.encoded_folder() / (std::to_string(sound->get_database_id()) + ".ogg");
                std::filesystem::create_directories(encodedSoundFile.parent_path());

                const sc_encoder_config encoderConfig = sc_encoder_config_init(sc_encoding_format_vorbis, ma_format_f32,
                                                                               0, ma_standard_sample_rate_48000, 8);

                std::filesystem::path soundPath = sound->getSoundName();

                if (!std::filesystem::exists(soundPath))
                {
                    soundPath = m_projectConfig.source_folder() / soundPath;
                }

                threadPool->post(
                    [sound, encoderConfig, encodedSoundFile, soundPath]
                    {
                        sc_result result = sc_encoder_write_from_file(
                            soundPath.string().c_str(), encodedSoundFile.string().c_str(), &encoderConfig);
                        BOOST_ASSERT(result == MA_SUCCESS);

                        concurrencpp::resume_on(sbk::engine::system::get()->get_game_thread_executer());

                        sound->setEncodedSoundName(encodedSoundFile.string());
                    });
            }
        }
    }
}

const sbk::editor::project_configuration& sbk::editor::project::get_config() const { return m_projectConfig; }

std::weak_ptr<sbk::engine::sound_container> sbk::editor::project::get_preview_container() const
{
    return m_previewSoundContainer;
}

void sbk::editor::project::loadSounds()
{
    for (const std::filesystem::directory_entry& p :
         std::filesystem::recursive_directory_iterator(m_projectConfig.source_folder()))
    {
        if (p.is_regular_file() && p.path().filename().string()[0] != '.')
        {
            const std::filesystem::path filename = p.path().filename();

            if (sbk::core::database* const database = sbk::engine::system::get())
            {
                if (database->try_find(filename.stem().string()).expired())
                {
                    if (const std::shared_ptr<sbk::core::database_object> createdSound =
                            create_database_object<sbk::engine::sound>())
                    {
                        createdSound->set_database_name(filename.stem().string());

                        if (sbk::engine::sound* const castedSound =
                                sbk::reflection::cast<sbk::engine::sound*, sbk::core::database_object*>(
                                    createdSound.get()))
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
         std::filesystem::directory_iterator(m_projectConfig.project_folder()))
    {
        if (p.path().extension() == ".yaml")
        {
            YAML::Node node = YAML::LoadFile(p.path().string());
            sbk::core::serialization::yaml_serializer::loadSystem(sbk::engine::system::get(), node);
        }
    }
}

void sbk::editor::project::loadObjects()
{
    const std::vector<std::filesystem::path> loadPaths{m_projectConfig.object_folder()};

    for (const std::filesystem::path& path : loadPaths)
    {
        std::filesystem::create_directories(path);

        for (const std::filesystem::directory_entry& directoryEntry :
             std::filesystem::recursive_directory_iterator(path))
        {
            if (directoryEntry.is_regular_file() && directoryEntry.path().extension() == ".yaml")
            {
                YAML::Node node                           = YAML::LoadFile(directoryEntry.path().string());
                std::shared_ptr<sbk::core::object> object = load_object(node);
            }
        }
    }
}

void sbk::editor::project::createPreviewContainer()
{
    if (auto previewContainer = create_database_object<sbk::engine::sound_container>())
    {
        previewContainer->set_database_name("Preview Node");
        previewContainer->set_editor_hidden(true);

        m_previewSoundContainer = previewContainer;
    }
}

void sbk::editor::project::build_soundbanks() const
{
    const std::unordered_set<sbk::core::object*> soundbankObjects =
        sbk::engine::system::get()->get_objects_of_category(SB_CATEGORY_BANK);

    for (const auto& soundbankObject : soundbankObjects)
    {
        if (const sbk::engine::soundbank* const soundbank = soundbankObject->try_convert_object<sbk::engine::soundbank>())
        {
            /*YAML::Emitter soundbankEmitter;
            SB::Core::Serialization::yaml_serializer::packageSoundbank(soundbank, soundbankEmitter);

            const std::filesystem::path filePath =
                m_projectConfig.objectFolder() / m_projectConfig.getIdFilename(soundbank, std::string(".bank"));

            saveYAML(soundbankEmitter, filePath);*/

            sbk::engine::soundbank_dependencies soundbankDependencies = soundbank->gather_dependencies();

            sc_bank bank;
            const sc_result initresult =
                sc_bank_init(&bank,
                             (m_projectConfig.build_folder() / (std::string(soundbank->get_database_name()) + ".bnk"))
                                 .string()
                                 .c_str(),
                             MA_OPEN_MODE_WRITE);
            BOOST_ASSERT(initresult == MA_SUCCESS);

            const sc_result buildResult = sc_bank_build(&bank, soundbankDependencies.encodedSoundPaths.data(),
                                                        soundbankDependencies.encodingFormats.data(),
                                                        soundbankDependencies.encodedSoundPaths.size());
            BOOST_ASSERT(buildResult == MA_SUCCESS);

            sc_bank_uninit(&bank);
        }
    }
}

void sbk::editor::project::saveSystem() const
{
    YAML::Emitter systemYaml;
    sbk::core::serialization::yaml_serializer::saveSystem(sbk::engine::system::get(), systemYaml);
    saveYAML(systemYaml, m_projectConfig.project_folder() / "system.yaml");
}

void sbk::editor::project::saveObjects() const
{
    for (const std::weak_ptr<sbk::core::database_object>& object : sbk::engine::system::get()->get_all())
    {
        if (const std::shared_ptr<sbk::core::database_object> sharedObject = object.lock())
        {
            if (sharedObject->get_editor_hidden())
            {
                continue;
            }

            std::filesystem::path filePath = m_projectConfig.type_folder(sharedObject->get_object_type()) /
                                                   m_projectConfig.get_filename_for_id(sharedObject.get());

            YAML::Emitter yaml;
            sbk::core::serialization::yaml_serializer::saveObject(sharedObject.get(), yaml);

            std::filesystem::create_directories(filePath.parent_path());

            saveYAML(yaml, filePath);

            sbk::core::serialization::text_serializer textSerializer;
            textSerializer.save_object(sharedObject, filePath.replace_extension("txt"));

            sbk::core::serialization::xml_serializer xmlSerializer;
            xmlSerializer.save_object(sharedObject, filePath.replace_extension("xml"));

            sbk::core::serialization::binary_serializer binarySerializer;
            binarySerializer.save_object(sharedObject, filePath.replace_extension("bnk"));
        }
    }
}

void sbk::editor::project::saveYAML(const YAML::Emitter& emitter, const std::filesystem::path& filePath) const
{
    std::ofstream outputStream(filePath);
    outputStream << emitter.c_str();
}