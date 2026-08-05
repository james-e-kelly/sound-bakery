#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/error/result.h"
#include "sound_bakery/core/memory/memory.h"

namespace sbk
{
    /**
     * @brief A multi-producer, single-consumer ring buffer that operates on bytes.
     *
     * The class is created to address two problems:
     * 1. Speed of command queues
     * 2. Memory allocation and deallocation
     *
     * To address speed, the buffer:
     * - Is lock free. Acquiring a lock could be slow
     * - Uses a fixed-size buffer so no allocations or deallocations happen during push/pop
     *
     * To address memory, the buffer:
     * - Allocates a fixed-pool once and never allocates memory on the heap for write or read
     * - Does not use double or triple buffering
     * - Does not use lists
     *
     * @remark The ring buffer size is rounded up to the nearest power of two. This is so indexes can be wrapped quickly and avoid the modulo operator that can be slow on some architectures.
     * @remark This is guaranteed to be thread-safe when there is only one consumer.
     * @remark The indexes grow infinitely but when indexing into the buffer, it is wrapped by a mask.
     * @remark Indexes are allowed to integer overflow.
     * @remark To avoid straddled writes, writes can be split up. Given an 8-byte buffer, allocating 6 bytes, then 4, will write 2 bytes, then 2 bytes at the start of the buffer.
     * Consuming containers should consider fixed-size messages to avoid this.
     *
     * @see https://github.com/bowtoyourlord/MPSCQueue
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

        mpsc_ring_buffer(const mpsc_ring_buffer&) noexcept                    = delete;
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
                    m_buffer         = nullptr;
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

            m_capacity            = std::bit_ceil(size);
            m_mask                = m_capacity - 1;
            SBK_CHECK(sbk::memory::is_pow_2(m_capacity), SBK_ERR_BAKERY);  // Probably impossible but better safe than sorry

            m_buffer = static_cast<std::uint8_t*>(allocator.allocate(m_capacity, sbk::memory::default_alignment));
            SBK_CHECK(m_buffer != nullptr, SBK_ERR_OUT_OF_MEMORY);

            m_memoryResource = &allocator;

            return sbk::ok();
        }

        /**
         * @brief Reserve a pointer to a contiguous region of the buffer for writing by the user.
         * 
         * Will try to reserve a contiguous region. If it can't (at the end of the buffer), it will reserve a padding + size, so that the user can fill the padding (end of buffer) with their own message.
         * 
         * @see write for a function that splits memory when the size can't easily fit into the buffer.
         * 
         * @remark Can be called from any producer thread.
         */
        [[nodiscard]] auto reserve_write(std::uint8_t** outBuffer, std::size_t size, std::size_t* outReserveIndex, std::uint8_t** outPadding, std::size_t* outPaddingSize) noexcept -> sbk_status
        {
            if (outBuffer == nullptr || outPadding == nullptr || outReserveIndex == nullptr || outPaddingSize == nullptr || size == 0 || size > m_capacity)
            {
                return SBK_ERR_INVALID_PARAMETER;
            }

            if (m_buffer == nullptr || m_capacity == 0)
            {
                return SBK_ERR_UNITIALIZED;
            }

            *outPadding     = nullptr;
            *outPaddingSize = 0U;

            while (true)
            {
                const std::size_t read   = m_readIndex.load(std::memory_order_relaxed);
                std::size_t reserveWrite = m_reserveWriteIndex.load(std::memory_order_relaxed);

                if (is_buffer_full(reserveWrite, read))
                {
                    // The buffer is completely full. The reader needs to read more bytes
                    return SBK_ERR_FULL;
                }

                const std::size_t writeOffset = reserveWrite & m_mask;
                const std::size_t firstBlockSize        = std::min(size, m_capacity - writeOffset);
                const std::size_t spaceFromWriteTillEnd = m_capacity - writeOffset;

                const bool needsWrapping = firstBlockSize < size;

                const std::size_t reserveSize = size + needsWrapping ? spaceFromWriteTillEnd : 0U;

                if (!can_reserve_bytes(reserveWrite, read, reserveSize))
                {
                    // There is space in the buffer but this message was too large
                    return SBK_ERR_TOO_LARGE;
                }

                if (m_reserveWriteIndex.compare_exchange_weak(reserveWrite, reserveWrite + reserveSize, std::memory_order_relaxed))
                {
                    *outReserveIndex = reserveWrite;

                    if (needsWrapping)
                    {
                        *outBuffer = m_buffer;
                        
                        *outPadding = m_buffer + writeOffset;
                        *outPaddingSize = firstBlockSize;
                    }
                    else
                    {
                        *outBuffer = m_buffer + writeOffset;
                    }

                    return SBK_SUCCESS;
                }
            }
        }

