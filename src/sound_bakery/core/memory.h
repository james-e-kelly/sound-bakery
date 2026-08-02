#pragma once

#include "sound_bakery/core/allocator.h"
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
