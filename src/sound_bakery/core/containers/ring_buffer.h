#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/error/result.h"
#include "sound_bakery/core/memory/memory.h"

namespace sbk
{
    /**
     * @brief Multi-producer, single-consumer byte ring buffer.
     *
     * Producers CAS-reserve a slot on @ref m_reserveWriteIndex, memcpy their
     * bytes, then publish by advancing @ref m_committedWriteIndex from their
     * reserved position to (position + size). Publication is strictly in reserve
     * order: a slow producer holds up any producer that reserved after it,
     * until it commits. In return the consumer only reads one atomic and knows
     * everything before that point is valid. Alternative schemes (per-slot
     * committed flags, Vyukov's bounded MPSC) trade the producer spin for
     * consumer or metadata complexity; for short command messages drained once
     * per update the spin window is memcpy-sized and this design wins.
     *
     * @warning A producer that reserves and never commits freezes every
     * producer behind it. Only well-behaved producers should hold the queue.
     *
     * @remark Capacity is rounded up to a power of two so the index can be
     * masked.
     * @remark Indexes grow forever; wrap-around is beyond any realistic runtime
     * and all occupancy comparisons are wrap-safe unsigned differences.
     *
     * @see https://github.com/bowtoyourlord/MPSCQueue
     */
    class mpsc_ring_buffer final
    {
    public:
        mpsc_ring_buffer() noexcept = default;
        ~mpsc_ring_buffer() noexcept
        {
            if (m_buffer != nullptr)
            {
                BOOST_ASSERT(m_memoryResource != nullptr);
                m_memoryResource->deallocate(m_buffer, m_capacity, sbk::memory::default_alignment);
                m_buffer = nullptr;
            }
            m_memoryResource = nullptr;
        }

        mpsc_ring_buffer(const mpsc_ring_buffer&)                    = delete;
        auto operator=(const mpsc_ring_buffer&) -> mpsc_ring_buffer& = delete;
        mpsc_ring_buffer(mpsc_ring_buffer&&)                         = delete;
        auto operator=(mpsc_ring_buffer&&) -> mpsc_ring_buffer&      = delete;

        /**
         * @brief Initialise a ring buffer.
         *
         * @param size      Requested buffer size in bytes. Rounded up to the next power of two.
         * @param allocator Memory resource used for the backing storage.
         */
        [[nodiscard]] auto init(std::size_t size, sbk::memory::memory_resource& allocator) noexcept -> sbk::result<>
        {
            SBK_CHECK(m_buffer == nullptr, SBK_ERR_ALREADY_INITIALIZED);
            SBK_CHECK(m_capacity == 0, SBK_ERR_ALREADY_INITIALIZED);
            SBK_CHECK_MSG(size > 2, SBK_ERR_INVALID_PARAMETER, "Size was too small");
            SBK_CHECK_MSG(decltype(m_readIndex)::is_always_lock_free, SBK_ERR_SYSTEM, "Atomic was found to not be lock free");

            m_capacity = std::bit_ceil(size);
            m_mask     = m_capacity - 1;
            SBK_CHECK(sbk::memory::is_pow_2(m_capacity), SBK_ERR_BAKERY);

            m_buffer = static_cast<std::uint8_t*>(allocator.allocate(m_capacity, sbk::memory::default_alignment));
            SBK_CHECK(m_buffer != nullptr, SBK_ERR_OUT_OF_MEMORY);

            m_memoryResource = &allocator;

            m_reserveWriteIndex.store(0, std::memory_order_relaxed);
            m_committedWriteIndex.store(0, std::memory_order_relaxed);
            m_readIndex.store(0, std::memory_order_relaxed);

            return sbk::ok();
        }

        /**
         * @brief Reserve a contiguous region for a producer to fill.
         *
         * When the request straddles the end of the buffer, the reservation
         * also covers the padding at the end. The producer's message goes into
         * @r outBuffer (at the start of the buffer for a wrap); @r outPadding
         * lets the caller write a skip marker over the padding so the consumer
         * knows to jump past it.
         *
         * @remark Callable from any producer thread.
         */
        [[nodiscard]] auto reserve_write(std::uint8_t** outBuffer, std::size_t size, std::size_t* outReserveIndex, std::uint8_t** outPadding, std::size_t* outPaddingSize) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK(outBuffer != nullptr && outReserveIndex != nullptr && outPadding != nullptr && outPaddingSize != nullptr && size != 0, SBK_ERR_INVALID_PARAMETER);
            SBK_STATUS_CHECK(m_buffer != nullptr && m_capacity > 0, SBK_ERR_UNINITIALIZED);
            SBK_STATUS_CHECK(size <= m_capacity, SBK_ERR_INVALID_PARAMETER);

            *outPadding     = nullptr;
            *outPaddingSize = 0U;

