#pragma once

#include <boost/assert.hpp>

#include <cstddef>
#include <cstdint>

// Contains only what EASTL needs to see before it processes any container header:
// the complete polymorphic_allocator (so eastl::compressed_pair can instantiate
// eastl::is_empty on it) plus its memory_resource base. Kept deliberately small so
// including it from pch.h does not drag in rpmalloc, the object system, or anything
// else. The rest of the memory subsystem (rpmalloc_resource, monotonic_buffer_resource,
// malloc/free, owned_object_deleter, ...) lives in sound_bakery/core/memory.h.

namespace sbk::memory
{
    /**
     * @brief Defines groups of objects that are rendered together/in the same tree
     */
    enum class object_category : std::uint8_t
    {
        unknown,
        sound,
        node,
        bus,
        music,
        event,
        bank,
        parameter,
        database_object,
        runtime_object,
        system,
        sound_chef,
        data,
        num
    };

    class memory_resource;
    class polymorphic_allocator;

    /**
     * @brief The memory_resource owned by the current sbk::engine::system, or nullptr if no
     * system exists. Callers must treat nullptr as "allocation attempted outside system lifetime"
     * -- this is the only path polymorphic_allocator uses to reach the system, so no other bit
     * of Sound Bakery can allocate before system::create() or after system::destroy().
     */
    [[nodiscard]] auto system_default_resource() noexcept -> memory_resource*;

    /**
     * @brief Process-local default polymorphic_allocator used by EASTL container defaults.
     * The returned allocator's resource is nullptr; it resolves lazily on each allocate()
     * via sbk::engine::system.
     */
    auto default_eastl_allocator() noexcept -> polymorphic_allocator*;

    /**
     * @brief Virtual, noexcept memory-source interface.
     *
     * Sound Bakery's version of std::pmr::memory_resource. Deliberately not derived from
     * std::pmr because (a) we do not want to participate in std::pmr's process-global default
     * registry and (b) do_allocate is not noexcept in std::pmr and standard resources throw
     * bad_alloc on failure. Every override here must be noexcept and must return nullptr on
     * failure. Callers turn nullptr into an sbk::result error.
     */
    class memory_resource
    {
    public:
        memory_resource()                                                              = default;
        virtual ~memory_resource()                                                     = default;
        memory_resource(const memory_resource&)                                        = delete;
        auto operator=(const memory_resource&) -> memory_resource&                     = delete;
        memory_resource(memory_resource&&)                                             = delete;
        auto operator=(memory_resource&&) -> memory_resource&                          = delete;

        [[nodiscard]] auto allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept -> void*
        {
            return do_allocate(bytes, alignment);
        }

        auto deallocate(void* pointer, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept -> void
        {
            do_deallocate(pointer, bytes, alignment);
        }

        [[nodiscard]] auto is_equal(const memory_resource& other) const noexcept -> bool
        {
            return do_is_equal(other);
        }

    protected:
        virtual auto do_allocate(std::size_t bytes, std::size_t alignment) noexcept -> void*                    = 0;
        virtual auto do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) noexcept -> void    = 0;

        [[nodiscard]] virtual auto do_is_equal(const memory_resource& other) const noexcept -> bool
        {
            return this == &other;
        }
    };

    /**
     * @brief EASTL-shaped allocator that forwards to a sbk::memory::memory_resource*.
     *
     * This is the single allocator type EASTL sees (see eastl_config.h). Two allocators
     * that point at the same resource are interchangeable, so EASTL containers can swap
     * memory sources without changing their template arguments.
     *
     * A default-constructed instance has a null resource; allocate() then resolves the
     * current default via default_eastl_allocator() -> sbk::engine::system. This keeps
     * pre-init and post-shutdown EASTL default-constructed containers safe.
     */
    class polymorphic_allocator
    {
    public:
        polymorphic_allocator() noexcept                                                = default;
        polymorphic_allocator(const polymorphic_allocator&) noexcept                    = default;
        polymorphic_allocator(polymorphic_allocator&&) noexcept                         = default;
        auto operator=(const polymorphic_allocator&) noexcept -> polymorphic_allocator& = default;
        auto operator=(polymorphic_allocator&&) noexcept -> polymorphic_allocator&      = default;
        ~polymorphic_allocator() noexcept                                               = default;

        explicit polymorphic_allocator(memory_resource* resource, const char* name = "sbk::polymorphic_allocator") noexcept
            : m_resource(resource), m_name(name)
        {
        }

        // Name-only constructor. EASTL container defaults expand to `allocator_type("EASTL <container>")`,
        // so every EASTL default-constructed container reaches this ctor with the container's tag as
        // its name. The resource stays null and resolves through sbk::engine::system on each allocate.
        explicit polymorphic_allocator(const char* name) noexcept : m_name(name) {}

        [[nodiscard]] auto get_name() const noexcept -> const char* { return m_name; }
        auto set_name(const char* name) noexcept -> void { m_name = name; }

        [[nodiscard]] auto get_resource() const noexcept -> memory_resource* { return m_resource; }

        [[nodiscard]] auto allocate(size_t size, int /*flags*/ = 0) noexcept -> void*
        {
            if (memory_resource* const resource = resolve())
            {
                return resource->allocate(size);
            }
            BOOST_ASSERT_MSG(false, "allocation attempted before sbk::engine::system::create()");
            return nullptr;
        }

        [[nodiscard]] auto allocate(size_t size, size_t alignment, size_t /*offset*/, int /*flags*/ = 0) noexcept -> void*
        {
            if (memory_resource* const resource = resolve())
            {
                return resource->allocate(size, alignment);
            }
            BOOST_ASSERT_MSG(false, "allocation attempted before sbk::engine::system::create()");
            return nullptr;
        }

        auto deallocate(void* pointer, size_t size) noexcept -> void
        {
            if (pointer == nullptr)
            {
                return;
            }
            if (memory_resource* const resource = resolve())
            {
                resource->deallocate(pointer, size);
                return;
            }
            // The resource that produced this block is gone (system torn down while a container
            // still holds memory). Leaking here is preferable to touching freed state.
            BOOST_ASSERT_MSG(false, "deallocation after sbk::engine::system::destroy()");
        }

    private:
        [[nodiscard]] auto resolve() const noexcept -> memory_resource*
        {
            if (m_resource != nullptr)
            {
                return m_resource;
            }
            return system_default_resource();
        }

        memory_resource* m_resource{};
        const char*      m_name{"sbk::polymorphic_allocator"};
    };

    inline auto operator==(const polymorphic_allocator& lhs, const polymorphic_allocator& rhs) noexcept -> bool
    {
        memory_resource* const lhsResource = lhs.get_resource();
        memory_resource* const rhsResource = rhs.get_resource();
        if (lhsResource == rhsResource)
        {
            return true;
        }
        if (lhsResource == nullptr || rhsResource == nullptr)
        {
            return false;
        }
        return lhsResource->is_equal(*rhsResource);
    }

    inline auto operator!=(const polymorphic_allocator& lhs, const polymorphic_allocator& rhs) noexcept -> bool
    {
        return !(lhs == rhs);
    }
}  // namespace sbk::memory