        /**
         * @brief Commits @r size so the reader can read the newly written values.
         * 
         * Expects @r reserve_write to be called before this.
         * 
         * @remark Can be called from any producer thread.
         */
        [[nodiscard]] auto commit_write(std::size_t reserveIndex, std::size_t size) noexcept -> sbk_status
        {
            std::size_t expected = reserveIndex;
            while (!m_committedWriteIndex.compare_exchange_weak(expected, reserveIndex + size, std::memory_order_release, std::memory_order_relaxed))
            {
                expected = reserveIndex;
                std::this_thread::yield();
            }
        }

        /**
         * @brief Write @r message into the buffer.
         *
         * @remark Can be called from any producer thread.
         */
        [[nodiscard]] auto write(const void* message, std::size_t size) noexcept -> sbk_status
        {
            if (message == nullptr || size == 0 || size > m_capacity)
            {
                return SBK_ERR_INVALID_PARAMETER;
            }

            if (m_buffer == nullptr || m_capacity == 0)
            {
                return SBK_ERR_UNITIALIZED;
            }

            while (true)
            {
                const std::size_t read   = m_readIndex.load(std::memory_order_relaxed);
                std::size_t reserveWrite = m_reserveWriteIndex.load(std::memory_order_relaxed);

                if (is_buffer_full(reserveWrite, read))
                {
                    // The buffer is completely full. The reader needs to read more bytes
                    return SBK_ERR_FULL;
                }

                if (!can_reserve_bytes(reserveWrite, read, size))
                {
                    // There is space in the buffer but this message was too large
                    return SBK_ERR_TOO_LARGE;
                }

                if (m_reserveWriteIndex.compare_exchange_weak(reserveWrite, reserveWrite + size, std::memory_order_relaxed))
                {
                    const std::size_t writeOffset    = reserveWrite & m_mask;
                    const std::size_t firstChunkSize = std::min(size, m_capacity - writeOffset);
                    const bool needsWrapping         = firstChunkSize < size;

                    std::memcpy(m_buffer + writeOffset, message, firstChunkSize);
                    if (needsWrapping)
                    {
                        std::memcpy(m_buffer, static_cast<const std::uint8_t*>(message) + firstChunkSize, size - firstChunkSize);
                    }

                    std::size_t expected = reserveWrite;
                    while (!m_committedWriteIndex.compare_exchange_weak(expected, reserveWrite + size, std::memory_order_release, std::memory_order_relaxed))
                    {
                        expected = reserveWrite;
                        std::this_thread::yield();
                    }
                    return SBK_SUCCESS;
                }
            }
        }

        /**
         * @brief Tries to read @r size bytes. If the size would go past the end of the buffer, it reads only the remaining size, and sets @r outActualSize to the number of bytes it read.
         */
        [[nodiscard]] auto read_begin(std::uint8_t** outBuffer, std::size_t* outReadIndex, std::size_t size, std::size_t* outActualSize) noexcept -> sbk_status
        {
            if (outBuffer == nullptr || outReadIndex == nullptr || outActualSize == nullptr || size == 0 || size > m_capacity)
            {
                return SBK_ERR_INVALID_PARAMETER;
            }

            if (m_buffer == nullptr || m_capacity == 0)
            {
                return SBK_ERR_UNITIALIZED;
            }

            const std::size_t read           = m_readIndex.load(std::memory_order_relaxed);
            const std::size_t committedWrite = m_committedWriteIndex.load(std::memory_order_acquire);

            if (is_buffer_empty(committedWrite, read))
            {
                // The reader is completely caught up to the writers. Producers need to write more data
                return SBK_ERR_EMPTY;
            }

            if (!can_read_bytes(committedWrite, read, size))
            {
                // There is data to be read, but this request was too large
                return SBK_ERR_TOO_LARGE;
            }

            const std::size_t readOffset     = read & m_mask;
            const std::size_t firstChunkSize = std::min(size, m_capacity - readOffset);

            *outBuffer = m_buffer + readOffset;
            *outReadIndex = read;
            *outActualSize = firstChunkSize;

            return SBK_SUCCESS;
        }

        /**
         * @brief Stores the read index so producers can see how much space is in the buffer.
         */
        [[nodiscard]] auto read_end(std::size_t readIndex, std::size_t size) noexcept -> sbk_status
        {
            m_readIndex.store(readIndex + size, std::memory_order_relaxed);
            return SBK_SUCCESS;
        }

