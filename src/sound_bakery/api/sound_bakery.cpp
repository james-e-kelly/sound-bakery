#include "sound_bakery/sound_bakery.h"

#include "sound_bakery/api/engine_api.h"
#include "sound_bakery/error/result.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/voice.h"

template<class T>
static auto convert_id_to_pointer(sbk_id id) -> T*
{
    return reinterpret_cast<T*>(id);
}

namespace
{
    using namespace sbk::engine;

    /**
     * @brief Runs a public-API body and converts any escaping exception into a sbk_status.
     *
     * A C++ exception propagating out of an `extern "C"` function is undefined behaviour, so every
     * public sbk_* entry point funnels its body through here.
     * 
     * TODO: Compile with -fno-exceptions and remove all code that throws to remove the need for catching exceptions
     */
    template <class body_fn>
    auto c_api_guard(body_fn&& body) -> sbk_status
    {
        try
        {
            return body();
        }
        catch (const std::exception& exception)
        {
            sbk::log_error(SBK_ERR_SYSTEM, exception.what());
            return SBK_ERR_SYSTEM;
        }
        catch (...)
        {
            sbk::log_error(SBK_ERR_SYSTEM, "unknown exception reached the C API boundary");
            return SBK_ERR_SYSTEM;
        }
    }

    auto play_container(game_object* gameObject, container* container) -> sbk::result<void>
    {
        SBK_CHECK(gameObject != nullptr, SBK_ERR_INVALID_PARAMETER);
        SBK_CHECK(container != nullptr, SBK_ERR_INVALID_PARAMETER);

        SBK_TRY(auto voice, gameObject->create_runtime_object<sbk::engine::voice>());
        return voice->play_container(container);
    }

    auto stop_container(game_object* gameObject, container* container) -> sbk::result<void>
    {
        SBK_CHECK(gameObject != nullptr, SBK_ERR_INVALID_PARAMETER);
        SBK_CHECK(container != nullptr, SBK_ERR_INVALID_PARAMETER);

        for (auto iter = gameObject->get_objects().begin(); iter != gameObject->get_objects().end(); ++iter)
        {
            if (const sbk::engine::voice* const voice = iter->get()->try_convert_object<sbk::engine::voice>())
            {
                if (voice->playing_container(container))
                {
                    gameObject->remove_object(*iter);
                    break;
                }
            }
        }
        return sbk::ok();
    }

    auto dispatch_event(game_object* gameObject, event* event) -> sbk::result<void>
    {
        ZoneScoped;

        SBK_CHECK(gameObject != nullptr, SBK_ERR_INVALID_PARAMETER);
        SBK_CHECK(event != nullptr, SBK_ERR_INVALID_PARAMETER);

        SBK_INFO("Posting Event");

        for (const action& action : event->m_actions)
        {
            SBK_CHECK(action.m_type != action_type::invalid, SBK_ERR_BAKERY);
            SBK_CHECK(action.m_type != action_type::num, SBK_ERR_BAKERY);

            sbk::engine::container* container           = nullptr;
            sbk::engine::event* childEvent              = nullptr;
            sbk::engine::game_object* targetGameObject  = nullptr;

            if (const auto destination = action.m_destination.shared())
            {
                container        = destination->try_convert_object<sbk::engine::container>();
                childEvent       = destination->try_convert_object<sbk::engine::event>();
                targetGameObject = destination->try_convert_object<sbk::engine::game_object>();
            }

            switch (action.m_type)
            {
                case action_type::play:
                    if (container)
                    {
                        SBK_TRYV(play_container(gameObject, container));
                    }
                    else if (childEvent)
                    {
                        SBK_TRYV(dispatch_event(gameObject, childEvent));
                    }
                    break;
                case action_type::stop:
                    if (container)
                    {
                        SBK_TRYV(stop_container(gameObject, container));
                    }
                    else if (targetGameObject)
                    {
                        targetGameObject->remove_all();  //< Assuming a game object only ever owns voices
                    }
                    break;
                default:
                    break;
            }
        }
        return sbk::ok();
    }
}  // namespace

