#pragma once

#include "sound_bakery/system.h"
#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/system.h"

template <typename T>
T* newObject()
{
    static_assert(std::is_base_of<SB::Core::object, T>::value);

    T* obj = new T();

    if (obj)
    {
        if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
        {
            objectTracker->trackObject((SB::Core::object*)obj);
        }
    }
    else
    {
        assert(false && "Memory allocation failed");
    }

    return obj;
}

template <typename T>
T* newDatabaseObject(sb_id id = 0)
{
    static_assert(std::is_base_of<SB::Core::database_object, T>::value);

    T* obj = new T();

    if (obj)
    {
        if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
        {
            objectTracker->trackObject((SB::Core::object*)obj);
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