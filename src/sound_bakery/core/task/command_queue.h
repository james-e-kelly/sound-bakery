#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/containers/message_queue.h"

namespace sbk
{
    namespace engine
    {
        class container;
        class event;
        class game_object;
        class system;
    }

    enum class message_type : std::uint8_t
    {
        end_of_frame,       //< End of the frame and the command queue should stop processing and wait for the next update
        load_soundbank,
        post_event,
        post_event_name,
        stop_all,

        // INTERNAL
        play_container
    };

    auto play_container(sbk::engine::system* system, sbk::engine::game_object* gameObject, sbk::engine::container* container) -> sbk::result<>;
    auto stop_container(sbk::engine::system* system, sbk::engine::game_object* gameObject, sbk::engine::container* container) -> sbk::result<>;
    auto dispatch_event(sbk::engine::system* system, sbk::engine::game_object* gameObject, sbk::engine::event* event) -> sbk::result<>;

    struct end_of_frame_message
    {
    };

    struct load_soundbank_message
    {
        char filename[256];
    };

    struct post_event_message
    {
        sbk_id eventID;
        sbk_id gameObjectID;
    };

    struct post_event_name_message
    {
        char eventName[256];
        sbk_id gameObjectID;
    };

    struct stop_all_message
    {
        sbk_id gameObjectID;
    };

    struct play_container_message
    {
        sbk_id containerID;
        sbk_id gameObjectID;
    };

    class command_queue final
    {
    public:
        [[nodiscard]] auto init(std::size_t size, const ma_allocation_callbacks* allocationCallbacks) noexcept -> sbk::result<>
        {
            return m_messageQueue.init(size, allocationCallbacks);
        }

        template<typename T>
        auto write_command(const message_type& type, const T& message) noexcept -> sbk_status
        {
            return m_messageQueue.write_message(type, message);
        }

        auto process_commands(sbk::engine::system* system) noexcept -> sbk::result<>;

    private:
        auto process_command(const message_queue<message_type>::message_view& messageView, sbk::engine::system* system) noexcept -> sbk::result<>;

        message_queue<message_type> m_messageQueue;
    };
}  // namespace sbk
