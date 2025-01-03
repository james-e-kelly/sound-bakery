#include "object_owner.h"

#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/system.h"

auto sbk::core::object_owner::create_runtime_object(const rttr::type& type) -> std::shared_ptr<sbk::core::object>
{
    sbk::engine::system* const system = sbk::engine::system::get();

    if (!system)
    {
        SBK_CRITICAL("Cannot create object. System is invalid");
        return {};
    }

    if (!type.is_valid())
    {
        SBK_ERROR("Cannot create object. Object type is invalid");
        return {};
    }

    if (!type.is_derived_from(rttr::type::get<sbk::core::object>()))
    {
        SBK_ERROR("Cannot create object. Objects must derive from the base object type");
        return {};
    }

    const rttr::constructor constructor = type.get_constructor();

    if (!constructor.is_valid())
    {
        SBK_ERROR("Objects in Sound Bakery must be constructable. Define this in the reflection file");
        return {};
    }

    const rttr::variant variant = constructor.invoke();

    if (!variant.is_valid() || !variant.get_type().is_valid() || !variant.get_type().get_raw_type().is_valid())
    {
        SBK_ERROR("Failed to create object");
        return {};
    }

    std::shared_ptr<sbk::core::object> result;

    // If shared_ptr
    if (variant.get_type().is_wrapper())
    {
        result = variant.convert<std::shared_ptr<sbk::core::object>>();
    }
    // If raw
    else if (variant.get_type().is_pointer())
    {
        result = std::shared_ptr<sbk::core::object>(variant.convert<sbk::core::object*>());
    }
    else
    {
        BOOST_ASSERT(false);
    }

    BOOST_ASSERT(result);

    m_objects.emplace_back(result);

    system->track_object(result.get());

    result->set_owner(this);

    result->cache_type();

    return result;
}

auto sbk::core::object_owner::create_database_object(const rttr::type& type,
                                                     bool addToDatabase) -> std::shared_ptr<sbk::core::database_object>
{
    sbk::engine::system* const system = sbk::engine::system::get();

    if (system == nullptr)
    {
        SBK_CRITICAL("Cannot create object. System is invalid");
        return {};
    }

    if (!type.is_derived_from(rttr::type::get<sbk::core::database_object>()))
    {
        SBK_ERROR("Cannot create object. Database objects must derive from the base database object type");
        return {};
    }

    if (const std::shared_ptr<object> object = create_runtime_object(type))
    {
        if (std::shared_ptr<database_object> databaseObject = std::static_pointer_cast<database_object>(object))
        {
            if (addToDatabase)
            {
                system->add_object_to_database(databaseObject);
            }

            return databaseObject;
        }
    }

    return {};
}

auto sbk::core::object_owner::add_reference_to_object(std::shared_ptr<database_object>& object) -> void
{
    if (object)
    {
        m_objects.push_back(object);
    }
}

auto sbk::core::object_owner::remove_object(const std::shared_ptr<object>& object)
    -> std::vector<std::shared_ptr<sbk::core::object>>::iterator
{
    if (object)
    {
        for (std::vector<std::shared_ptr<sbk::core::object>>::iterator iter = m_objects.begin();
             iter != m_objects.end(); ++iter)
        {
            if (*iter == object)
            {
                return m_objects.erase(iter);
            }
        }
    }
    return m_objects.end();
}

auto sbk::core::object_owner::destroy_all() -> void { m_objects.clear(); }

auto sbk::core::object_owner::get_objects() -> std::vector<std::shared_ptr<object>>& { return m_objects; }

auto sbk::core::object_owner::get_objects() const -> const std::vector<std::shared_ptr<object>>& { return m_objects; }

auto sbk::core::object_owner::get_objects_size() const -> std::size_t { return m_objects.size(); }
