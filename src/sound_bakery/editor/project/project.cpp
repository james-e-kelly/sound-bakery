#include "project.h"

#include "sound_bakery/error/result.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"
#include "sound_bakery/task/task.h"

#include "sound_chef/sound_chef_bank.h"
#include "sound_chef/sound_chef_encoder.h"

auto sbk::editor::project::open_project(const std::filesystem::path& projectFile) -> sbk::result<void>
{
    SBK_CHECK(!projectFile.empty(), SBK_ERR_INVALID_PARAMETER);
    SBK_CHECK_MSG(std::filesystem::exists(projectFile), SBK_ERR_INVALID_FILE, "project file '{}' does not exist", projectFile.string());

    m_projectConfig = project_configuration(projectFile);

    SBK_TRYV(load_objects());
    SBK_TRYV(load_system());
    SBK_TRYV(load_sounds());

    SBK_TRYV(create_preview_container());

    return sbk::ok();
}

auto sbk::editor::project::save_project() const -> sbk::result<void>
{
    SBK_TRYV(save_system());
    SBK_TRYV(save_objects());
    return sbk::ok();
}

namespace
{
    // The per-sound encode, as a named fire-and-forget coroutine. Everything it needs is passed BY
    // VALUE, so it stays valid across the co_await thread hops (a coroutine's parameters live in its
    // frame; a temporary lambda's captures would dangle after the first suspension).
    auto encode_sound(sbk::engine::sound* sound,
                      sc_encoder_config encoderConfig,
                      std::filesystem::path source,
                      std::filesystem::path destination,
                      std::shared_ptr<sbk::executor> worker,
                      std::shared_ptr<sbk::executor> systemThread) -> sbk::detached_task
    {
        // Do the heavy encode off the calling thread.
        co_await worker->schedule();

        if (const sbk_status encodeResult = sc_encoder_write_from_file(source.string().c_str(), destination.string().c_str(), &encoderConfig); encodeResult != SBK_SUCCESS)
        {
            sbk::log_error(encodeResult, "sc_encoder_write_from_file");  //< Detached task; log and stop.
            co_return;
        }

        // Hop to the system thread to mutate the object.
        co_await systemThread->schedule();
        sound->set_encoded_sound_name(destination.string());
    }
}  // namespace

auto sbk::editor::project::encode_all_media() const -> void
{
    sbk::engine::system* const system = sbk::engine::system::get();
    if (system == nullptr)
    {
        return;
    }

    std::shared_ptr<sbk::executor> workerThread = system->get_worker_executer();
    std::shared_ptr<sbk::executor> systemThread = system->get_system_executer();

    for (sbk::core::object* const soundObject : system->get_objects_of_type(sbk::engine::sound::type()))
    {
        if (sbk::engine::sound* const sound = soundObject->try_convert_object<sbk::engine::sound>())
        {
            const std::filesystem::path encodedSoundFile =
                m_projectConfig.encoded_folder() / (std::to_string(sound->get_database_id()) + ".ogg");
            std::filesystem::create_directories(encodedSoundFile.parent_path());

            const sc_encoder_config encoderConfig = sc_encoder_config_init(sc_encoding_format_vorbis, ma_format_f32,
                                                                           0, ma_standard_sample_rate_48000, 8);

            std::filesystem::path soundPath = sound->get_sound_name();

            if (!std::filesystem::exists(soundPath))
            {
                soundPath = m_projectConfig.source_folder() / soundPath;
            }

            // Fire and forget: the coroutine owns itself and hops onto the worker thread on its own.
            encode_sound(sound, encoderConfig, soundPath, encodedSoundFile, workerThread, systemThread);
        }
    }
}

auto sbk::editor::project::get_config() const -> const sbk::editor::project_configuration& { return m_projectConfig; }

auto sbk::editor::project::get_preview_container() const -> std::weak_ptr<sbk::engine::sound_container>
{
    return m_previewSoundContainer;
}

auto sbk::editor::project::load_sounds() -> sbk::result<void>
{
    for (const std::filesystem::directory_entry& p : std::filesystem::recursive_directory_iterator(m_projectConfig.source_folder()))
    {
        if (p.is_regular_file() && p.path().filename().string()[0] != '.')
        {
            const std::filesystem::path filename = p.path().filename();

            if (const sbk::core::database* const database = sbk::engine::system::get())
            {
                if (database->try_find_database_object(sbk::core::database_name(sbk::engine::sound::type().get_name().data(), filename.stem().string())).expired())
                {
                    SBK_TRY(const auto createdSound, create_database_object<sbk::engine::sound>());

                    createdSound->set_object_name(filename.stem().string());
                    createdSound->set_sound_name(p.path().string());
                }
            }
        }
    }
    return sbk::ok();
}

