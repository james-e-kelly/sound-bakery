#pragma once

#include "sound_bakery/core/containers/ring_buffer.h"

namespace sbk
{
    /**
     * @brief The enum, or something else, that defines the message's type.
     */
    template<typename T>
    concept message_type = std::is_integral_v<T> || std::is_enum_v<T>;

    /**
     * @brief User messages must be POD types so they can be memcpy'd easily.
     */
    template<typename T>
    concept pod = std::is_trivial_v<T>;

    template <typename T>
    struct numeric_type {
        using type = T;
    };

    template <typename T>
    requires std::is_enum_v<T>
    struct numeric_type<T> {
        using type = std::underlying_type_t<T>;
    };
    
    /**
     * @brief Lock-free, multi-producer, single-consumer queue of arbitrary sized messages.
     * 
     * Messages are identified by a "message header" that contains a type and payload size.
     * The type can be any integral type, or an enum. The queue handles all conversions for the user.
     * The payload size is used to store size of the user's section of the message. 
     * 
     * On every read, the queue tries to read `sizeof(message_header)`.
     * Once it has the header, it reads a further number of bytes, equal to the payload size.
     * The data is returned to the user as a @s message_view. 
     * This gives the user the pointer into the buffer for reading, and the message type.
     * 
     * Users are expected to cast `payload` to their message type:
     * `const start_message* startMessage = reinterpret_cast<const start_message*>(view.payload);`
     * 
     * When writing, the write function reserves space in the buffer, then memcpy's the message into the buffer.
     * This makes it relatively simple to add data into the queue:
     * `messageQueue.write_message(message_type::start, start_message{.a = 7, .b = 9});`
     * 
     * The message type is not deduced from the message structure, so the user must pass in the type explicitly.
     * 
     * The queue uses a @see mpsc_ring_buffer to handle reading and writing bytes in a thread-safe way.
     * 
     * @remark To ensure payloads are returned to the user as contiguous blocks of memory, messages "wrap".
     * When a message won't fit at the end of the buffer, the end of the buffer is filled with a special "skip" header.
     * This skip message tells the reader (automatically, no concern for the user) to read all bytes at the end of the buffer.
     * The user's message is then written to the start of the buffer and the user is given a contiguous block of memory when reading.
     */
    template <message_type T = std::uint32_t>
    class message_queue final
    {
        using numeric_message_type = typename numeric_type<T>::type;
        using message_size_type = std::uint8_t;   //< Keep it as small as possible

        /**
         * @brief A special flag that tells the reader to just read the message size and move on because it was the end of the buffer and the message didn't fit.
         */
        static constexpr numeric_message_type s_skipFlag = std::numeric_limits<numeric_message_type>::max();

        /**
         * @brief Basic header of all messages. Written to the buffer.
         */
        struct message_header
        {
            numeric_message_type m_type{};      //< The type of message, used by consumers to handle each message in a different way (HandlePostEvent / HandleSetRTPC etc)
            message_size_type m_payloadSize{};  //< Message size, including the header
        };

        template <typename T>
        constexpr auto to_numeric(T value) noexcept
        {
            if constexpr (std::is_enum_v<T>)
            {
                return static_cast<std::underlying_type_t<T>>(value);
            }
            else
            {
                return static_cast<T>(value);
            }
        }

    public:
        /**
         * @brief Returned data from the buffer. Not the actual raw data that was inside the buffer - just a pointer to it.
         */
        struct message_view
        {
            T m_type{};                         //< The type of message, used by consumers to handle each message and cast the payload to the correct type
            message_size_type m_payloadSize{};  //< Payload/message size. Used by read_end. Should not be edited
            const void* payload{};              //< Pointer to the message payload. Can be null
        };

        /**
         * @brief Initialize the message queue.
         * @param size size of the queue in bytes
         * @return SBK_ERR_INVALID_PARAMETER if the size is not large enough to fit the message header
         */
        [[nodiscard]] auto init(std::size_t size, sbk::memory::memory_resource& allocator) noexcept -> sbk::result<>
        {
            SBK_CHECK_MSG(size >= get_header_size(), SBK_ERR_INVALID_PARAMETER, "The message queue needs to be big enough to hold a header and, at least, a small payload");

            return m_ringBuffer.init(size, allocator);
        }

