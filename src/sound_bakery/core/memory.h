#pragma once

#include "sound_bakery/pch.h"

namespace sbk::core
{
    class object;
}

namespace sbk::memory
{
    struct object_deleter
    {
        auto operator()(sbk::core::object* object) const noexcept -> void;
    };

    auto malloc(std::size_t size, SB_OBJECT_CATEGORY category) -> void*;
    auto realloc(void* pointer, std::size_t size) -> void*;
    auto free(void* pointer, SB_OBJECT_CATEGORY category) -> void;

    auto thread_start(std::string_view threadName) -> void;
    auto thread_end(std::string_view threadName) -> void;

    template <typename T>
    struct owned_object_deleter
    {
        SB_OBJECT_CATEGORY category = SB_CATEGORY_UNKNOWN;

        owned_object_deleter() = default;
        explicit owned_object_deleter(SB_OBJECT_CATEGORY inCategory) noexcept : category(inCategory) {}

        template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
        owned_object_deleter(const owned_object_deleter<U>& other) noexcept : category(other.category) {}

        auto operator()(T* object) const noexcept -> void
        {
            static_assert(0 < sizeof(T), "can't delete an incomplete type");
            object->~T();
            sbk::memory::free(object, category);
        }
    };
}  // namespace sbk::memory

namespace sbk
{
    /**
     * @brief A simple unique ptr that lets us delete the object with sbk::memory::free
     */
    template<typename T>
    using owned_ptr = std::unique_ptr<T, sbk::memory::owned_object_deleter<T>>;
}