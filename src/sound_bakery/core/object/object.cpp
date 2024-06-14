#include "sound_bakery/core/object/object.h"

#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/system.h"

using namespace SB::Core;

DEFINE_REFLECTION(SB::Core::object)

object::~object()
{
    if (ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
    {
        objectTracker->untrackObject(this, m_type);
    }
}

SB::Engine::System* object_utilities::getSystem() const { return SB::Engine::System::get(); }

sc_system* object_utilities::getChef() const
{
    if (SB::Engine::System* system = getSystem())
    {
        return system->getChef();
    }
    return nullptr;
}

ma_engine* object_utilities::getMini() const
{
    if (sc_system* system = getChef())
    {
        return (ma_engine*)system;
    }

    return nullptr;
}