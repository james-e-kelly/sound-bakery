#include "sound_bakery/core/object/object.h"

#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/system.h"

using namespace SB::Core;

DEFINE_REFLECTION(SB::Core::Object)

Object::~Object() 
{ 
    if (ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
    {
        objectTracker->untrackObject(this, m_type);
    }
}

SB::Engine::System* ObjectUtilities::getSystem() const { return SB::Engine::System::get(); }

SC_SYSTEM* ObjectUtilities::getChef() const
{
    if (SB::Engine::System* system = getSystem())
    {
        return system->getChef();
    }
    return nullptr;
}

ma_engine* ObjectUtilities::getMini() const
{
    if (SC_SYSTEM* system = getChef())
    {
        return (ma_engine*)system;
    }

    return nullptr;
}