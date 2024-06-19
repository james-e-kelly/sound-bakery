#include "object_tracker.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/util/type_helper.h"

using namespace sbk::core;

void object_tracker::track_object(object* object)
{
    if (object != nullptr)
    {
        const rttr::type type             = object->getType();
        const SB_OBJECT_CATEGORY category = sbk::Util::TypeHelper::getCategoryFromType(type);

        m_typeToObjects[type].emplace(object);
        m_categoryToObjects[category].emplace(object);

        object->get_on_destroy().AddRaw(this, &object_tracker::on_object_destroyed);
    }
}

void object_tracker::untrack_object(object* object, std::optional<rttr::type> typeOverride)
{
    if (object != nullptr)
    {
        const rttr::type type             = typeOverride.has_value() ? typeOverride.value() : object->getType();
        const SB_OBJECT_CATEGORY category = sbk::Util::TypeHelper::getCategoryFromType(type);

        if (type.is_valid())
        {
            m_typeToObjects[type].erase(object);
        }

        m_categoryToObjects[category].erase(object);

        object->get_on_destroy().RemoveObject(this);
    }
}

std::unordered_set<object*> object_tracker::get_objects_of_category(const SB_OBJECT_CATEGORY& category) const
{
    return m_categoryToObjects.at(category);
}

std::unordered_set<object*> object_tracker::get_objects_of_type(const rttr::type& type) const { return m_typeToObjects.at(type); }

void object_tracker::on_object_destroyed(object* object)
{
    if (object != nullptr)
    {
        untrack_object(object);
    }
}