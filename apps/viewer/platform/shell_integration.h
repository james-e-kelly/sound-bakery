#pragma once

#include "tl/expected.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

/**
 * @brief Keeps Sound Viewer to one window and hooks it into Explorer's "Open with".
 *
 * Only does anything on Windows; elsewhere every launch runs its own instance and registration reports unsupported.
 */
namespace shell_integration
{
    /**
     * @brief If Sound Viewer is already running, hands it these files and brings it to the front.
     *
     * Must be called before any window is created. Waits briefly for an instance that is still starting up,
     * since selecting several files in Explorer and pressing Enter launches them all at once.
     *
     * @return true when a running instance took the launch and this process should exit.
     */
    auto forward_to_running_instance(const std::vector<std::filesystem::path>& files) -> bool;

    /**
     * @brief Accepts launches forwarded from later instances. The callback runs on the main thread while window events are polled.
     */
    auto start_accepting_forwarded_launches(std::function<void(std::vector<std::filesystem::path>)> onFilesReceived) -> void;
    auto stop_accepting_forwarded_launches() -> void;

    enum class registration_state
    {
        not_registered,
        registered,
        registered_to_other_copy  //< Registered, but "Open with" launches a different Sound Viewer executable
    };

    auto get_registration_state() -> registration_state;

    /**
     * @brief Adds Sound Viewer to "Open with" for the supported audio types, for the current user only (no admin needed).
     *
     * Windows doesn't let apps make themselves the default; @see open_default_apps_settings lets the user do that.
     */
    auto register_file_types() -> tl::expected<void, std::string>;
    auto unregister_file_types() -> tl::expected<void, std::string>;

    /**
     * @brief Opens Windows' Default Apps page for Sound Viewer.
     */
    auto open_default_apps_settings() -> void;
}  // namespace shell_integration
