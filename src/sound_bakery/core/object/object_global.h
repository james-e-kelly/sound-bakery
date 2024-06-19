#pragma once

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/system.h"

template <typename T>
T* newObject()
{
    static_assert(std::is_base_of<sbk::core::object, T>::value);

    T* obj = new T();

    if (obj)
    {
        if (sbk::core::object_tracker* const objectTracker = sbk::engine::system::getObjectTracker())
        {
            objectTracker->track_object((sbk::core::object*)obj);
        }
    }
    else
    {
        assert(false && "Memory allocation failed");
    }

    return obj;
}

template <typename T>
T* newDatabaseObject(sbk_id id = 0)
{
    static_assert(std::is_base_of<sbk::core::database_object, T>::value);

    T* obj = new T();

    if (obj)
    {
        if (sbk::core::object_tracker* const objectTracker = sbk::engine::system::get())
        {
            objectTracker->track_object((sbk::core::object*)obj);
        }
    }
    else
    {
        assert(false && "Memory allocation failed");
    }

    obj->set_database_id(id);

    assert(obj->get_database_id() && "ID must be valid by this point");

    return obj;
}