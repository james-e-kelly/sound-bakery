#pragma once

#include "sound_bakery/error/error.h"
#include "sound_bakery/profiling/remote_protocol.h"
#include "sound_bakery/sound_bakery_common.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace sbk::engine::profiling
{
    /**
     * @brief Connects to a running Sound Bakery instance and receives telemetry.
     *
     * The tool-side half of remote profiling: the editor (or any external tool)
     * owns one of these and points it at a game running @ref remote_session_host.
     *
     * Like the host, Asio is hidden behind a pimpl and nothing is threaded -
     * the owner calls @ref update every frame and polls @ref get_latest_telemetry.
     */
    class SB_CLASS remote_session final
    {
    public:
        remote_session();
        ~remote_session();

        remote_session(const remote_session&)                    = delete;
        auto operator=(const remote_session&) -> remote_session& = delete;

        /**
         * @brief Starts connecting to @p host : @p port.
         *
         * Name resolution is blocking but the connect itself is asynchronous;
         * poll @ref is_connected after calling @ref update.
         */
        [[nodiscard]] auto connect(std::string_view host, std::uint16_t port) -> sbk::result<void>;

        auto disconnect() -> void;

        /**
         * @brief Pumps the connection and any received messages. Call once per frame.
         */
        auto update() -> void;

        [[nodiscard]] auto is_connecting() const -> bool;
        [[nodiscard]] auto is_connected() const -> bool;  //< True once the host's hello has arrived.

        /**
         * @brief Sends a live edit to the connected runtime, e.g. a bus volume change.
         *
         * @p propertyID is the @c sbk::core::synced_property_id of the
         * reflected property name. Commands are queued in order and never
         * dropped; the runtime applies them on its next update. Fails if not
         * connected.
         */
        [[nodiscard]] auto send_set_property(sbk_id objectID, std::uint32_t propertyID, float value)
            -> sbk::result<void>;

        /**
         * @brief The most recent snapshot received, if any.
         */
        [[nodiscard]] auto get_latest_telemetry() const -> std::optional<telemetry_snapshot>;

        /**
         * @brief Total telemetry messages received; compare across frames to detect fresh data.
         */
        [[nodiscard]] auto get_telemetry_count() const -> std::uint64_t;

    private:
        struct impl;
        std::unique_ptr<impl> m_impl;
    };
}  // namespace sbk::engine::profiling