        /**
         * Read @r size of data from the buffer into @r outBuffer.
         *
         * @remark Must be called from the same consumer thread.
         */
        [[nodiscard]] auto read(void* outBuffer, std::size_t size) noexcept -> sbk_status
        {
            if (size == 0 || outBuffer == nullptr || size > m_capacity)
            {
                return SBK_ERR_INVALID_PARAMETER;
            }

            if (m_buffer == nullptr || m_capacity == 0)
            {
                return SBK_ERR_UNITIALIZED;
            }

            const std::size_t read           = m_readIndex.load(std::memory_order_relaxed);
            const std::size_t committedWrite = m_committedWriteIndex.load(std::memory_order_acquire);

            if (is_buffer_empty(committedWrite, read))
            {
                // The reader is completely caught up to the writers. Producers need to write more data
                return SBK_ERR_EMPTY;
            }

            if (!can_read_bytes(committedWrite, read, size))
            {
                // There is data to be read, but this request was too large
                return SBK_ERR_TOO_LARGE;
            }

            const std::size_t readOffset     = read & m_mask;
            const std::size_t firstChunkSize = std::min(size, m_capacity - readOffset);
            const bool needsWrapping         = firstChunkSize < size;

            std::memcpy(outBuffer, m_buffer + readOffset, firstChunkSize);
            if (needsWrapping)
            {
                std::memcpy(static_cast<std::uint8_t*>(outBuffer) + firstChunkSize, m_buffer + readOffset + firstChunkSize, size - firstChunkSize);
            }

            m_readIndex.store(read + size, std::memory_order_relaxed);

            return SBK_SUCCESS;
        }

        /**
         * @brief Move the read index forward without reading or copying anything.
         */
        [[nodiscard]] auto advance_read_index(std::size_t size) noexcept -> sbk_status
        {
            if (size == 0 || size > m_capacity)
            {
                return SBK_ERR_INVALID_PARAMETER;
            }

            if (m_buffer == nullptr || m_capacity == 0)
            {
                return SBK_ERR_UNITIALIZED;
            }

            const std::size_t read           = m_readIndex.load(std::memory_order_relaxed);
            const std::size_t committedWrite = m_committedWriteIndex.load(std::memory_order_acquire);

            if (is_buffer_empty(committedWrite, read))
            {
                // The reader is completely caught up to the writers. Producers need to write more data
                return SBK_ERR_EMPTY;
            }

            if (!can_read_bytes(committedWrite, read, size))
            {
                // There is data to be read, but this request was too large
                return SBK_ERR_TOO_LARGE;
            }

            m_readIndex.store(read + size, std::memory_order_relaxed);

            return SBK_SUCCESS;
        }

        [[nodiscard]] auto get_capacity() const noexcept -> std::size_t
        {
            return m_capacity;
        }

    private:
        [[nodiscard]] inline auto is_buffer_full(const std::size_t reserveWrite, const std::size_t read) const noexcept -> bool
        {
            return reserveWrite - read == m_capacity;
        }

        [[nodiscard]] inline auto is_buffer_empty(const std::size_t commitedWrite, const std::size_t read) const noexcept -> bool
        {
            return commitedWrite == read;
        }

        [[nodiscard]] inline auto can_reserve_bytes(const std::size_t reserveWrite, const std::size_t read, const std::size_t size) const noexcept -> bool
        {
            return reserveWrite + size - read <= m_capacity;
        }

        [[nodiscard]] inline auto can_read_bytes(const std::size_t committedWrite, const std::size_t read, const std::size_t size) const noexcept -> bool
        {
            return read + size <= committedWrite;
        }

        static_assert(std::atomic<std::size_t>::is_always_lock_free);

        sbk::memory::memory_resource* m_memoryResource{};

        std::uint8_t* m_buffer{};
        std::size_t m_capacity{};
        std::size_t m_mask{};

#pragma warning(disable : 4324)  // Disable "structure was padded due to alignment specifier"
        alignas(sbk::memory::hardware_destructive_interference_size) std::atomic<std::size_t> m_reserveWriteIndex{0};
        alignas(sbk::memory::hardware_destructive_interference_size) std::atomic<std::size_t> m_committedWriteIndex{0};
        alignas(sbk::memory::hardware_destructive_interference_size) std::atomic<std::size_t> m_readIndex{0};
#pragma warning(default : 4324)
    };
}
