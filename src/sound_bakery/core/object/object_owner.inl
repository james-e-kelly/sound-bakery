#pragma once

template <typename T>
auto object_owner::create_raw_object() -> sbk::result<std::shared_ptr<T>>
{
    ZoneScoped;
    std::shared_ptr<T> result = std::make_shared<T>();

    m_objects.emplace_back(result);

    result->set_owner(this);
    result->cache_type();

    return result;
}

template <typename T>
auto object_owner::create_runtime_object() -> sbk::result<std::shared_ptr<T>>
{
    ZoneScoped;
    static_assert(std::is_base_of_v<sbk::core::object, T>, "Runtime objects must derive from sbk::core::object");

    SBK_TRY(auto object, create_runtime_object(rttr::type::get<T>()));

    if (std::shared_ptr<T> castedObject = std::static_pointer_cast<T>(object))
    {
        return castedObject;
    }

    return sbk::make_error(SBK_ERR_BAKERY);
}

template <typename T>
auto object_owner::create_database_object(bool addToDatabase) -> sbk::result<std::shared_ptr<T>>
{
    ZoneScoped;
    static_assert(std::is_base_of_v<sbk::core::database_object, T>, "Database objects must derive from sbk::core::database_object");

    SBK_TRY(auto object, create_database_object(rttr::type::get<T>(), addToDatabase));

    if (std::shared_ptr<T> castedObject = std::static_pointer_cast<T>(object))
    {
        return castedObject;
    }

    return sbk::make_error(SBK_ERR_BAKERY);
}