            for (;;)
            {
                // Acquire pairs with the reader's release-store on m_readIndex.
                const std::size_t read         = m_readIndex.load(std::memory_order_acquire);
                std::size_t       reserveWrite = m_reserveWriteIndex.load(std::memory_order_relaxed);

                const std::size_t writeOffset           = reserveWrite & m_mask;
                const std::size_t spaceFromWriteTillEnd = m_capacity - writeOffset;
                const bool        needsWrapping         = size > spaceFromWriteTillEnd;

                // Wrap reservations pay for the end-of-buffer padding too.
                const std::size_t reserveSize = needsWrapping ? (size + spaceFromWriteTillEnd) : size;

                SBK_STATUS_CHECK(can_reserve_bytes(reserveWrite, read, reserveSize), SBK_ERR_FULL);

                if (m_reserveWriteIndex.compare_exchange_weak(reserveWrite, reserveWrite + reserveSize, std::memory_order_relaxed))
                {
                    *outReserveIndex = reserveWrite;

                    if (needsWrapping)
                    {
                        *outBuffer      = m_buffer;
                        *outPadding     = m_buffer + writeOffset;
                        *outPaddingSize = spaceFromWriteTillEnd;
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
         * @brief Publish a reservation returned by @ref reserve_write.
         *
         * @param size Bytes to publish, including any padding returned by the reserve.
         *
         * @warning Every reserve MUST be paired with a commit.
         *
         * @remark Callable from any producer thread.
         */
        [[nodiscard]] auto commit_write(std::size_t reserveIndex, std::size_t size) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK(m_buffer != nullptr && m_capacity > 0, SBK_ERR_UNINITIALIZED);

            // Publish in reserve order - see the commit-ordering note on the class.
            std::size_t expected = reserveIndex;
            while (!m_committedWriteIndex.compare_exchange_weak(expected, reserveIndex + size, std::memory_order_release, std::memory_order_relaxed))
            {
                expected = reserveIndex;
                std::this_thread::yield();
            }
            return SBK_SUCCESS;
        }

        /**
         * @brief Reserve, memcpy, and commit in one call.
         *
         * Splits the message across the end/start of the buffer if it doesn't
         * fit contiguously. @ref read handles the straddle automatically;
         * @ref read_begin callers must expect a partial return.
         *
         * @remark Callable from any producer thread.
         */
        [[nodiscard]] auto write(const void* message, std::size_t size) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK(message != nullptr && size != 0, SBK_ERR_INVALID_PARAMETER);
            SBK_STATUS_CHECK(m_buffer != nullptr && m_capacity > 0, SBK_ERR_UNINITIALIZED);
            SBK_STATUS_CHECK(size <= m_capacity, SBK_ERR_INVALID_PARAMETER);

            for (;;)
            {
                const std::size_t read         = m_readIndex.load(std::memory_order_acquire);
                std::size_t       reserveWrite = m_reserveWriteIndex.load(std::memory_order_relaxed);

                SBK_STATUS_CHECK(can_reserve_bytes(reserveWrite, read, size), SBK_ERR_FULL);

                if (m_reserveWriteIndex.compare_exchange_weak(reserveWrite, reserveWrite + size, std::memory_order_relaxed))
                {
                    const std::size_t writeOffset    = reserveWrite & m_mask;
                    const std::size_t firstChunkSize = std::min(size, m_capacity - writeOffset);
                    const bool        needsWrapping  = firstChunkSize < size;

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
         * @brief Zero-copy read. Returns a pointer into the buffer.
         *
         * If the request straddles the buffer end, only the bytes up to the
         * end are returned via @r outActualSize; the consumer must call again
         * for the rest.
         *
         * @remark Consumer thread only.
         */
        [[nodiscard]] auto read_begin(std::uint8_t** outBuffer, std::size_t* outReadIndex, std::size_t size, std::size_t* outActualSize) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK(outBuffer != nullptr && outReadIndex != nullptr && outActualSize != nullptr && size != 0, SBK_ERR_INVALID_PARAMETER);
            SBK_STATUS_CHECK(m_buffer != nullptr && m_capacity > 0, SBK_ERR_UNINITIALIZED);
            SBK_STATUS_CHECK(size <= m_capacity, SBK_ERR_INVALID_PARAMETER);

            const std::size_t read = m_readIndex.load(std::memory_order_relaxed);
            // Acquire pairs with the producer's release-store on m_committedWriteIndex.
            const std::size_t committedWrite = m_committedWriteIndex.load(std::memory_order_acquire);

            SBK_STATUS_CHECK(!is_buffer_empty(committedWrite, read), SBK_ERR_EMPTY);
            SBK_STATUS_CHECK(can_read_bytes(committedWrite, read, size), SBK_ERR_TOO_LARGE);

            const std::size_t readOffset     = read & m_mask;
            const std::size_t firstChunkSize = std::min(size, m_capacity - readOffset);

            *outBuffer     = m_buffer + readOffset;
            *outReadIndex  = read;
            *outActualSize = firstChunkSize;
            return SBK_SUCCESS;
        }

        /** 
         * @remark Consumer thread only. 
         */
        [[nodiscard]] auto read_end(std::size_t size) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK(m_buffer != nullptr && m_capacity > 0, SBK_ERR_UNINITIALIZED);
            // Release so producers that acquire-load m_readIndex see the freed space.
            m_readIndex.fetch_add(size, std::memory_order_release);
            return SBK_SUCCESS;
        }

        /**
         * @brief Copy @r size bytes out of the buffer, handling any straddle.
         *
         * @remark Consumer thread only.
         */
        [[nodiscard]] auto read(void* outBuffer, std::size_t size) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK(outBuffer != nullptr && size != 0, SBK_ERR_INVALID_PARAMETER);
            SBK_STATUS_CHECK(m_buffer != nullptr && m_capacity > 0, SBK_ERR_UNINITIALIZED);
            SBK_STATUS_CHECK(size <= m_capacity, SBK_ERR_INVALID_PARAMETER);

            const std::size_t read           = m_readIndex.load(std::memory_order_relaxed);
            const std::size_t committedWrite = m_committedWriteIndex.load(std::memory_order_acquire);

            SBK_STATUS_CHECK(!is_buffer_empty(committedWrite, read), SBK_ERR_EMPTY);
            SBK_STATUS_CHECK(can_read_bytes(committedWrite, read, size), SBK_ERR_TOO_LARGE);

            const std::size_t readOffset     = read & m_mask;
            const std::size_t firstChunkSize = std::min(size, m_capacity - readOffset);
            const bool        needsWrapping  = firstChunkSize < size;

            std::memcpy(outBuffer, m_buffer + readOffset, firstChunkSize);
            if (needsWrapping)
            {
                std::memcpy(static_cast<std::uint8_t*>(outBuffer) + firstChunkSize, m_buffer, size - firstChunkSize);
            }

            m_readIndex.store(read + size, std::memory_order_release);
            return SBK_SUCCESS;
        }

        /**
         * @remark Consumer thread only. 
         */
        [[nodiscard]] auto advance_read_index(std::size_t size) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK(size != 0, SBK_ERR_INVALID_PARAMETER);
            SBK_STATUS_CHECK(m_buffer != nullptr && m_capacity > 0, SBK_ERR_UNINITIALIZED);
            SBK_STATUS_CHECK(size <= m_capacity, SBK_ERR_INVALID_PARAMETER);

            const std::size_t read           = m_readIndex.load(std::memory_order_relaxed);
            const std::size_t committedWrite = m_committedWriteIndex.load(std::memory_order_acquire);

            SBK_STATUS_CHECK(!is_buffer_empty(committedWrite, read), SBK_ERR_EMPTY);
            SBK_STATUS_CHECK(can_read_bytes(committedWrite, read, size), SBK_ERR_TOO_LARGE);

            m_readIndex.store(read + size, std::memory_order_release);
            return SBK_SUCCESS;
        }

