#include "object_owner.h"

#include "sound_bakery/system.h"

std::shared_ptr<sbk::core::object> sbk::core::object_owner::create_runtime_object(const rttr::type& type)
{
    sbk::engine::system* const system = sbk::engine::system::get();

    if (!system)
    {
        SPDLOG_CRITICAL("Cannot create object. System is invalid");
        return {};
    }

    if (!type.is_valid())
    {
        SPDLOG_ERROR("Cannot create object. Object type is invalid");
        return {};
    }

    if (!type.is_derived_from(rttr::type::get<sbk::core::object>()))
    {
        SPDLOG_ERROR("Cannot create object. Objects must derive from the base object type");
        return {};
    }

    const rttr::constructor constructor = type.get_constructor();

    if (!constructor.is_valid())
    {
        SPDLOG_ERROR("Objects in Sound Bakery must be constructable. Define this in the reflection file");
        return {};
    }

    const rttr::variant variant = constructor.invoke();

    if (!variant.is_valid() || !variant.get_type().is_valid() || !variant.get_type().get_raw_type().is_valid())
    {
        SPDLOG_ERROR("Failed to create object");
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
        assert(false);
    }

    assert(result);

    m_objects.emplace_back(result);

    system->track_object(result.get());

    result->set_owner(this);

    return result;
}

std::shared_ptr<sbk::core::database_object> sbk::core::object_owner::create_database_object(const rttr::type& type)
{
    sbk::engine::system* const system = sbk::engine::system::get();

    if (!system)
    {
        SPDLOG_CRITICAL("Cannot create object. System is invalid");
        return {};
    }

    if (!type.is_derived_from(rttr::type::get<sbk::core::database_object>()))
    {
        SPDLOG_ERROR("Cannot create object. Database objects must derive from the base database object type");
        return {};
    }

    if (std::shared_ptr<object> object = create_runtime_object(type))
    {
        if (std::shared_ptr<database_object> databaseObject = std::static_pointer_cast<database_object>(object))
        {
            system->add_object_to_database(databaseObject);

            return databaseObject;
        }
    }

    return {};
}
