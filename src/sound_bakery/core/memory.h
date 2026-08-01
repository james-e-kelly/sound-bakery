#pragma once

#include "sound_bakery/core/eastl_config.h"
#include "sound_bakery/error/result.h"
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

    template <class T>
    concept pointer = std::is_pointer_v<T>;

    template <pointer T>
    [[nodiscard]] constexpr auto offset_ptr(T ptr, std::size_t offset) noexcept -> T
    {
        return static_cast<std::uint8_t*>(ptr) + offset;
    }

    /**
     * @brief Default alignment used by call sites that do not know the specific type's alignment
     * (miniaudio allocation callbacks, EASTL global new[] overloads, object::operator new). Matches
     * std::max_align_t.
     */
    inline constexpr std::size_t default_alignment = alignof(std::max_align_t);

    /**
     * @brief Defines groups of objects that are rendered together/in the same tree
     */
    enum class object_category : std::uint8_t
    {
        /**
         * @brief Unkown category
         */
        unknown,
        /**
         * @brief Sound file
         */
        sound,
        /**
         * @brief Every sound, random, blend etc.
         */
        node,
        /**
         * @brief Bus or aux busses
         */
        bus,
        /**
         * @brief Music nodes like music segments
         */
        music,
        /**
         * @brief Events
         */
        event,
        /**
         * @brief Soundbanks
         */
        bank,
        /**
         * @brief Parameter types
         */
        parameter,
        /**
         * @brief Any identifiable object not categorised above
         */
        database_object,
        /**
         * @brief Any runtime object
         */
        runtime_object,
        /**
         * @brief System object, or anything that is global
         */
        system,
        /**
         * @brief Low-level audio allocations from Sound Chef
         */
        sound_chef,
        /**
         * @brief General data and structures allocated from vectors, queues, strings etc.
         */
        data,
        num
    };

    /**
     * @name Lifecycle of the underlying allocator.
     *
     * init() must be called before the first malloc(). shutdown() must be called after the last
     * free(). sbk::engine::system owns this pairing (see system::create / system::destroy) so no
     * code outside the system's lifetime can allocate through sbk::memory.
     */
    /**@{*/
    auto init() -> void;
    auto shutdown() -> void;
    /**@}*/

    /**
     * @name Low-level allocation for all of Sound Bakery.
     */
    /**@{*/
    auto malloc(std::size_t size, std::size_t alignment, sbk::memory::object_category category) -> void*;
    auto realloc(void* pointer, std::size_t size) -> void*;
    auto free(void* pointer, sbk::memory::object_category category) -> void;
    /**@}*/

    /**
     * @name Sets up and tears down thread-local memory structures.
     */
    /**@{*/
    auto thread_start(std::string_view threadName) -> void;
    auto thread_end(std::string_view threadName) -> void;
    /**@}*/

    class memory_resource;

    /**
     * @brief The memory_resource owned by the current sbk::engine::system, or nullptr if no
     * system exists. Callers must treat nullptr as "allocation attempted outside system lifetime"
     * -- this is the only path polymorphic_allocator uses to reach the system, so no other bit
     * of Sound Bakery can allocate before system::create() or after system::destroy().
     */
    [[nodiscard]] auto system_default_resource() noexcept -> memory_resource*;

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
        memory_resource()                                           = default;
        virtual ~memory_resource()                                  = default;
        memory_resource(const memory_resource&)                     = delete;
        auto operator=(const memory_resource&) -> memory_resource&  = delete;
        memory_resource(memory_resource&&)                          = delete;
        auto operator=(memory_resource&&) -> memory_resource&       = delete;

        /**
         * @brief Public allocate function. Calls the derived @resource do_allocate internally.
         * @return memory address
         * @return nullptr on failure or OOM
         */
        [[nodiscard]] auto allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept -> void*
        {
            return do_allocate(bytes, alignment);
        }

        /**
         * @brief Free the memory at @resource pointer.
         * @param pointer memory to free
         * @param size size of pointer. Used for profiling more than actual freeing of memory
         */
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
        polymorphic_allocator() noexcept                                    = default;
        polymorphic_allocator(const polymorphic_allocator&) noexcept        = default;
        polymorphic_allocator(polymorphic_allocator&&) noexcept             = default;
        auto operator=(const polymorphic_allocator&) noexcept -> polymorphic_allocator& = default;
        auto operator=(polymorphic_allocator&&) noexcept -> polymorphic_allocator&      = default;
        ~polymorphic_allocator() noexcept                                   = default;

        explicit polymorphic_allocator(memory_resource* resource, const char* name = "sbk::polymorphic_allocator") noexcept
            : m_resource(resource), m_name(name)
        {
        }

        // EASTL name accessors (used only when EASTL_NAME_ENABLED).
        [[nodiscard]] auto get_name() const noexcept -> const char* { return m_name; }
        auto set_name(const char* name) noexcept -> void { m_name = name; }

        [[nodiscard]] auto get_resource() const noexcept -> memory_resource* { return m_resource; }

        // EASTL allocator interface. Each call resolves the resource lazily so that a
        // default-constructed allocator (as produced by EASTLAllocatorDefault) picks up the
        // currently-active sbk::engine::system. Allocating before sbk::engine::system::create()
        // is caller error; the allocate paths assert in debug and return nullptr in release,
        // which propagates to the caller as an sbk::result OOM.
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

    /**
     * @brief General-purpose memory_resource backed by sbk::memory::malloc/free (rpmalloc).
     */
    class rpmalloc_resource final : public memory_resource
    {
    public:
        rpmalloc_resource() = default;
        explicit rpmalloc_resource(sbk::memory::object_category category) noexcept : m_category(category) {}

    protected:
        auto do_allocate(std::size_t bytes, std::size_t alignment) noexcept -> void* override
        {
            return sbk::memory::malloc(bytes, alignment, m_category);
        }

        auto do_deallocate(void* pointer, std::size_t /*size*/, std::size_t /*alignment*/) noexcept -> void override
        {
            sbk::memory::free(pointer, m_category);
        }

    private:
        sbk::memory::object_category m_category{sbk::memory::object_category::unknown};
    };

    /**
     * @brief Bump-allocator memory_resource over a fixed rpmalloc-owned block.
     *
     * Analogue of std::pmr::monotonic_buffer_resource, but with our contract:
     *   - never throws; returns nullptr on exhaustion or any failure
     *   - no upstream fallback: once the block is full it stays full
     *   - deallocate() is a no-op; the block is released only when this resource is destroyed
     *
     * @remark Not thread-safe. Pair with the engine's thread-domain guards if shared.
     * @remark Must call @r init explicitly. Memory is not allocated by default in any constructor.
     */
    class monotonic_buffer_resource final : public memory_resource
    {
    public:
        monotonic_buffer_resource() = delete;

        monotonic_buffer_resource(sbk::memory::object_category category) noexcept : m_category(category) {}

        ~monotonic_buffer_resource() noexcept override
        {
            if (m_buffer != nullptr)
            {
                sbk::memory::free(m_buffer, m_category);
            }
        }

        monotonic_buffer_resource(const monotonic_buffer_resource&)                    = delete;
        auto operator=(const monotonic_buffer_resource&) -> monotonic_buffer_resource& = delete;

        /**
         * @brief Allocate memory for the arena and set up variables.
         * @param bufferSize size of the arena
         */
        auto init(std::size_t bufferSize) noexcept -> sbk::result<void>
        {
            SBK_CHECK(bufferSize > 0, SBK_ERR_INVALID_PARAMETER);
            m_buffer = static_cast<std::uint8_t*>(sbk::memory::malloc(bufferSize, default_alignment, m_category));
            SBK_CHECK(m_buffer != nullptr, SBK_ERR_OUT_OF_MEMORY);
            m_bump     = m_buffer;
            m_capacity = bufferSize;
            return sbk::ok();
        }

        [[nodiscard]] auto capacity() const noexcept -> std::size_t { return m_capacity; }
        [[nodiscard]] auto used() const noexcept -> std::size_t
        {
            return static_cast<std::size_t>(m_bump - m_buffer);
        }

    protected:
        auto do_allocate(std::size_t bytes, std::size_t alignment) noexcept -> void* override
        {
            if (m_buffer == nullptr || bytes == 0U)
            {
                return nullptr;
            }

            std::uint8_t* const aligned = align_up(m_bump, alignment);
            if (aligned == nullptr)
            {
                return nullptr;
            }

            const auto used = static_cast<std::size_t>(aligned - m_buffer);
            const bool overCapacity = used > m_capacity;
            const std::ptrdiff_t freeSpace = m_capacity - used;
            const bool noRoom              = bytes > freeSpace;

            if (overCapacity || noRoom)
            {
                return nullptr;
            }

            m_bump = offset_ptr(aligned, bytes);
            return aligned;
        }

        auto do_deallocate(void* /*pointer*/, std::size_t /*size*/, std::size_t /*alignment*/) noexcept -> void override
        {
            // Bump allocator: individual frees are no-ops. The whole block is released at destruction.
        }

    private:
        static auto align_up(std::uint8_t* p, std::size_t alignment) noexcept -> std::uint8_t*
        {
            if (alignment == 0U || (alignment & (alignment - 1U)) != 0U)
            {
                return nullptr;  // non-power-of-two alignment is invalid
            }
            const auto        addr    = reinterpret_cast<std::uintptr_t>(p);
            const std::size_t misalign = static_cast<std::size_t>(addr & (alignment - 1U));
            const std::size_t adjust  = misalign == 0U ? 0U : (alignment - misalign);
            return offset_ptr(p, adjust);
        }

        sbk::memory::object_category m_category{sbk::memory::object_category::unknown};
        std::uint8_t*      m_buffer{};
        std::uint8_t*      m_bump{};
        std::size_t        m_capacity{};
    };

    /**
     * @brief Deleter for sbk::owned_ptr. Runs the destructor and returns the block to the
     * memory_resource that produced it. The resource pointer is captured at construction, so
     * allocate/deallocate can never diverge.
     */
    template <typename T>
    struct owned_object_deleter
    {
        memory_resource* m_resource{};

        owned_object_deleter() = default;
        explicit owned_object_deleter(memory_resource& resource) noexcept : m_resource(&resource) {}

        template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
        owned_object_deleter(const owned_object_deleter<U>& other) noexcept : m_resource(other.m_resource)
        {
        }

        auto operator()(T* object) const noexcept -> void
        {
            static_assert(0 < sizeof(T), "can't delete an incomplete type");
            if (object == nullptr)
            {
                return;
            }
            object->~T();
            if (m_resource != nullptr)
            {
                m_resource->deallocate(object, sizeof(T), alignof(T));
            }
        }
    };
}  // namespace sbk::memory

namespace sbk
{
    /**
     * @brief A simple unique ptr that returns memory to its originating sbk::memory::memory_resource.
     */
    template <typename T>
    using owned_ptr = std::unique_ptr<T, sbk::memory::owned_object_deleter<T>>;
}
