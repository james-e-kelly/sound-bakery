#include "factory.h"

#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/node/bus/aux_bus.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/blend_container.h"
#include "sound_bakery/node/container/random_container.h"
#include "sound_bakery/node/container/sequence_container.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/container/switch_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"

SB::Core::Object* SB::Engine::Factory::createObjectFromType(rttr::type objectType, SB_ID id)
{
    rttr::constructor constructor = objectType.get_constructor();
    assert(constructor.is_valid() && "Object in Sound Bakery must be constructable. Define this in the "
                                     "Reflection file");

    rttr::variant variant = constructor.invoke();
    assert(variant.is_valid());
    assert(variant.get_type().is_pointer() && "Database Objects must be constructed as raw ptrs. Raw objects or "
                                              "shared ptrs is disallowed");

    SB::Core::DatabaseObject* converted = variant.convert<SB::Core::DatabaseObject*>();
    assert(converted != nullptr);

    SB::Core::ObjectTracker::get()->trackObject((SB::Core::Object*)converted);
    converted->setDatabaseID(id);

    return converted;
}

SB::Core::DatabaseObject* SB::Engine::Factory::createDatabaseObjectFromType(rttr::type objectType, SB_ID id)
{
    assert(objectType.is_derived_from<SB::Core::DatabaseObject>());

    return createObjectFromType(objectType, id)->tryConvertObject<SB::Core::DatabaseObject>();
}
