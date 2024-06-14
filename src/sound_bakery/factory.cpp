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

SB::Core::object* SB::Engine::Factory::createObjectFromType(rttr::type objectType, sb_id id)
{
    assert(objectType.is_valid() && "object Type must be valid!");

    rttr::constructor constructor = objectType.get_constructor();
    assert(constructor.is_valid() && "object in Sound Bakery must be constructable. Define this in the "
                                     "Reflection file");

    rttr::variant variant        = constructor.invoke();
    const rttr::type variantType = variant.get_type();
    assert(variant.is_valid());
    assert(variantType.is_valid());
    assert(variant.get_type().is_pointer() && "database Objects must be constructed as raw ptrs. Raw objects or "
                                              "shared ptrs are disallowed");
    assert(variantType.get_raw_type().is_valid());

    const rttr::type testObjectType = rttr::type::get<SB::Core::object>();
    const rttr::type databaseType   = rttr::type::get<SB::Core::database_object>();

    assert(databaseType.is_derived_from(testObjectType));

    const rttr::type type    = variant.get_type();
    const rttr::type rawType = type.get_raw_type();

    assert(rawType.is_derived_from(testObjectType) && "Type must derive from the SB::Core::object type");

    bool conversionWasSuccessful        = false;
    SB::Core::database_object* converted = variant.convert<SB::Core::database_object*>(&conversionWasSuccessful);
    assert(converted != nullptr);
    assert(conversionWasSuccessful == true);

    if (SB::Core::ObjectTracker* const objectTracker = SB::Engine::System::getObjectTracker())
    {
        objectTracker->trackObject((SB::Core::object*)converted);
    }

    converted->set_database_id(id);

    return converted;
}

SB::Core::database_object* SB::Engine::Factory::createDatabaseObjectFromType(rttr::type objectType, sb_id id)
{
    assert(objectType.is_derived_from<SB::Core::database_object>());

    return createObjectFromType(objectType, id)->try_convert_object<SB::Core::database_object>();
}
