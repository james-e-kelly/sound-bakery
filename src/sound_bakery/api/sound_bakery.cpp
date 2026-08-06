#include "sound_bakery/sound_bakery.h"

#include "sound_bakery/api/engine_api.h"
#include "sound_bakery/core/error/result.h"
#include "sound_bakery/core/task/system_thread.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/runtime/runtime.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/voice.h"

template <class T>
static auto convert_id_to_pointer(sbk_id id) -> T*
{
    return reinterpret_cast<T*>(id);
}

sbk_status sbk_log(ma_log_level level, const char* message)
{
    // Can't do any logging inside the log function so just return the error codes

    if (message == NULL)
    {
        return SBK_ERR_INVALID_PARAMETER;
    }
    const sbk::engine::system* const system = sbk::engine::system::get();

    if (system == NULL)
    {
        return SBK_ERR_BAKERY_UNINITIALIZED;
    }

    switch (level)
    {
        case MA_LOG_LEVEL_DEBUG:
            system->get_logger()->log(spdlog::level::debug, message);
            TracyMessageC(message, strlen(message), 0xffffff);
            break;
        case MA_LOG_LEVEL_INFO:
            system->get_logger()->log(spdlog::level::info, message);
            TracyMessageC(message, strlen(message), 0xff4500);
            break;
        case MA_LOG_LEVEL_WARNING:
            system->get_logger()->log(spdlog::level::warn, message);
            TracyMessageC(message, strlen(message), 0xff0000);
            break;
        case MA_LOG_LEVEL_ERROR:
            system->get_logger()->log(spdlog::level::err, message);
            TracyMessageC(message, strlen(message), 0x8b0000);
            break;
    }

    return SBK_SUCCESS;
}

sbk_system_config sbk_system_config_init_default()
{
    sbk_system_config config;
    std::memset(&config, 0, sizeof(sbk_system_config));
    config.soundChefConfig = sc_system_config_init_default();
    config.commandQueueSize = 1024UL * 32UL;
    return config;
}

sbk_system_config sbk_system_config_init(const char* pluginPath)
{
    sbk_system_config config          = sbk_system_config_init_default();
    config.soundChefConfig.pluginPath = pluginPath;
    return config;
}

sbk_status sbk_system_create()
{
    ZoneScoped;

    return sbk::to_status(sbk::engine::system::create());
}

sbk_status sbk_system_init(sbk_system_config config)
{
    ZoneScoped;

    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        return sbk::to_status(system->init(config));
    }

    return SBK_ERR_BAKERY_UNINITIALIZED;
}

sbk_status sbk_system_update()
{
    ZoneScoped;

    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        return sbk::to_status(system->update());
    }

    return SBK_ERR_BAKERY_UNINITIALIZED;
}

sbk_status sbk_system_destroy()
{
    ZoneScoped;

    sbk::engine::system::destroy();
    return SBK_SUCCESS;
}

sbk_status sbk_system_load_soundbank(const char* soundbankFilePath, sbk_id* outSoundbankID)
{
    (void)outSoundbankID;
    
    ZoneScoped;

    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        sbk::load_soundbank_message message;
        std::strcpy(message.filename, soundbankFilePath);
        return system->get_command_queue().write_command(sbk::message_type::load_soundbank, message);
    }

    return SBK_ERR_BAKERY_UNINITIALIZED;
}

sbk_status sbk_system_post_event(sbk_id eventID, sbk_id gameObjectID)
{
    ZoneScoped;

    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        return system->get_command_queue().write_command(sbk::message_type::post_event, sbk::post_event_message{.eventID = eventID, .gameObjectID = gameObjectID});
    }

    return SBK_ERR_BAKERY_UNINITIALIZED;
}

sbk_status sbk_system_post_event_name(const char* eventName, sbk_id gameObjectID)
{
    ZoneScoped;

    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        sbk::post_event_name_message message{.gameObjectID = gameObjectID};
        std::strcpy(message.eventName, eventName);
        return system->get_command_queue().write_command(sbk::message_type::post_event_name, message);
    }

    return SBK_ERR_BAKERY_UNINITIALIZED;
}

sbk_status sbk_system_stop_all(sbk_id gameObjectID)
{
    ZoneScoped;

    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        return system->get_command_queue().write_command(sbk::message_type::stop_all, sbk::stop_all_message{.gameObjectID = gameObjectID});
    }

    return SBK_ERR_BAKERY_UNINITIALIZED;
}

