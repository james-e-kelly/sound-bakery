#pragma once

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/system.h"

template <typename T>
T* newObject()
{
    static_assert(std::is_base_of<SB::Core::Object, T>::value);

    T* obj = new T();

    if (obj)
    {
        if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
        {
            objectTracker->trackObject((SB::Core::Object*)obj);
        }
    }
    else
    {
        assert(false && "Memory allocation failed");
    }

    return obj;
}

template <typename T>
T* newDatabaseObject(SB_ID id = 0)
{
    static_assert(std::is_base_of<SB::Core::DatabaseObject, T>::value);

    T* obj = new T();

    if (obj)
    {
        if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
        {
            objectTracker->trackObject((SB::Core::Object*)obj);
        }
    }
    else
    {
        assert(false && "Memory allocation failed");
    }

    obj->setDatabaseID(id);

    assert(obj->getDatabaseID() && "ID must be valid by this point");

    return obj;
}