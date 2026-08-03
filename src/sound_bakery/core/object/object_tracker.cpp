#include "object_tracker.h"

#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/core/object/object.h"
#include "sound_bakery/util/type_helper.h"

using namespace sbk::core;

auto object_ptr_comparator::operator()(const object* lhs, const object* rhs) const -> bool
{
    if (lhs && rhs)
    {
        if (const database_object* const databaseLHS = lhs->try_convert_object<database_object>())
        {
            if (const database_object* const databaseRHS = rhs->try_convert_object<database_object>())
            {
                return database_name_comparator{}(databaseLHS->get_database_name(), databaseRHS->get_database_name());
            }
        }
    }
    return lhs < rhs;
}

auto object_tracker::track_object(object* object) -> void
{
    ZoneScoped;
    if (object != nullptr)
    {
        const rttr::type type             = object->get_object_type();
        const sbk::memory::object_category category = sbk::util::type_helper::get_category_from_type(type);

        m_typeToObjects[type].emplace(object);
        m_categoryToObjects[category].emplace(object);
    }
}

auto object_tracker::untrack_object(object* object, std::optional<rttr::type> typeOverride) -> void
{
    ZoneScoped;
    if (object != nullptr)
    {
        const rttr::type type             = typeOverride.has_value() ? typeOverride.value() : object->get_object_type();
        const sbk::memory::object_category category = sbk::util::type_helper::get_category_from_type(type);

        if (type.is_valid())
        {
            m_typeToObjects[type].erase(object);
        }

        m_categoryToObjects[category].erase(object);
    }
}

auto object_tracker::get_objects_of_category(const sbk::memory::object_category& category) const -> std::unordered_set<object*>
{
    ZoneScoped;
    if (m_categoryToObjects.find(category) != m_categoryToObjects.cend())
    {
        return m_categoryToObjects.at(category);
    }

    return {};
}

auto object_tracker::get_objects_of_type(const rttr::type& type) const -> std::unordered_set<object*>
{
    ZoneScoped;
    if (m_typeToObjects.find(type) != m_typeToObjects.cend())
    {
        return m_typeToObjects.at(type);
    }

    return {};
}

auto object_tracker::get_objects_count() const -> size_t
{
    ZoneScoped;
    size_t count = 0;

    for (auto& keyValuePair : m_categoryToObjects)
    {
        count += keyValuePair.second.size();
    }

    return count;
}

auto object_tracker::get_all_category_to_objects() const -> const std::unordered_map<sbk::memory::object_category, std::unordered_set<object*>>&
{
    return m_categoryToObjects;
}

auto object_tracker::get_all_type_to_objects() const -> const std::unordered_map<rttr::type, std::unordered_set<object*>>&
{
    return m_typeToObjects;
}

auto object_tracker::convert_to_ordered(const std::unordered_set<object*>& unordered) const
    -> std::set<object*, object_ptr_comparator>
{
    ZoneScoped;
    std::set<object*, object_ptr_comparator> result;
    for (object* object : unordered)
    {
        result.insert(object);
    }
    return result;
}

auto object_tracker::on_object_destroyed(object* object) -> void
{
    if (object != nullptr)
    {
        untrack_object(object);
    }
}