sbk_status sbk_log(ma_log_level level, const char* message)
{
    SBK_STATUS_CHECK(message != NULL, SBK_ERR_INVALID_PARAMETER);

    if (const sbk::engine::system* const system = sbk::engine::system::get())
    {
        switch (level)
        {
            case MA_LOG_LEVEL_DEBUG:
                system->get_logger()->log(spdlog::level::debug, message);
                TracyMessageC(message, sizeof(message), 0xffffff);
                break;
            case MA_LOG_LEVEL_INFO:
                system->get_logger()->log(spdlog::level::info, message);
                TracyMessageC(message, sizeof(message), 0xff4500);
                break;
            case MA_LOG_LEVEL_WARNING:
                system->get_logger()->log(spdlog::level::warn, message);
                TracyMessageC(message, sizeof(message), 0xff0000);
                break;
            case MA_LOG_LEVEL_ERROR:
                system->get_logger()->log(spdlog::level::err, message);
                TracyMessageC(message, sizeof(message), 0x8b0000);
                break;
        }

    }

    return SBK_SUCCESS;
}

sbk_system_config sbk_system_config_init_default()
{
    sbk_system_config config;
    std::memset(&config, 0, sizeof(sbk_system_config));
    config.soundChefConfig = sc_system_config_init_default();
    return config;
}

sbk_system_config sbk_system_config_init(const char* pluginPath)
{
    sbk_system_config config           = sbk_system_config_init_default();
    config.soundChefConfig.pluginPath = pluginPath;
    return config;
}

sbk_status sbk_system_create()
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status { return sbk::to_status(sbk::engine::system::create()); });
}

sbk_status sbk_system_init(sbk_system_config config)
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status
    {
        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_STATUS_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);
        return sbk::to_status(system->init(config));
    });
}

sbk_status sbk_system_update()
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status
    {
        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_STATUS_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);
        return sbk::to_status(system->update());
    });
}

sbk_status sbk_system_destroy()
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status
    {
        sbk::engine::system::destroy();
        return SBK_SUCCESS;
    });
}

sbk_status sbk_system_get_object_count(uint64_t* count)
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status
    {
        SBK_STATUS_CHECK(count != NULL, SBK_ERR_INVALID_PARAMETER);
        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_STATUS_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);

        *count = system->get_database_object_count();
        return SBK_SUCCESS;
    });
}

sbk_status sbk_system_get_object_info(uint64_t index, sbk_id* id, char* name, uint64_t nameSize, uint64_t* actualNameSize)
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status
    {
        SBK_STATUS_CHECK(name != NULL, SBK_ERR_INVALID_PARAMETER);
        SBK_STATUS_CHECK(nameSize > 0, SBK_ERR_INVALID_PARAMETER);
        SBK_STATUS_CHECK(actualNameSize != NULL, SBK_ERR_INVALID_PARAMETER);

        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_STATUS_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);

        if (const std::shared_ptr<sbk::core::database_object> object = system->get_database_object_at(index).lock())
        {
            const sbk_id objectID        = object->get_database_id();
            const std::string objectName = object->get_database_name();

            *id = objectID;
            *actualNameSize = objectName.copy(name, nameSize);
            return SBK_SUCCESS;
        }
        return SBK_ERR_BAKERY_OBJECT_NOT_FOUND;
    });
}

