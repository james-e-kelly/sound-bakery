#pragma once

#include "sound_bakery/core/object/object_owner.h"
#include "sound_bakery/system.h"

namespace sbk
{
    template <typename T>
    std::shared_ptr<T> new_object()
    {
        if (sbk::engine::system* const system = sbk::engine::system::get())
        {
            if (sbk::core::object_owner* const objectOwner = system->current_object_owner())
            {
                return objectOwner->create_runtime_object<T>();
            }
        }

        return {};
    }

    template <typename T>
    std::shared_ptr<T> new_database_object()
    {
        if (sbk::engine::system* const system = sbk::engine::system::get())
        {
            if (sbk::core::object_owner* const objectOwner = system->current_object_owner())
            {
                return objectOwner->create_database_object<T>();
            }
        }

        return {};
    }
}  // namespace sbk