#pragma once

#include "sound_bakery/core/containers/ring_buffer.h"

namespace sbk
{
    /**
     * @brief The enum, or something else, that defines the message's type.
     */
    template<typename T>
    concept message_identifier = std::is_integral_v<T> || std::is_enum_v<T>;

    /**
     * @brief User messages must be POD types so they can be memcpy'd easily.
     */
    template<typename T>
    concept message_payload = std::is_trivial_v<T>;

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
    template <message_identifier T = std::uint8_t>
    class message_queue final
    {
        using numeric_message_identifier_t  = typename numeric_type<T>::type;   //< Gets the numeric value of T, either an integer or the underlying value of an enum
        using payload_size_t                = std::uint8_t;                     //< Keep it as small as possible

        static_assert(std::is_integral_v<numeric_message_identifier_t>);

        /**
         * @brief A special flag that tells the reader to just read the message size and move on because it was the end of the buffer and the message didn't fit.
         * 
         * The queue automatically handles this message type and will read past it. The user only ever sees their actual message types.
         */
        static constexpr numeric_message_identifier_t s_skipFlag = std::numeric_limits<numeric_message_identifier_t>::max();

        /**
         * @brief Basic header of all messages. Written to the buffer. Payloads are written after the header.
         */
        struct message_header
        {
            numeric_message_identifier_t m_identifier{};  //< The type of message, used by consumers to handle each message in a different way (HandlePostEvent / HandleSetRTPC etc)
            payload_size_t m_payloadSize{};               //< Payload size. Excludes the header size
        };

        template <typename U>
        constexpr auto to_numeric(U value) noexcept
        {
            if constexpr (std::is_enum_v<U>)
            {
                return static_cast<std::underlying_type_t<U>>(value);
            }
            else
            {
                return static_cast<U>(value);
            }
        }

    public:
        /**
         * @brief Returned data from the buffer. Not the actual raw data that was inside the buffer - just a pointer to it.
         */
        struct message_view
        {
            T m_identifier{};                   //< The type of message, used by consumers to handle each message and cast the payload to the correct type
            payload_size_t m_payloadSize{};     //< Payload/message size. Used by read_end. Should not be edited
            const void* payload{};              //< Pointer to the message payload. Can be null

            /**
             * @brief Cast the payload to the user's message type.
             * @tparam U message type
             * @return pointer to the user's message
             */
            template<typename U>
            [[nodiscard]] inline auto cast() const -> const U*
            {
                return payload == nullptr ? nullptr : reinterpret_cast<const U*>(payload);
            }
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
        template <message_payload U>
        [[nodiscard]] auto write_message(T type, const U& message) noexcept -> sbk_status
        {
            ZoneScoped;
            SBK_STATUS_CHECK_MSG(to_numeric(type) != s_skipFlag, SBK_ERR_INVALID_PARAMETER, "Users should not create messages with a flag equal to s_skipFlag. This should only be used by the message_queue to automically wrap around the end of the buffer");
            BOOST_ASSERT_MSG(std::numeric_limits<payload_size_t>::max() >= sizeof(U), "Message is too large for the queue. Consider increasing the payload_size_t size or decreasing the message size");

            std::uint8_t* messageBuffer{};
            std::uint8_t* paddingBuffer{};
            std::size_t paddingSize{};
            std::size_t reserveIndex{};

            message_header header{.m_identifier = to_numeric(type), .m_payloadSize = static_cast<payload_size_t>(sizeof(U))};
            SBK_STATUS_TRY_C(m_ringBuffer.reserve_write(&messageBuffer, sizeof(message_header) + header.m_payloadSize, &reserveIndex, &paddingBuffer, &paddingSize));

            // If we were given padding, it means the buffer is split
            // Add a skip message into end of the buffer. Write the actual message as normal
            if (paddingSize > 0U)
            {
                if (paddingSize >= sizeof(message_header))  // If we can add a header, add one. Otherwise, the reader can also detect there is a tiny space before the end of the buffer and skip it
                {
                    message_header skipHeader{.m_identifier = s_skipFlag, .m_payloadSize = static_cast<payload_size_t>(paddingSize - sizeof(message_header))};
                    std::memcpy(paddingBuffer, &skipHeader, sizeof(message_header));
                }
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
            ZoneScoped;
            SBK_STATUS_CHECK(outMessageView != nullptr, SBK_ERR_INVALID_PARAMETER);

            std::uint8_t* messageBuffer{};
            std::size_t readIndex{};
            std::size_t bytesRead{};
            numeric_message_identifier_t messageType = s_skipFlag;
            payload_size_t payloadSize{};

            do
            {
                // READ HEADER

                SBK_STATUS_TRY_C(m_ringBuffer.read_begin(&messageBuffer, &readIndex, sizeof(message_header), &bytesRead));
                BOOST_ASSERT(bytesRead <= sizeof(message_header));
                if (bytesRead == sizeof(message_header))
                {
                    message_header* header = reinterpret_cast<message_header*>(messageBuffer);
                    messageType            = header->m_identifier;
                    payloadSize            = header->m_payloadSize;
                    SBK_STATUS_TRY_C(m_ringBuffer.read_end(sizeof(message_header)));

                    // AUTO READ SKIP PAYLOAD

                    if (header->m_identifier == s_skipFlag)
                    {
                        SBK_STATUS_TRY_C(m_ringBuffer.read_begin(&messageBuffer, &readIndex, payloadSize, &bytesRead));
                        SBK_STATUS_TRY_C(m_ringBuffer.read_end(payloadSize));
                    }
                }
                else
                {
                    // If the writer couldn't fit a header, just skip round
                    SBK_STATUS_TRY_C(m_ringBuffer.read_end(bytesRead));
                }


            } while (messageType == s_skipFlag);

            outMessageView->m_identifier  = static_cast<T>(messageType);
            outMessageView->m_payloadSize = payloadSize;
            outMessageView->payload       = messageBuffer + sizeof(message_header);
            return SBK_SUCCESS;
        }

        /**
         * @brief Finish reading, allowing producers to write over the previously read data.
         */
        [[nodiscard]] auto read_end(const message_view& message) noexcept -> sbk_status
        {
            ZoneScoped;
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
