#pragma once

#include "sound_bakery/pch.h"
#include "sound_bakery/error/error.h"
#include "sound_bakery/profiling/remote_protocol.h"

namespace sbk::engine::profiling
{
    /**
     * @brief The "game" side of profiling.
     *
     * Lives inside the game/runtime instance. The editor (or any tool) connects
     * with @ref remote_session and receives @ref telemetry_snapshot messages.
     *
     * Built on Boost.Asio, which stays hidden behind a pimpl so no consumer of
     * this header pulls in Asio or the platform socket headers. No threads are
     * created: all network work happens when @ref update polls the io context,
     * which @ref sbk::engine::system::update drives once per frame.
     */
    class SB_CLASS remote_session_host final
    {
    public:
        remote_session_host();
        ~remote_session_host();

        remote_session_host(const remote_session_host&)                    = delete;
        remote_session_host(remote_session_host&&)                         = delete;
        auto operator=(const remote_session_host&) -> remote_session_host& = delete;
        auto operator=(remote_session_host&&) -> remote_session_host&      = delete;

        /**
         * @brief Starts listening for tools on @p port.
         *
         * Pass 0 to let the OS pick a free port (see @ref get_port), or
         * @ref remoteDefaultPort for the well-known one. Closes any previous
         * connection first.
         */
        [[nodiscard]] auto open(std::uint16_t port) -> sbk::result<void>;

        /**
         * @brief Stops listening and drops every connected tool.
         */
        auto close() -> void;

        /**
         * @brief Pumps accepts, sends, and disconnects. Call once per frame.
         */
        auto update() -> void;

        /**
         * @brief Queues @p snapshot for broadcast to every connected tool.
         *
         * If a tool is still mid-send, only the newest snapshot is kept for
         * it - telemetry is a "latest wins" stream, never a backlog.
         */
        auto publish_telemetry(const telemetry_snapshot& snapshot) -> void;

        /**
         * @brief Takes ownership of every live-edit command received since the last call.
         *
         * Commands arrive during @ref update; the owner (the system's update
         * loop) drains and applies them. Returned in arrival order.
         */
        [[nodiscard]] auto consume_property_commands() -> std::vector<set_property_command>;

        [[nodiscard]] auto is_open() const -> bool;
        [[nodiscard]] auto get_port() const -> std::uint16_t;  //< The actual listening port; useful when opened with port 0.
        [[nodiscard]] auto get_connection_count() const -> std::size_t;

    private:
        struct impl;
        std::unique_ptr<impl> m_impl;
    };
}  // namespace sbk::engine::profiling
