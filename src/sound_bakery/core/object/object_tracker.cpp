#include "object_tracker.h"

#include "sound_bakery/system.h"
#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/util/type_helper.h"

using namespace sbk::core;

bool object_ptr_comparator::operator()(const object* lhs, const object* rhs) const 
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

auto object_tracker::track_object(object* object) -> concurrencpp::result<void>
{
    if (object != nullptr)
    {
        const rttr::type type             = object->get_object_type();
        const SB_OBJECT_CATEGORY category = sbk::util::type_helper::getCategoryFromType(type);

        {
            const concurrencpp::scoped_async_lock typeToObjectsLock = co_await m_typeToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());
            const concurrencpp::scoped_async_lock categoryToObjectsLock = co_await m_categoryToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

            m_typeToObjects[type].emplace(object);
            m_categoryToObjects[category].emplace(object);
        }

        object->get_on_destroy().AddRaw(this, &object_tracker::on_object_destroyed);
    }
}

auto object_tracker::untrack_object(object* object, std::optional<rttr::type> typeOverride) -> concurrencpp::result<void>
{
    if (object != nullptr)
    {
        const rttr::type type             = typeOverride.has_value() ? typeOverride.value() : object->get_object_type();
        const SB_OBJECT_CATEGORY category = sbk::util::type_helper::getCategoryFromType(type);

        if (type.is_valid())
        {
            const concurrencpp::scoped_async_lock typeToObjectsLock = co_await m_typeToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());
            m_typeToObjects[type].erase(object);
        }

        {
            const concurrencpp::scoped_async_lock categoryToObjectsLock = co_await m_categoryToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());
            m_categoryToObjects[category].erase(object);
        }

        object->get_on_destroy().RemoveObject(this);
    }
}

auto object_tracker::get_objects_of_category(const SB_OBJECT_CATEGORY& category) const -> concurrencpp::result<std::vector<object*>>
{
    const concurrencpp::scoped_async_lock categoryToObjectsLock = co_await m_categoryToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

    if (m_categoryToObjects.find(category) != m_categoryToObjects.cend())
    {
        const std::unordered_set<object*>& categoryObjects = m_categoryToObjects.at(category);

        std::vector<object*> objects;
        objects.reserve(categoryObjects.size());

        std::transform(categoryObjects.begin(), categoryObjects.end(), std::back_inserter(objects),
                       [](const auto& iter) { return iter; });

        co_return objects;
    }

    co_return std::vector<object*>{};
}

auto object_tracker::get_objects_of_type(const rttr::type& type) const -> concurrencpp::result<std::vector<object*>>
{
    const concurrencpp::scoped_async_lock typeToObjectsLock = co_await m_typeToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

    if (m_typeToObjects.find(type) != m_typeToObjects.cend())
    {
        const std::unordered_set<object*>& typeObjects = m_typeToObjects.at(type);

        std::vector<object*> objects;
        objects.reserve(typeObjects.size());

        std::transform(typeObjects.begin(), typeObjects.end(), std::back_inserter(objects), [](const auto& iter) { return iter; });

        co_return objects;
    }

    co_return std::vector<object*>{};
}

auto object_tracker::get_objects_of_type_size(const rttr::type& type) const -> concurrencpp::result<std::size_t>
{
    const concurrencpp::scoped_async_lock typeToObjectsLock = co_await m_typeToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

    if (m_typeToObjects.find(type) != m_typeToObjects.cend())
    {
        co_return m_typeToObjects.at(type).size();
    }

    co_return 0;
}

auto object_tracker::get_objects_size() const -> concurrencpp::result<size_t>
{
    size_t count = 0;

    const concurrencpp::scoped_async_lock categoryToObjectsLock = co_await m_categoryToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

    for (auto& keyValuePair : m_categoryToObjects)
    {
        count += keyValuePair.second.size();
    }

    co_return count;
}

auto object_tracker::get_all_tracked_objects() const -> concurrencpp::result<std::vector<object*>> 
{
    const concurrencpp::scoped_async_lock typeToObjectsLock = co_await m_typeToObjectsMapLock.lock(sbk::engine::system::get()->get_thread_pool_executor());

    std::vector<object*> objects;
    objects.reserve(m_typeToObjects.size());

    for (const auto& categoryIter : m_typeToObjects)
    {
        objects.reserve(objects.capacity() + categoryIter.second.size());
        
        std::transform(categoryIter.second.begin(), categoryIter.second.end(), std::back_inserter(objects), [](const auto& object) { return object; });
    }
}

auto object_tracker::convert_to_ordered(const std::unordered_set<object*>& unordered) -> std::set<object*, object_ptr_comparator>
{
    std::set<object*, object_ptr_comparator> result;
    for (object* object : unordered)
    {
        result.insert(object);
    }
    return result;
}

void object_tracker::on_object_destroyed(object* object)
{
    if (object != nullptr)
    {
        untrack_object(object);
    }
}