        /**
         * @brief Write a message to the queue.
         * 
         * The write function automatically handles wrapping the messages and ensuring messages are placed in contiguous blocks of memory.
         * 
         * @tparam U user message type
         * @param type type of message
         * @param message message data
         */
        template <pod U>
        [[nodiscard]] auto write_message(T type, const U& message) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK_MSG(to_numeric(type) != s_skipFlag, SBK_ERR_INVALID_PARAMETER, "Users should not create messages with a flag equal to s_skipFlag. This should only be used by the message_queue to automically wrap around the end of the buffer");

            std::uint8_t* messageBuffer{};
            std::uint8_t* paddingBuffer{};
            std::size_t paddingSize{};
            std::size_t reserveIndex{};

            message_header header{.m_type = to_numeric(type), .m_payloadSize = sizeof(U)};
            SBK_STATUS_TRY_C(m_ringBuffer.reserve_write(&messageBuffer, sizeof(message_header) + header.m_payloadSize, &reserveIndex, &paddingBuffer, &paddingSize));

            // If we were given padding, it means the buffer is split
            // Add a skip message into end of the buffer. Write the actual message as normal
            if (paddingSize > 0U)
            {
                BOOST_ASSERT(paddingSize >= sizeof(message_header));
                BOOST_ASSERT(paddingSize <= std::numeric_limits<message_size_type>::max());
                BOOST_ASSERT(paddingBuffer > messageBuffer);
                message_header skipHeader{.m_type = s_skipFlag, .m_payloadSize = static_cast<message_size_type>(paddingSize) - sizeof(message_header)};
                std::memcpy(paddingBuffer, &skipHeader, sizeof(message_header));
            }

            std::memcpy(messageBuffer, &header, sizeof(message_header));
            std::memcpy(messageBuffer + sizeof(message_header), &message, header.m_payloadSize);

            SBK_STATUS_TRY_C(m_ringBuffer.commit_write(reserveIndex, sizeof(message_header) + header.m_payloadSize + paddingSize));
            return SBK_SUCCESS;
        }

        /**
         * @brief Start reading, giving the user a chance to use the data before producers write more data.
         * 
         * Must call @see read_end after finished with the data.
         * 
         * @param outMessageView data to access the buffer
         */
        [[nodiscard]] auto read_begin(message_view* outMessageView) noexcept -> sbk_status
        {
            SBK_STATUS_CHECK(outMessageView != nullptr, SBK_ERR_INVALID_PARAMETER);

            std::uint8_t* messageBuffer{};
            std::size_t readIndex{};
            std::size_t bytesRead{};
            numeric_message_type messageType = s_skipFlag;
            message_size_type payloadSize{};

            do
            {
                // READ HEADER

                SBK_STATUS_TRY_C(m_ringBuffer.read_begin(&messageBuffer, &readIndex, sizeof(message_header), &bytesRead));
                message_header* header = reinterpret_cast<message_header*>(messageBuffer);
                messageType            = header->m_type;
                payloadSize            = header->m_payloadSize;
                SBK_STATUS_TRY_C(m_ringBuffer.read_end(sizeof(message_header)));

                // AUTO READ SKIP PAYLOAD

                if (header->m_type == s_skipFlag)
                {
                    SBK_STATUS_TRY_C(m_ringBuffer.read_begin(&messageBuffer, &readIndex, payloadSize, &bytesRead));
                    SBK_STATUS_TRY_C(m_ringBuffer.read_end(payloadSize));
                }

            } while (messageType == s_skipFlag);

            outMessageView->m_type        = static_cast<T>(messageType);
            outMessageView->m_payloadSize = payloadSize;
            outMessageView->payload       = messageBuffer + sizeof(message_header);
            return SBK_SUCCESS;
        }

        /**
         * @brief Finish reading, allowing producers to write over the previously read data.
         */
        [[nodiscard]] auto read_end(const message_view& message) noexcept -> sbk_status
        {
            return m_ringBuffer.read_end(static_cast<std::size_t>(message.m_payloadSize));
        }

        [[nodiscard]] constexpr auto get_header_size() const noexcept -> std::size_t
        {
            return sizeof(message_header);
        }

    private:
        mpsc_ring_buffer m_ringBuffer;
    };
}