sbk_status sbk_system_load_soundbank(const char* soundbankFilePath, sbk_soundbank** outSoundbank)
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status
    {
        SBK_STATUS_CHECK(soundbankFilePath != NULL, SBK_ERR_INVALID_PARAMETER);
        SBK_STATUS_CHECK(outSoundbank != NULL, SBK_ERR_INVALID_PARAMETER);

        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_STATUS_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);
        SBK_STATUS_CHECK(std::filesystem::exists(soundbankFilePath), SBK_ERR_INVALID_FILE);

        sbk::core::serialization::binary_serializer binarySerializer;
        const sbk::result<sbk_id> soundbankID = binarySerializer.load_object<sbk::core::serialization::serialized_soundbank>(system, soundbankFilePath);
        SBK_STATUS_CHECK_MSG(soundbankID.has_value(), SBK_ERR_BAKERY_SERIALIZATION, "failed to load soundbank '{}'", soundbankFilePath);

        *outSoundbank = convert_id_to_pointer<sbk_soundbank>(soundbankID.value());
        return SBK_SUCCESS;
    });
}

sbk_status sbk_system_post_event(const char* eventName, sbk_id gameObjectID)
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status
    {
        SBK_STATUS_CHECK(eventName != NULL, SBK_ERR_INVALID_PARAMETER);

        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_STATUS_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);

        std::weak_ptr<sbk::core::database_object> event =
            system->try_find_database_object(sbk::core::database_name(eventName));
        SBK_STATUS_CHECK_MSG(!event.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND, "no event named '{}'", eventName);

        std::weak_ptr<sbk::core::database_object> gameObject = sbk::engine::get_game_object(gameObjectID);
        SBK_STATUS_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

        system->get_system_thread_executer()->post(
            [event, gameObject]()
            {
                if (std::shared_ptr<sbk::engine::game_object> sharedGameObject =
                        std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock()))
                {
                    if (std::shared_ptr<sbk::engine::event> sharedEvent =
                            std::static_pointer_cast<sbk::engine::event>(event.lock()))
                    {
                        (void)dispatch_event(sharedGameObject.get(), sharedEvent.get());
                    }
                }
            });

        return SBK_SUCCESS;
    });
}

sbk_status sbk_system_stop_all(sbk_id gameObjectID)
{
    ZoneScoped;
    return c_api_guard([&]() -> sbk_status
    {
        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_STATUS_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);

        std::weak_ptr<sbk::core::database_object> gameObject = sbk::engine::get_game_object(gameObjectID);
        SBK_STATUS_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

        system->get_system_thread_executer()->post(
            [gameObject]()
            {
                if (std::shared_ptr<sbk::engine::game_object> sharedGameObject =
                        std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock()))
                {
                    sharedGameObject->remove_all();  //< Assuming a game object only ever owns voices
                }
            });

        return SBK_SUCCESS;
    });
}

namespace sbk::engine
{
    auto post_container(sbk_id containerID, sbk_id gameObjectID) -> sbk::result<void>
    {
        ZoneScoped;
        SBK_CHECK(containerID != 0, SBK_ERR_INVALID_PARAMETER);

        sbk::engine::system* const system = sbk::engine::system::get();
        SBK_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);

        std::weak_ptr<sbk::core::database_object> container = system->try_find_database_object(containerID);
        SBK_CHECK(!container.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

        std::weak_ptr<sbk::core::database_object> gameObject = get_game_object(gameObjectID);
        SBK_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

        system->get_system_thread_executer()->post(
            [container, gameObject]()
            {
                if (std::shared_ptr<sbk::engine::game_object> sharedGameObject = std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock()))
                {
                    if (std::shared_ptr<sbk::engine::container> sharedContainer = std::static_pointer_cast<sbk::engine::container>(container.lock()))
                    {
                        (void)play_container(sharedGameObject.get(), sharedContainer.get());
                    }
                }
            });

        return sbk::ok();
    }

    auto get_game_object(sbk_id gameObjectID) -> std::weak_ptr<sbk::core::database_object>
    {
        std::weak_ptr<sbk::core::database_object> result;

        if (sbk::engine::system* const system = sbk::engine::system::get())
        {
            if (gameObjectID == 0)
            {
                if (const auto listener = system->get_listener_game_object())
                {
                    return system->try_find_database_object(listener->get_database_id());
                }
            }
            return system->try_find_database_object(gameObjectID);
        }

        return result;
    }
}