sbk_status sbk_system_get_object_count(uint64_t* count)
{
    ZoneScoped;

    SBK_STATUS_CHECK(count != NULL, SBK_ERR_INVALID_PARAMETER);

    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        *count = system->get_database_object_count();
        return SBK_SUCCESS;
    }

    return SBK_ERR_BAKERY_UNINITIALIZED;
}

sbk_status sbk_system_get_object_info(uint64_t index, sbk_id* id, char* name, uint64_t nameSize, uint64_t* actualNameSize)
{
    ZoneScoped;

    SBK_STATUS_CHECK(name != NULL, SBK_ERR_INVALID_PARAMETER);
    SBK_STATUS_CHECK(nameSize > 0, SBK_ERR_INVALID_PARAMETER);
    SBK_STATUS_CHECK(actualNameSize != NULL, SBK_ERR_INVALID_PARAMETER);

    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        const std::shared_ptr<sbk::core::database_object> object = system->get_database_object_at(index).lock();
        SBK_STATUS_CHECK(object, SBK_ERR_BAKERY_OBJECT_NOT_FOUND);
        
        const sbk_id objectID        = object->get_database_id();
        const std::string objectName = object->get_database_name();

        *id = objectID;
        *actualNameSize = objectName.copy(name, nameSize);

        return SBK_SUCCESS;
    }

    return SBK_ERR_BAKERY_UNINITIALIZED;
}

namespace sbk::engine
{
    auto open_project(const std::filesystem::path& projectFile, sbk::core::sbk_log_callback_proc logCallback) -> sbk::result<void>
    {
        ZoneScoped;

        sbk::engine::system::destroy();

        const sbk::editor::project_configuration tempProject = sbk::editor::project_configuration(projectFile);

        SBK_TRYV(sbk::engine::system::create(tempProject.log_folder() / (std::string(tempProject.project_name()) + ".txt")));
        system* const system = sbk::engine::system::get();

        if (logCallback)
        {
            system->add_external_log(logCallback);
        }

        const std::string pluginFolder    = tempProject.plugin_folder().string();
        sbk_system_config systemConfig    = sbk_system_config_init(pluginFolder.c_str());
        systemConfig.singleThreadedUpdate = true;  // If we're opening a project, we are in the editor and must be single threaded

        SBK_TRYV(system->init(systemConfig));

        SBK_TRY(sbk::editor::project* project, system->create_project());
        return project->open_project(projectFile);
    }

    auto create_project(const std::filesystem::directory_entry& projectDirectory, std::string_view projectName) -> sbk::result<void>
    {
        ZoneScoped;

        const sbk::editor::project_configuration projectConfig(projectDirectory, projectName);

        SBK_TRYV(open_project(projectConfig.project_file(), nullptr));
        system* const system = sbk::engine::system::get();
        sbk::editor::project* const project = system->get_project();
        SBK_CHECK_MSG(project != nullptr, SBK_ERR_BAKERY, "Project variable was null. This should have been created");

        SBK_TRY(auto masterBus, project->create_database_object<sbk::engine::bus>());
        masterBus->set_object_name("Master Bus");
        masterBus->set_master_bus(true);

        return project->save_project();
    }

    auto post_container(sbk_id containerID, sbk_id gameObjectID) -> sbk::result<void>
    {
        ZoneScoped;

        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_CHECK(system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);

        //::post_container(system, containerID, gameObjectID);
        return sbk::make_error(SBK_ERR_NULL);   // turn off for now
    }

    auto get_game_object(sbk_id gameObjectID) -> std::weak_ptr<sbk::core::database_object>
    {
        ZoneScoped;

        std::weak_ptr<sbk::core::database_object> result;

        if (sbk::engine::system* const system = sbk::engine::system::get())
        {
            if (gameObjectID == 0)
            {
                if (const sbk::engine::runtime* const runtime = system->get_runtime())
                {
                    if (const auto listener = runtime->get_listener_game_object())
                    {
                        return system->try_find_database_object(listener->get_database_id());
                    }
                }
            }
            return system->try_find_database_object(gameObjectID);
        }

        return result;
    }
}  // namespace sbk::engine