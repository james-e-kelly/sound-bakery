#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/thread_domain.h"
#include "sound_bakery/error/result.h"
#include "sound_bakery/task/executor.h"

namespace sbk
{
    /**
     * @brief A multi-producer, single-consumer ring buffer that operates on bytes.
     * 
     * The class is created to address two problems:
     * 1. Speed of command queues
     * 2. Memory consumption
     * 
     * To address speed, the buffer:
     * - Is lock free. Acquiring a lock could be slow
     * - Uses a fixed-size buffer so no allocations or deallocations happen during push/pop
     * 
     * To address memory, the buffer:
     * - Allocates a fixed-pool once and never allocates memory on the heap for push or pop
     * - Does not use double or triple buffering
     * - Does not use lists
     * 
     * @remark The ring buffer size is rounded up to the nearest power of two. This is so indexes can be wrapped quickly and avoid the modulo operator that can be slow on some architectures.
     * @remark This is guaranteed to be thread-safe when there is only one consumer.
     * @remark The indexes grow infinitely but when indexing into the buffer, it is wrapped by a mask.
     * @remark Indexes are allowed to integer overflow.
     */
    class mpsc_ring_buffer final
    {
    public:
        mpsc_ring_buffer() noexcept = default;
        ~mpsc_ring_buffer() noexcept
        {
            if (m_buffer)
            {
                BOOST_ASSERT(m_memoryResource != nullptr);
                m_memoryResource->deallocate(m_buffer, m_capacity, sbk::memory::default_alignment);
                m_buffer = nullptr;
            }
            m_memoryResource = nullptr;
        }

        mpsc_ring_buffer(const mpsc_ring_buffer&) noexcept = delete;
        auto operator=(const mpsc_ring_buffer&) noexcept -> mpsc_ring_buffer& = delete;

        mpsc_ring_buffer(mpsc_ring_buffer&& other) noexcept
        {
            std::exchange(m_memoryResource, other.m_memoryResource);
            std::exchange(m_buffer, other.m_buffer);
            std::exchange(m_capacity, other.m_capacity);
            std::exchange(m_mask, other.m_mask);
            m_reserveWriteIndex.exchange(other.m_reserveWriteIndex);
            m_committedWriteIndex.exchange(other.m_committedWriteIndex);
            m_readIndex.exchange(other.m_readIndex);
        }

        [[nodiscard]] auto operator=(mpsc_ring_buffer&& other) noexcept -> mpsc_ring_buffer&
        {
            if (this != &other)
            {
                if (m_buffer)
                {
                    BOOST_ASSERT(m_memoryResource != nullptr);
                    m_memoryResource->deallocate(m_buffer, m_capacity, sbk::memory::default_alignment);
                    m_buffer = nullptr;
                    m_memoryResource = nullptr;
                }

                std::exchange(m_memoryResource, other.m_memoryResource);
                std::exchange(m_buffer, other.m_buffer);
                std::exchange(m_capacity, other.m_capacity);
                std::exchange(m_mask, other.m_mask);
                m_reserveWriteIndex.exchange(other.m_reserveWriteIndex);
                m_committedWriteIndex.exchange(other.m_committedWriteIndex);
                m_readIndex.exchange(other.m_readIndex);
            }
            return *this;
        }

        /**
         * @brief Initialize the buffer and indexes.
         * 
         * @param size requested buffer size, in bytes. Rounded up to nearest power of two
         */
        [[nodiscard]] auto init(std::size_t size, sbk::memory::memory_resource& allocator) noexcept -> sbk::result<> 
        {
            SBK_CHECK(m_buffer == nullptr, SBK_ERR_ALREADY_INITIALIZED);
            SBK_CHECK(m_capacity == 0, SBK_ERR_ALREADY_INITIALIZED);
            SBK_CHECK(size > 0, SBK_ERR_INVALID_PARAMETER);
            SBK_CHECK_MSG(size > 2, SBK_ERR_INVALID_PARAMETER, "Size was too small");
            SBK_CHECK_MSG(decltype(m_readIndex)::is_always_lock_free, SBK_ERR_SYSTEM, "Atomic was found to not be lock free");

            m_capacity = std::bit_ceil(size);
            m_mask     = m_capacity - 1;
            SBK_CHECK(std::_Is_pow_2(m_capacity), SBK_ERR_BAKERY);  // Probably impossible but better safe than sorry

            m_buffer   = static_cast<std::uint8_t*>(allocator.allocate(m_capacity, sbk::memory::default_alignment));
            SBK_CHECK(m_buffer != nullptr, SBK_ERR_OUT_OF_MEMORY);

            m_memoryResource = &allocator;
            
            return sbk::ok();
        }

