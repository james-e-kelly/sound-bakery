#include "project.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"
#include "sound_chef/sound_chef_bank.h"
#include "sound_chef/sound_chef_encoder.h"

auto sbk::editor::project::open_project(const std::filesystem::path& projectFile) -> concurrencpp::result<bool>
{
    const std::filesystem::path projectFileCopy = projectFile;

    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_thread_pool_executor());

    const concurrencpp::scoped_async_lock projectLock =
        co_await m_projectLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

    if (projectFileCopy.empty())
    {
        co_return false;
    }

    if (!std::filesystem::exists(projectFileCopy))
    {
        co_return false;
    }

    m_projectConfig = project_configuration(projectFileCopy);

    co_await load_objects();

    SBK_INFO("Loaded objects");

    concurrencpp::result<void> loadSystemResult = sbk::engine::system::get()->get_background_thread_executor()->submit([this]() { load_system(); });
    concurrencpp::result<void> loadSoundsResult = sbk::engine::system::get()->get_background_thread_executor()->submit([this]() { load_sounds(); });

    co_await concurrencpp::when_all(sbk::engine::system::get()->get_thread_pool_executor(), std::move(loadSystemResult), std::move(loadSoundsResult));

    create_preview_container();

    co_return true;
}

void sbk::editor::project::save_project() const
{
    save_system();
    save_objects();
}

auto sbk::editor::project::encode_all_media() const -> concurrencpp::result<void>
{
    std::vector<concurrencpp::result<void>> encodeTasks;

    for (sbk::core::object* const soundObject : co_await sbk::engine::system::get()->get_objects_of_type(sbk::engine::sound::type()))
    {
        if (sbk::engine::sound* const sound = soundObject->try_convert_object<sbk::engine::sound>())
        {
            encodeTasks.emplace_back(sbk::engine::system::get()->get_background_thread_executor()->submit(
                [sound = sound, this]() 
                {
                    const std::filesystem::path encodedSoundFile = m_projectConfig.encoded_folder() / (std::to_string(sound->get_database_id()) + ".ogg");
                    std::filesystem::create_directories(encodedSoundFile.parent_path());

                    std::filesystem::path soundPath = sound->get_sound_name();

                    if (!std::filesystem::exists(soundPath))
                    {
                        soundPath = m_projectConfig.source_folder() / soundPath;
                    }

                    const sc_encoder_config encoderConfig = sc_encoder_config_init(
                        sc_encoding_format_vorbis, ma_format_f32, 0, ma_standard_sample_rate_48000, 8);

                    sbk_result result = sc_encoder_write_from_file(
                        soundPath.string().c_str(), encodedSoundFile.string().c_str(), &encoderConfig);
                    BOOST_ASSERT(result == MA_SUCCESS);

                    sound->set_encoded_sound_name(encodedSoundFile.string());
                }));
        }
    }

    co_await concurrencpp::when_all(sbk::engine::system::get()->get_background_thread_executor(), encodeTasks.begin(), encodeTasks.end());
}

const sbk::editor::project_configuration& sbk::editor::project::get_config() const { return m_projectConfig; }

std::weak_ptr<sbk::engine::sound_container> sbk::editor::project::get_preview_container() const
{
    return m_previewSoundContainer;
}

auto sbk::editor::project::load_sounds() -> concurrencpp::result<void>
{
    std::vector<concurrencpp::result<void>> loadTasks;

    for (const std::filesystem::directory_entry& p :
         std::filesystem::recursive_directory_iterator(m_projectConfig.source_folder()))
    {
        if (p.is_regular_file() && p.path().filename().string()[0] != '.')
        {
            loadTasks.emplace_back(load_single_sound(this, p.path()));
        }
    }

    co_await concurrencpp::when_all(sbk::engine::system::get()->get_thread_pool_executor(), loadTasks.begin(), loadTasks.end());
}

auto sbk::editor::project::load_system() -> concurrencpp::result<void>
{
    for (const std::filesystem::directory_entry& p :
         std::filesystem::directory_iterator(m_projectConfig.project_folder()))
    {
        if (p.path().extension() == ".yaml")
        {
            sbk::core::serialization::yaml_serializer yamlSerializer;
            co_await yamlSerializer.load_object<sbk::core::serialization::serialized_system>(sbk::engine::system::get(), p.path());
        }
    }
}

auto sbk::editor::project::load_objects() -> concurrencpp::result<void>
{
    const std::vector<std::filesystem::path> loadPaths{m_projectConfig.object_folder()};

    std::vector<concurrencpp::result<void>> loadingTasks;

    for (const std::filesystem::path& path : loadPaths)
    {
        std::filesystem::create_directories(path);

        for (const std::filesystem::directory_entry& directoryEntry :
             std::filesystem::recursive_directory_iterator(path))
        {
            if (directoryEntry.is_regular_file() && directoryEntry.path().extension() == ".yaml")
            {
                loadingTasks.emplace_back(load_single_object(this, directoryEntry.path()));
            }
        }
    }

    for (auto& result : loadingTasks)
    {
        co_await result;
    }
}

