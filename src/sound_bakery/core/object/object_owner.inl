#pragma once

template <typename T>
auto object_owner::create_runtime_object() -> std::shared_ptr<T>
{
    if (!rttr::type::get<T>().is_derived_from(rttr::type::get<sbk::core::object>()))
    {
        SPDLOG_ERROR("Cannot create object. Objects must derive from the base object type");
        return {};
    }

    if (std::shared_ptr<object> object = create_runtime_object(rttr::type::get<T>()))
    {
        if (std::shared_ptr<T> castedObject = std::static_pointer_cast<T>(object))
        {
            return castedObject;
        }
    }

    return {};
}

template <typename T>
auto object_owner::create_database_object(bool addToDatabase) -> std::shared_ptr<T>
{
    if (!rttr::type::get<T>().is_derived_from(rttr::type::get<sbk::core::database_object>()))
    {
        SPDLOG_ERROR("Cannot create object. Database objects must derive from the base database object type");
        return {};
    }

    if (std::shared_ptr<database_object> object = create_database_object(rttr::type::get<T>(), addToDatabase))
    {
        if (std::shared_ptr<T> castedObject = std::static_pointer_cast<T>(object))
        {
            return castedObject;
        }
    }

    return {};
}