#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/error/result.h"

namespace sbk::engine
{
    /**
     * @brief Creates an instance of Sound Bakery and opens the project.
     * 
     * @todo Make these functions in the editor folder or inside the project itself.
     */
    [[nodiscard]] auto open_project(const std::filesystem::path& projectFile, sbk::core::sbk_log_callback_proc logCallback) -> sbk::result<void>;

    /**
     * @brief Creates a project and initializes Sound Bakery.
     */
    [[nodiscard]] auto create_project(const std::filesystem::directory_entry& projectDirectory, std::string_view projectName) -> sbk::result<void>;

    /**
     * @brief Plays a container directly on a game object, bypassing events.
     */
    auto post_container(sbk_id containerID, sbk_id gameObjectID) -> sbk::result<void>;

    /**
     * @brief Get a weak reference to a game object based on its ID.
     */
    auto get_game_object(sbk_id gameObjectID) -> std::weak_ptr<sbk::core::database_object>;
}  // namespace sbk::engine