        [[nodiscard]] auto get_capacity() const noexcept -> std::size_t { return m_capacity; }

    private:
        [[nodiscard]] inline auto is_buffer_empty(std::size_t committedWrite, std::size_t read) const noexcept -> bool
        {
            return committedWrite == read;
        }

        [[nodiscard]] inline auto can_reserve_bytes(std::size_t reserveWrite, std::size_t read, std::size_t size) const noexcept -> bool
        {
            return (reserveWrite - read) + size <= m_capacity;
        }

        [[nodiscard]] inline auto can_read_bytes(std::size_t committedWrite, std::size_t read, std::size_t size) const noexcept -> bool
        {
            return (committedWrite - read) >= size;
        }

        static_assert(std::atomic<std::size_t>::is_always_lock_free);

        sbk::memory::memory_resource* m_memoryResource{};

        std::uint8_t* m_buffer{};
        std::size_t   m_capacity{};
        std::size_t   m_mask{};

#pragma warning(disable : 4324)  // Disable "structure was padded due to alignment specifier"
        alignas(sbk::memory::hardware_destructive_interference_size) std::atomic<std::size_t> m_reserveWriteIndex{0};
        alignas(sbk::memory::hardware_destructive_interference_size) std::atomic<std::size_t> m_committedWriteIndex{0};
        alignas(sbk::memory::hardware_destructive_interference_size) std::atomic<std::size_t> m_readIndex{0};
#pragma warning(default : 4324)
    };
}  // namespace sbk