        /**
         * @brief Write @r message into the buffer.
         * 
         * @remark Can be called from any producer thread.
         */
        [[nodiscard]] auto write(void* message, std::size_t messageSize) noexcept -> sbk_status
        {
            if (message == nullptr || messageSize == 0)
            {
                return SBK_ERR_INVALID_PARAMETER;
            }

            while (true)
            {
                const std::size_t read   = m_readIndex.load(std::memory_order_relaxed);
                std::size_t reserveWrite = m_reserveWriteIndex.load(std::memory_order_relaxed);

                const bool isFull = reserveWrite - read == m_capacity;
                const bool hasEnoughSpace = reserveWrite + messageSize - read <= m_capacity;

                if (isFull || !hasEnoughSpace)
                {
                    return SBK_ERR_BAKERY;
                }

                if (m_reserveWriteIndex.compare_exchange_weak(reserveWrite, reserveWrite + messageSize, std::memory_order_relaxed))
                {
                    std::memcpy(m_buffer + (reserveWrite & m_mask), message, messageSize);

                    std::size_t expected = reserveWrite;
                    while (!m_committedWriteIndex.compare_exchange_weak(expected, reserveWrite + messageSize, std::memory_order_release, std::memory_order_relaxed))
                    {
                        expected = reserveWrite;
                        std::this_thread::yield();
                    }
                    return SBK_SUCCESS;
                }
            }
        }

        /**
         * Read @r readBytes of data from the buffer into @r outBuffer.
         * 
         * @remark Must be called from the same consumer thread.
         */
        [[nodiscard]] auto read(void* outBuffer, std::size_t readBytes) noexcept -> sbk_status
        {
            if (readBytes == 0 || outBuffer == nullptr)
            {
                return SBK_ERR_INVALID_PARAMETER;
            }

            const std::size_t read = m_readIndex.load(std::memory_order_relaxed);
            const std::size_t committedWrite = m_committedWriteIndex.load(std::memory_order_acquire);

            const bool bufferEmpty = read == committedWrite;
            const bool canReadBytes = read + readBytes <= committedWrite;

            if (bufferEmpty || !canReadBytes)
            {
                return SBK_ERR_BAKERY;
            }

            std::memcpy(outBuffer, m_buffer + (read & m_mask), readBytes);

            m_readIndex.store(read + readBytes, std::memory_order_relaxed);

            return SBK_SUCCESS;
        }

        [[nodiscard]] auto get_capacity() const noexcept -> std::size_t
        {
            return m_capacity;
        }

    private:
        using atomic = std::atomic<std::size_t>;

        static_assert(atomic::is_always_lock_free);
        static constexpr std::size_t atomic_alignment = std::hardware_destructive_interference_size;

        sbk::memory::memory_resource* m_memoryResource{};

        std::uint8_t* m_buffer{};
        std::size_t m_capacity{};
        std::size_t m_mask{};

#pragma warning(disable : 4324)  // Disable "structure was padded due to alignment specifier"
        alignas (atomic_alignment) atomic m_reserveWriteIndex{0};
        alignas (atomic_alignment) atomic m_committedWriteIndex{0};
        alignas (atomic_alignment) atomic m_readIndex{0};
#pragma warning(default : 4324)
    };

    /**
     * @brief Queues tasks until it is "flushed" onto another executor.
     *
     * For Sound Bakery, this means queuing all commands from the game thread, or any thread, then flushing it to the system thread.
     */
    class command_queue : public executor
    {
    public:
        command_queue(std::string name) : executor(name) {}

        auto enqueue(work_item item) -> sbk::result<> override
        {
            ZoneScopedN("command_queue enqueue");
            const std::lock_guard lock(m_mutex);
            LockMark(m_mutex);
            SBK_CHECK_MSG(m_stopped == false, SBK_ERR_BAKERY, "Cannot enqueue command. Command queue shut down");
            m_staging.push_back(std::move(item));
            return sbk::ok();
        }

        /**
         * @brief Flush all tasks to the target executor.
         * @return SBK_SUCCESS if the command queue was empty, or successfully flushed
         */
        auto flush() -> sbk::result<> override
        {
            ZoneScopedN("command_queue flush");

            eastl::vector<work_item> batch;
            {
                const std::lock_guard lock(m_mutex);
                LockMark(m_mutex);
                m_staging.swap(batch);
            }

            if (batch.empty())
            {
                return sbk::ok();
            }

            return m_target->enqueue(work_item{
                [commands = std::move(batch)]() mutable
                {
                    const sbk::core::scoped_thread_domain studioDomain(sbk::core::thread_domain::studio);
                    ZoneScopedN("command_queue execute all commands");
                    for (auto& command : commands)
                    {
                        ZoneScopedN("command_queue execute command");
                        command();
                    }
                }});
        }

        /**
         * @brief Drop all staged commands and refuse further work. Does not flush.
         */
        auto abandon() -> void override
        {
            ZoneScopedN("command_queue abandon");
            const std::lock_guard lock(m_mutex);
            LockMark(m_mutex);
            m_stopped = true;
            eastl::vector<work_item> dropped;
            m_staging.swap(dropped);
        }

    private:
        executor* m_target{};
        TracyLockableN(std::mutex, m_mutex, "command_queue mutex");
        eastl::vector<work_item> m_staging;
        bool m_stopped = false;
        friend class ::sbk::engine::system;
    };
}  // namespace sbk
