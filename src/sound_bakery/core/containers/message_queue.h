#pragma once

#include "sound_bakery/core/containers/ring_buffer.h"

namespace sbk
{
    /**
     * @brief The enum, or something else, that defines the message's type (PostEvent, SetRTPC).
     */
    template<typename T>
    concept message_flag = std::is_integral_v<T>;

    template<typename T>
    concept pod = std::is_pod_v<T>;

    /**
     * @brief Lock-free, multi-producer, single-consumer queue of arbitrary sized messages.
     * 
     * Uses a @see mpsc_ring_buffer to handle reading and writing bytes in a thread-safe way.
     * 
     * The message queue uses @see reserve_write and @see commit_write on the ring buffer to ensure all messages are placed in contiguous blocks. This queue fills the end of buffers with special messages that makes the reader move past it.
     */
    template <message_flag message_flag_type = std::uint32_t>
    class message_queue final
    {
    public:
        /**
         * @brief A special flag that tells the reader to just read the message size and move on because it was the end of the buffer and the message didn't fit.
         */
        static constexpr message_flag_type s_skipFlag = message_flag_type{} - 1;

        struct message_header
        {
            message_flag_type m_type{};     //< The type of message, used by consumers to handle each message in a different way (HandlePostEvent / HandleSetRTPC etc)
            std::size_t m_messageSize{};    //< Message size, including the header
        };

        [[nodiscard]] auto init(std::size_t size, sbk::memory::memory_resource& allocator) noexcept -> sbk::result<>
        {
            SBK_CHECK_MSG(size > (sizeof(message_header) + sizeof(std::byte)), SBK_ERR_INVALID_PARAMETER, "The message queue needs to be big enough to hold a header and, at least, a small payload");

            return m_ringBuffer.init(size, allocator);
        }

        template <pod T>
        [[nodiscard]] auto write_message(const T& message) noexcept -> sbk_status
        {
            SBK_CHECK(message != nullptr, SBK_ERR_INVALID_PARAMETER);
            SBK_CHECK_MSG(message->m_type != s_skipFlag, SBK_ERR_INVALID_PARAMETER, "Users should not create messages with a flag equal to s_skipFlag. This should only be used by the message_queue to automically wrap around the end of the buffer");

            std::uint8_t* messageBuffer{};
            std::uint8_t* paddingBuffer{};
            std::size_t paddingSize{};
            std::size_t reserveIndex{};

            constexpr std::size_t size = sizeof(T);

            if (m_ringBuffer.reserve_write(&messageBuffer, size, &reserveIndex, &paddingBuffer, &paddingSize) == SBK_SUCCESS)
            {
                // Add a skip message into end of the buffer. Write the actual message as normal
                if (paddingSize > 0U)
                {
                    BOOST_ASSERT(paddingSize >= sizeof(message_header));

                    message_header skipHeader{.m_type = s_skipFlag, .m_messageSize = paddingSize};
                    std::memcpy(paddingBuffer, &skipHeader, sizeof(message_header));
                }

                std::memcpy(messageBuffer, &message, size);

                SBK_STATUS_TRY_C(m_ringBuffer.commit_write(reserveIndex, size));
            }
            return SBK_SUCCESS;
        }

        [[nodiscard]] auto read_begin(message_header* header, std::size_t* outReadIndex) noexcept -> sbk_status
        {
            SBK_CHECK(header != nullptr, SBK_ERR_INVALID_PARAMETER);
            SBK_CHECK(outReadIndex != nullptr, SBK_ERR_INVALID_PARAMETER);
            
            std::uint8_t* messageBuffer{};
            std::size_t readIndex{};
            std::size_t bytesRead{};

            constexpr std::size_t size = sizeof(message_header);

            SBK_STATUS_TRY_C(m_ringBuffer.read_begin(&messageBuffer, &readIndex, size, &bytesRead));
            
            // If we reached the end of the buffer:
            // - Advance the read pointer by the skip message's size
            // - Read again to the user is given the actual message
            if (bytesRead < size)
            {
                header = static_cast<message_header*>(messageBuffer);
                BOOST_ASSERT(header->m_type == s_skipFlag);
                m_ringBuffer.read_end(readIndex, header->m_messageSize - size); 
                 
                if (m_ringBuffer.read_begin(&messageBuffer, &readIndex, size, &bytesRead))
                {
                    BOOST_ASSERT(bytesRead == size);
                }
            }

            header = static_cast<message_header*>(messageBuffer);
            *outReadIndex = readIndex;
            return SBK_SUCCESS;
        }

        [[nodiscard]] auto read_end(const message_header* header, const std::size_t readIndex) noexcept -> sbk_status
        {
            SBK_CHECK(header != nullptr, SBK_ERR_INVALID_PARAMETER);

            return m_ringBuffer.read_end(readIndex, header->m_messageSize - sizeof(message_header));
        }

    private:
        mpsc_ring_buffer m_ringBuffer;
    };
}
