#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/error/error.h"

namespace sbk::engine
{
    /**
     * @brief Plays a container directly on a game object, bypassing events.
     */
    auto post_container(sbk_id containerID, sbk_id gameObjectID) -> sbk::result<void>;

    /**
     * @brief Get a weak reference to a game object based on its ID.
     */
    auto get_game_object(sbk_id gameObjectID) -> std::weak_ptr<sbk::core::database_object>;
}  // namespace sbk::engine
