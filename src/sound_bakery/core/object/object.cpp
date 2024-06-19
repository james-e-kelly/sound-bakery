#include "sound_bakery/core/object/object.h"

#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/system.h"

using namespace sbk::core;

DEFINE_REFLECTION(sbk::core::object)

object::~object() 
{ 
    m_onDestroyEvent.Broadcast(this); 
}

sbk::engine::system* object_utilities::getSystem() const { return sbk::engine::system::get(); }

sc_system* object_utilities::getChef() const { return getSystem(); }

ma_engine* object_utilities::getMini() const
{
    if (sc_system* system = getChef())
    {
        return (ma_engine*)system;
    }

    return nullptr;
}