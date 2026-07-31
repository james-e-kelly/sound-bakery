#pragma once

#include "sound_bakery/pch.h"

#include "eastl/allocator.h"

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

    class rpmalloc_allocator : public eastl::allocator
    {
    public:
        rpmalloc_allocator() = delete;
        rpmalloc_allocator(SB_OBJECT_CATEGORY objectCategory) : eastl::allocator("rpmalloc_allocator"), m_objectCategory(m_objectCategory) {}

        void* allocate(size_t n, int flags = 0)
        {
            (flags);
            return malloc(n, m_objectCategory);
        }

        void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
        {
            (alignment);
            (offset);
            (flags);
            return malloc(n, m_objectCategory);
        }

        void deallocate(void* p, size_t n)
        {
            (n);
            free(p, m_objectCategory);
        }

    private:
        SB_OBJECT_CATEGORY m_objectCategory{};
    };

    class arena_allocator
    {
    public:
        arena_allocator() = delete;
        arena_allocator(SB_OBJECT_CATEGORY objectCategory) : m_privateAllocator(objectCategory) {}

    private: 
        rpmalloc_allocator m_privateAllocator;
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