auto sbk::editor::project::load_single_sound(sbk::editor::project* project, std::filesystem::path filePath) -> concurrencpp::result<void> 
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_background_thread_executor());
    const sbk::core::database_name objectName(sbk::engine::sound::type().get_name().data(), filePath.stem().string());
    const std::weak_ptr<sbk::core::database_object> existingSound =
        co_await sbk::engine::system::get()->try_find_database_object(objectName);

    if (existingSound.expired())
    {
        if (const std::shared_ptr<sbk::core::database_object> createdSound =
                co_await project->create_database_object<sbk::engine::sound>())
        {
            createdSound->set_object_name(filePath.stem().string());

            if (sbk::engine::sound* const castedSound =
                    sbk::reflection::cast<sbk::engine::sound*, sbk::core::database_object*>(createdSound.get()))
            {
                castedSound->set_sound_name(filePath.string());
            }
        }
    }
}

auto sbk::editor::project::load_single_object(sbk::editor::project* project, std::filesystem::path filePath) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_background_thread_executor());
    sbk::core::serialization::yaml_serializer yamlSerializer;
    co_await yamlSerializer.load_object<sbk::core::serialization::serialized_standalone_object>(project, filePath);
}

void sbk::editor::project::create_preview_container()
{
    if (auto previewContainer = create_database_object<sbk::engine::sound_container>().get())
    {
        previewContainer->set_object_name("Preview Node");
        previewContainer->set_editor_hidden(true);

        m_previewSoundContainer = previewContainer;
    }
}

auto sbk::editor::project::build_soundbanks() -> concurrencpp::result<void>
{
    std::vector<sbk::core::object*> soundbankObjects = co_await sbk::engine::system::get()->get_objects_of_category(SB_CATEGORY_BANK);

    std::shared_ptr<sbk::engine::soundbank> initSoundbank = co_await create_database_object<sbk::engine::soundbank>();
    initSoundbank->set_editor_hidden(true);
    initSoundbank->set_object_name(m_projectConfig.initBankName);
    initSoundbank->set_init_soundbank(true);
    initSoundbank->set_lookup_soundbank(true);
    co_await remove_reference_to_object(initSoundbank);   // Will delete when going out of this scope

    soundbankObjects.push_back(initSoundbank.get());

    std::vector<concurrencpp::result<void>> buildTasks;
    buildTasks.reserve(soundbankObjects.size());

    for (const auto& soundbankObject : soundbankObjects)
    {
        if (sbk::engine::soundbank* const soundbank = soundbankObject->try_convert_object<sbk::engine::soundbank>())
        {
            buildTasks.emplace_back(sbk::engine::system::get()->get_background_thread_executor()->submit(
                [soundbank = std::move(soundbank), this]()
                {
                    std::shared_ptr<sbk::core::database_object> sharedDatabaseObject = soundbank->casted_shared_from_this<sbk::core::database_object>();

                    sbk::core::serialization::binary_serializer binarySerializer;
                    binarySerializer.save_database_object<sbk::core::serialization::serialized_soundbank>(
                        sharedDatabaseObject,
                        m_projectConfig.build_folder() / (std::string(soundbank->get_object_name()) + (std::string(m_projectConfig.outputBankExtensionWithDot))));

                    sbk::core::serialization::yaml_serializer yamlSerializer;
                    yamlSerializer.save_database_object<sbk::core::serialization::serialized_soundbank>(
                        sharedDatabaseObject, m_projectConfig.build_folder() / ((std::string(soundbank->get_object_name()) + ".yaml")));
                }));
        }
    }

    co_await concurrencpp::when_all(sbk::engine::system::get()->get_thread_pool_executor(), buildTasks.begin(), buildTasks.end());
}

auto sbk::editor::project::save_system() const -> concurrencpp::result<void>
{
    const concurrencpp::scoped_async_lock projectLock = co_await m_projectLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

    sbk::core::serialization::yaml_serializer yamlSerializer;
    co_await yamlSerializer.save_system(m_projectConfig.project_folder() / "system.yaml");
}

auto sbk::editor::project::save_objects() const -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_background_thread_executor());

    const concurrencpp::scoped_async_lock projectLock = co_await m_projectLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

    for (const std::weak_ptr<sbk::core::database_object>& object : co_await sbk::engine::system::get()->get_all_database_objects())
    {
        if (std::shared_ptr<sbk::core::database_object> sharedObject = object.lock())
        {
            if (sharedObject->get_editor_hidden())
            {
                continue;
            }

            std::filesystem::path filePath = m_projectConfig.type_folder(sharedObject->get_object_type()) /
                                                   m_projectConfig.get_filename_for_id(sharedObject.get());

            std::filesystem::create_directories(filePath.parent_path());

            sbk::core::serialization::yaml_serializer yamlSerializer;
            co_await yamlSerializer.save_database_object<sbk::core::serialization::serialized_standalone_object>(sharedObject, filePath.replace_extension("yaml"));
        }
    }
}