#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/containers/message_queue.h"
#include "sound_bakery/core/object/object_owner.h"

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
        stop_all
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

    class command_queue final : public sbk::core::object_owner
    {
    public:
        [[nodiscard]] auto init(std::size_t size, sbk::memory::memory_resource& allocator) noexcept -> sbk::result<>
        {
            return m_messageQueue.init(size, allocator);
        }

        template<typename T>
        auto write_command(const message_type& type, const T& message) noexcept -> sbk_status
        {
            return m_messageQueue.write_message(type, message);
        }

        auto process_commands() noexcept -> sbk::result<>;

    private:
        auto process_command(const message_queue<message_type>::message_view& messageView, sbk::engine::system* system) noexcept -> sbk::result<>;

        message_queue<message_type> m_messageQueue;
    };
}  // namespace sbk
