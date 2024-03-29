#include "object_tracker.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/util/type_helper.h"

using namespace SB::Core;

void ObjectTracker::trackObject(RawObjectPtr object)
{
    if (object != nullptr)
    {
        const rttr::type type             = object->getType();
        const SB_OBJECT_CATEGORY category = SB::Util::TypeHelper::getCategoryFromType(type);

        m_typeToObjects[type].emplace(object);
        m_categoryToObjects[category].emplace(object);
    }
}

void ObjectTracker::untrackObject(RawObjectPtr object, std::optional<rttr::type> typeOverride)
{
    if (object != nullptr)
    {
        const rttr::type type             = typeOverride.has_value() ? typeOverride.value() : object->getType();
        const SB_OBJECT_CATEGORY category = SB::Util::TypeHelper::getCategoryFromType(type);

        m_typeToObjects[type].erase(object);
        m_categoryToObjects[category].erase(object);
    }
}

std::unordered_set<ObjectTracker::RawObjectPtr> ObjectTracker::getObjectsOfCategory(SB_OBJECT_CATEGORY category)
{
    return m_categoryToObjects[category];
}

std::unordered_set<ObjectTracker::RawObjectPtr> ObjectTracker::getObjectsOfType(rttr::type type)
{
    return m_typeToObjects[type];
}