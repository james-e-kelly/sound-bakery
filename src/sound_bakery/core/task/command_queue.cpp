#include "command_queue.h"

#include "sound_bakery/api/engine_api.h"
#include "sound_bakery/core/thread_domain.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/voice.h"

auto sbk::play_container(sbk::engine::system* system, sbk::engine::game_object* gameObject, sbk::engine::container* container) -> sbk::result<>
{
    SBK_CHECK(system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);
    SBK_CHECK(gameObject != nullptr, SBK_ERR_INVALID_PARAMETER);
    SBK_CHECK(container != nullptr, SBK_ERR_INVALID_PARAMETER);

    SBK_TRY(auto voice, gameObject->create_runtime_object<sbk::engine::voice>());
    return voice->play_container(container);
}

auto sbk::stop_container(sbk::engine::system* system, sbk::engine::game_object* gameObject, sbk::engine::container* container) -> sbk::result<>
{
    SBK_CHECK(system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);
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

auto sbk::dispatch_event(sbk::engine::system* system, sbk::engine::game_object* gameObject, sbk::engine::event* event) -> sbk::result<>
{
    SBK_CHECK(system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);
    SBK_CHECK(gameObject != nullptr, SBK_ERR_INVALID_PARAMETER);
    SBK_CHECK(event != nullptr, SBK_ERR_INVALID_PARAMETER);

    SBK_INFO("Posting Event");

    for (const sbk::engine::action& action : event->m_actions)
    {
        SBK_CHECK(action.m_type != sbk::engine::action_type::invalid, SBK_ERR_BAKERY);
        SBK_CHECK(action.m_type != sbk::engine::action_type::num, SBK_ERR_BAKERY);

        sbk::engine::container* container          = nullptr;
        sbk::engine::event* childEvent             = nullptr;
        sbk::engine::game_object* targetGameObject = nullptr;

        if (const auto destination = action.m_destination.shared())
        {
            container        = destination->try_convert_object<sbk::engine::container>();
            childEvent       = destination->try_convert_object<sbk::engine::event>();
            targetGameObject = destination->try_convert_object<sbk::engine::game_object>();
        }

        switch (action.m_type)
        {
            case sbk::engine::action_type::play:
                if (container)
                {
                    SBK_TRYV(play_container(system, gameObject, container));
                }
                else if (childEvent)
                {
                    SBK_TRYV(dispatch_event(system, gameObject, childEvent));
                }
                break;
            case sbk::engine::action_type::stop:
                if (container)
                {
                    SBK_TRYV(stop_container(system, gameObject, container));
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

auto sbk::command_queue::process_commands(sbk::engine::system* system) noexcept -> sbk::result<>
{
    ZoneScoped;
    SBK_CHECK(system != nullptr, SBK_ERR_INVALID_PARAMETER);

    message_queue<message_type>::message_view messageView;

    for (;;)
    {
        const sbk_status readResult = m_messageQueue.read_begin(&messageView);

        switch (readResult)
        {
            case SBK_ERR_INVALID_PARAMETER:
                return sbk::make_error(readResult, "Invalid parameter used when reading a command");
            case SBK_ERR_UNINITIALIZED:
                return sbk::make_error(readResult, "Command buffer was not initialized");
            case SBK_ERR_EMPTY:
                return sbk::ok();
            case SBK_ERR_TOO_LARGE:
                sbk::log_error(readResult, "Read request was too large");
                continue;
            default:
                break;
        }

        BOOST_ASSERT(readResult == SBK_SUCCESS);

        if (messageView.m_identifier == message_type::end_of_frame)
        {
            SBK_TRY_C(m_messageQueue.read_end(messageView));
            break;
        }

        (void)process_command(messageView, system);

        SBK_TRY_C(m_messageQueue.read_end(messageView));
    }
    return sbk::ok();
}

auto sbk::command_queue::process_command(const message_queue<message_type>::message_view& messageView, sbk::engine::system* system) noexcept -> sbk::result<>
{
    ZoneScoped;

    switch (messageView.m_identifier)
    {
        case message_type::end_of_frame:
            // We expect the calling process_commands to handle end_of_frame messages
            // But if not, return an error without logging
            return tl::make_unexpected<sbk::error>(sbk::error(SBK_ERR_AT_END, std::source_location::current()));
        case message_type::load_soundbank:
        {
            const load_soundbank_message* message = messageView.cast<load_soundbank_message>();

            sbk::core::serialization::binary_serializer binarySerializer;
            const sbk::result<sbk_id> soundbankID = binarySerializer.load_object<sbk::core::serialization::serialized_soundbank>(system->get_current_object_owner(), message->filename);
            SBK_CHECK_MSG(soundbankID.has_value(), SBK_ERR_BAKERY_SERIALIZATION, "failed to load soundbank '{}'", message->filename);
        }
        break;
        case message_type::post_event:
        {
            const post_event_message* message = messageView.cast<post_event_message>();

            auto event = system->try_find_database_object(message->eventID);
            SBK_CHECK(!event.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            auto gameObject = sbk::engine::get_game_object(message->gameObjectID);
            SBK_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            auto sharedEvent      = std::static_pointer_cast<sbk::engine::event>(event.lock());
            auto sharedGameObject = std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock());
            SBK_CHECK(sharedEvent && sharedGameObject, SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            SBK_TRYV(dispatch_event(system, sharedGameObject.get(), sharedEvent.get()));
        }
        break;
        case message_type::post_event_name:
        {
            const post_event_name_message* message = messageView.cast<post_event_name_message>();

            auto event = system->try_find_database_object(sbk::core::database_name(message->eventName));
            SBK_CHECK_MSG(!event.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND, "no event named '{}'", message->eventName);

            auto gameObject = sbk::engine::get_game_object(message->gameObjectID);
            SBK_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            auto sharedEvent      = std::static_pointer_cast<sbk::engine::event>(event.lock());
            auto sharedGameObject = std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock());
            SBK_CHECK(sharedEvent && sharedGameObject, SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            SBK_TRYV(dispatch_event(system, sharedGameObject.get(), sharedEvent.get()));
        }
        break;
        case message_type::stop_all:
        {
            const stop_all_message* message = messageView.cast<stop_all_message>();

            const std::weak_ptr<sbk::core::database_object> gameObject = sbk::engine::get_game_object(message->gameObjectID);
            SBK_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            auto sharedGameObject = std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock());
            SBK_CHECK(sharedGameObject, SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            sharedGameObject->remove_all();
        }
        break;
        case message_type::play_container:
        {
            const play_container_message* message = messageView.cast<play_container_message>();

            auto container = system->try_find_database_object(message->containerID);
            SBK_CHECK_MSG(!container.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND, "no container with ID '{}'", message->containerID);

            auto gameObject = sbk::engine::get_game_object(message->gameObjectID);
            SBK_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            auto sharedContainer  = std::static_pointer_cast<sbk::engine::container>(container.lock());
            auto sharedGameObject = std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock());
            SBK_CHECK(sharedContainer && sharedGameObject, SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

            SBK_TRYV(play_container(system, sharedGameObject.get(), sharedContainer.get()));
        }
        break;
    }
    return sbk::ok();
}