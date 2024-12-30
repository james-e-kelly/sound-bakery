#pragma once

template <typename T>
[[nodiscard]] auto object::casted_shared_from_this() -> std::shared_ptr<T>
{
    return std::static_pointer_cast<T, object>(shared_from_this());
}

template <typename T>
[[nodiscard]] auto object::try_convert_object() noexcept -> T*
{
    if (get_object_type().is_derived_from(T::type()) || get_object_type() == T::type())
    {
        return sbk::reflection::cast<T*, object*>(this);
    }
    return nullptr;
}

template <typename T>
[[nodiscard]] auto object::try_convert_object() const noexcept -> const T*
{
    if (get_object_type().is_derived_from(T::type()) || get_object_type() == T::type())
    {
        return sbk::reflection::cast<const T*, const object*>(this);
    }
    return nullptr;
}