#include "sound_bakery/core/object/object.h"

#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/system.h"

using namespace sbk::core;

DEFINE_REFLECTION(sbk::core::object)

object::~object() { m_onDestroyEvent.Broadcast(this); }

void object::destroy()
{
    if (m_owner)
    {
        m_owner->remove_object(shared_from_this());
    }
}