auto sbk::editor::project::load_system() -> sbk::result<void>
{
    //int foundSystemFiles = 0;

    for (const std::filesystem::directory_entry& p : std::filesystem::directory_iterator(m_projectConfig.project_folder()))
    {
        if (p.path().extension() == ".yaml")
        {
            /// @todo Determine if we still need to load the system object
            //sbk::core::serialization::yaml_serializer yamlSerializer;
            //SBK_TRYV(yamlSerializer.load_object<sbk::core::serialization::serialized_system>(sbk::engine::system::get(), p.path()));  //< Best-effort; failures log at origin.
            //SBK_CHECK_MSG(++foundSystemFiles == 1, SBK_ERR_BAKERY, "Multiple system files were found");
        }
    }
    return sbk::ok();
}

auto sbk::editor::project::load_objects() -> sbk::result<void>
{
    const std::vector<std::filesystem::path> loadPaths{m_projectConfig.object_folder()};

    for (const std::filesystem::path& path : loadPaths)
    {
        std::filesystem::create_directories(path);

        for (const std::filesystem::directory_entry& directoryEntry : std::filesystem::recursive_directory_iterator(path))
        {
            if (directoryEntry.is_regular_file() && directoryEntry.path().extension() == ".yaml")
            {
                sbk::core::serialization::yaml_serializer yamlSerializer;
                SBK_TRYV(yamlSerializer.load_object<sbk::core::serialization::serialized_standalone_object>(this, directoryEntry.path()));  //< Best-effort; failures log at origin.
            }
        }
    }

    return sbk::ok();
}

auto sbk::editor::project::create_preview_container() -> sbk::result<void>
{
    SBK_TRY(auto previewContainer, create_database_object<sbk::engine::sound_container>());

    previewContainer->set_object_name("Preview Node");
    previewContainer->set_editor_hidden(true);

    m_previewSoundContainer = previewContainer;

    return sbk::ok();
}

auto sbk::editor::project::build_soundbanks() -> sbk::result<void>
{
    std::unordered_set<sbk::core::object*> soundbankObjects = sbk::engine::system::get()->get_objects_of_category(SB_CATEGORY_BANK);

    SBK_TRY(auto initSoundbank, create_database_object<sbk::engine::soundbank>());

    initSoundbank->set_editor_hidden(true);
    initSoundbank->set_object_name(m_projectConfig.initBankName);
    initSoundbank->set_init_soundbank(true);
    initSoundbank->set_lookup_soundbank(true);
    remove_object(initSoundbank);  // Will delete when going out of this scope

    soundbankObjects.insert(initSoundbank.get());

    for (const auto& soundbankObject : soundbankObjects)
    {
        if (sbk::engine::soundbank* const soundbank = soundbankObject->try_convert_object<sbk::engine::soundbank>())
        {
            std::shared_ptr<sbk::core::database_object> sharedDatabaseObject = soundbank->casted_shared_from_this<sbk::core::database_object>();

            sbk::core::serialization::binary_serializer binarySerializer;
            (void)binarySerializer.save_database_object<sbk::core::serialization::serialized_soundbank>(
                sharedDatabaseObject,
                m_projectConfig.build_folder() / (std::string(soundbank->get_object_name()) + (std::string(m_projectConfig.outputBankExtensionWithDot))));

            sbk::core::serialization::yaml_serializer yamlSerializer;
            (void)yamlSerializer.save_database_object<sbk::core::serialization::serialized_soundbank>(
                sharedDatabaseObject, m_projectConfig.build_folder() / ((std::string(soundbank->get_object_name()) + ".yaml")));
        }
    }

    return sbk::ok();
}

auto sbk::editor::project::save_system() const -> sbk::result<void>
{
    sbk::core::serialization::yaml_serializer yamlSerializer;
    return yamlSerializer.save_system(m_projectConfig.project_folder() / "system.yaml");
}

auto sbk::editor::project::save_objects() const -> sbk::result<void>
{
    for (const std::weak_ptr<sbk::core::database_object>& object : sbk::engine::system::get()->get_all_database_objects())
    {
        if (std::shared_ptr<sbk::core::database_object> sharedObject = object.lock())
        {
            if (sharedObject->get_editor_hidden())
            {
                continue;
            }

            std::filesystem::path filePath = m_projectConfig.type_folder(sharedObject->get_object_type()) / m_projectConfig.get_filename_for_id(sharedObject.get());

            std::filesystem::create_directories(filePath.parent_path());

            /*sbk::core::serialization::text_serializer textSerializer;
            textSerializer.save_database_object(sharedObject, filePath.replace_extension("txt"));

            sbk::core::serialization::xml_serializer xmlSerializer;
            xmlSerializer.save_database_object(sharedObject, filePath.replace_extension("xml"));

            sbk::core::serialization::binary_serializer binarySerializer;
            binarySerializer.save_database_object(sharedObject, filePath.replace_extension("bnk"));*/

            sbk::core::serialization::yaml_serializer yamlSerializer;
            SBK_TRYV(yamlSerializer.save_database_object<sbk::core::serialization::serialized_standalone_object>(sharedObject, filePath.replace_extension("yaml")));
        }
    }

    return sbk::ok();
}