#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/error/result.h"
#include "sound_chef/sound_chef_ring_buffer.h"

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
     * Wraps @ref sc_ring_buffer (Sound Chef, C) and adds a small header before
     * each payload so the consumer can identify messages by type and skip
     * end-of-buffer padding automatically.
     *
     * The header carries the message type (any integral or enum type) and the
     * payload size in bytes.
     *
     * On every read, the queue tries to read `sizeof(message_header)`.
     * Once it has the header, it reads a further number of bytes equal to the
     * payload size and returns the data as a @s message_view — a pointer into
     * the buffer plus the message type.
     *
     * Users cast `payload` to their message type:
     * `const start_message* startMessage = view.cast<start_message>();`
     *
     * Writes reserve space then memcpy the message into the buffer:
     * `messageQueue.write_message(message_type::start, start_message{.a = 7, .b = 9});`
     *
     * The message type is not deduced from the message structure, so the user
     * must pass it explicitly.
     *
     * @remark To keep payloads contiguous, messages "wrap" via a skip header.
     * When a message won't fit at the end of the buffer, the queue writes a
     * special "skip" header over the padding at the end. The reader
     * automatically consumes the skip and returns the user's message from the
     * start of the buffer, so callers never see the skip type.
     */
    template <message_identifier T = std::uint8_t>
    class message_queue final
    {
        using numeric_message_identifier_t  = typename numeric_type<T>::type;   //< Gets the numeric value of T, either an integer or the underlying value of an enum
        using payload_size_t                = std::uint16_t;

        static_assert(std::is_integral_v<numeric_message_identifier_t>);

        /**
         * @brief Special flag that tells the reader to read the message size and move on because the actual message wouldn't fit at the end of the buffer.
         *
         * Handled entirely inside the queue; users never see it in a @ref message_view.
         */
        static constexpr numeric_message_identifier_t s_skipFlag = std::numeric_limits<numeric_message_identifier_t>::max();

        /**
         * @brief Basic header of all messages. Written to the buffer before the payload bytes.
         */
        struct message_header
        {
            numeric_message_identifier_t m_identifier{};  //< Message type used by consumers to route each message
            payload_size_t m_payloadSize{};               //< Payload bytes following the header (excludes header itself)
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
        message_queue() noexcept = default;
        ~message_queue() noexcept { sc_ring_buffer_uninit(&m_ringBuffer); }

        message_queue(const message_queue&)                    = delete;
        auto operator=(const message_queue&) -> message_queue& = delete;
        message_queue(message_queue&&)                         = delete;
        auto operator=(message_queue&&) -> message_queue&      = delete;

        /**
         * @brief Returned data from the buffer. Not the actual raw data — just a pointer into it.
         */
        struct message_view
        {
            T m_identifier{};                   //< Message type, used to cast the payload to the correct concrete type
            payload_size_t m_payloadSize{};     //< Payload size. Used by read_end. Do not edit.
            const void* payload{};              //< Pointer to the message payload. Can be null for zero-sized payloads.

            /**
             * @brief Cast the payload to the user's message type.
             */
            template<typename U>
            [[nodiscard]] inline auto cast() const -> const U*
            {
                return payload == nullptr ? nullptr : reinterpret_cast<const U*>(payload);
            }
        };

        /**
         * @brief Initialise the message queue.
         *
         * @param size                  Requested buffer size in bytes. Rounded up to the next power of two.
         * @param allocationCallbacks   Allocation callbacks passed to the underlying ring buffer. May be NULL for miniaudio defaults.
         * @return  SBK_ERR_INVALID_PARAMETER if @r size cannot fit a header plus at least a small payload.
         */
        [[nodiscard]] auto init(std::size_t size, const ma_allocation_callbacks* allocationCallbacks) noexcept -> sbk::result<>
        {
            SBK_CHECK_MSG(size >= get_header_size(), SBK_ERR_INVALID_PARAMETER, "The message queue needs to be big enough to hold a header and, at least, a small payload");
            SBK_TRY_C(sc_ring_buffer_init(&m_ringBuffer, size, allocationCallbacks));
            return sbk::ok();
        }

        /**
         * @brief Write a message to the queue.
         *
         * Automatically wraps messages around the end of the buffer via a skip
         * header so payloads are always contiguous for the consumer.
         *
         * @tparam U user message type
         * @param type type of message
         * @param message message data
         */
        template <message_payload U>
        [[nodiscard]] auto write_message(T type, const U& message) noexcept -> sbk_status
        {
            ZoneScoped;
            SBK_STATUS_CHECK_MSG(to_numeric(type) != s_skipFlag, SBK_ERR_INVALID_PARAMETER, "Users should not create messages with a flag equal to s_skipFlag. This value is reserved for the queue's own end-of-buffer wrap marker");
            BOOST_ASSERT_MSG(std::numeric_limits<payload_size_t>::max() >= sizeof(U), "Message is too large for the queue. Consider increasing the payload_size_t size or decreasing the message size");

            std::uint8_t* messageBuffer{};
            std::uint8_t* paddingBuffer{};
            std::size_t   paddingSize{};
            std::size_t   reserveIndex{};

            const message_header header{
                .m_identifier  = to_numeric(type),
                .m_payloadSize = static_cast<payload_size_t>(sizeof(U))
            };
            SBK_STATUS_TRY_C(sc_ring_buffer_reserve_write(&m_ringBuffer,
                                                          sizeof(message_header) + header.m_payloadSize,
                                                          &messageBuffer,
                                                          &reserveIndex,
                                                          &paddingBuffer,
                                                          &paddingSize));

            // Write a skip header over end-of-buffer padding so the reader can jump it.
            // Padding smaller than a header is handled by the reader from size alone.
            if (paddingSize >= sizeof(message_header))
            {
                const message_header skipHeader{
                    .m_identifier  = s_skipFlag,
                    .m_payloadSize = static_cast<payload_size_t>(paddingSize - sizeof(message_header))
                };
                std::memcpy(paddingBuffer, &skipHeader, sizeof(message_header));
            }

            std::memcpy(messageBuffer, &header, sizeof(message_header));
            std::memcpy(messageBuffer + sizeof(message_header), &message, header.m_payloadSize);

            SBK_STATUS_TRY_C(sc_ring_buffer_commit_write(&m_ringBuffer, reserveIndex, sizeof(message_header) + header.m_payloadSize + paddingSize));
            return SBK_SUCCESS;
        }

        /**
         * @brief Start reading, giving the caller a chance to use the data before producers write more.
         *
         * Must be paired with @ref read_end.
         */
        [[nodiscard]] auto read_begin(message_view* outMessageView) noexcept -> sbk_status
        {
            ZoneScoped;
            SBK_STATUS_CHECK(outMessageView != nullptr, SBK_ERR_INVALID_PARAMETER);

            std::uint8_t*                 messageBuffer{};
            std::size_t                   readIndex{};
            std::size_t                   bytesRead{};
            numeric_message_identifier_t  messageType = s_skipFlag;
            payload_size_t                payloadSize{};

            do
            {
                SBK_STATUS_TRY_C(sc_ring_buffer_read_begin(&m_ringBuffer, sizeof(message_header), &messageBuffer, &readIndex, &bytesRead));
                BOOST_ASSERT(bytesRead <= sizeof(message_header));

                if (bytesRead == sizeof(message_header))
                {
                    message_header* header = reinterpret_cast<message_header*>(messageBuffer);
                    messageType            = header->m_identifier;
                    payloadSize            = header->m_payloadSize;
                    SBK_STATUS_TRY_C(sc_ring_buffer_read_end(&m_ringBuffer, sizeof(message_header)));

                    if (header->m_identifier == s_skipFlag)
                    {
                        SBK_STATUS_TRY_C(sc_ring_buffer_read_begin(&m_ringBuffer, payloadSize, &messageBuffer, &readIndex, &bytesRead));
                        SBK_STATUS_TRY_C(sc_ring_buffer_read_end(&m_ringBuffer, payloadSize));
                    }
                }
                else
                {
                    SBK_STATUS_TRY_C(sc_ring_buffer_read_end(&m_ringBuffer, bytesRead));
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
            return sc_ring_buffer_read_end(&m_ringBuffer, static_cast<std::size_t>(message.m_payloadSize));
        }

        [[nodiscard]] constexpr auto get_header_size() const noexcept -> std::size_t
        {
            return sizeof(message_header);
        }

        [[nodiscard]] auto raw_ring_buffer() noexcept -> sc_ring_buffer* { return &m_ringBuffer; }

    private:
        sc_ring_buffer m_ringBuffer{};
    };
}
