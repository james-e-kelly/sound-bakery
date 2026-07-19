#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

/**
 * @file remote_protocol.h
 * @brief Wire format for Sound Bakery's remote profiling connection.
 *
 * A deliberately tiny, versioned protocol: every message is a fixed
 * @ref remote_message_header followed by @c payloadSize bytes of payload.
 * Payloads are trivially-copyable structs sent as raw bytes, so both ends are
 * assumed to be little-endian (true for every platform Sound Bakery targets).
 *
 * The host (game/runtime) pushes @c hello and @c telemetry; the connected tool
 * (editor or other authoring app) pushes live-edit commands like @c set_property. Unknown
 * message types are skipped by length, so newer peers stay compatible.
 */

namespace sbk::engine::profiling
{
    constexpr std::uint32_t remoteProtocolMagic   = 0x53424B50;  //< "SBKP"
    constexpr std::uint16_t remoteProtocolVersion = 2;

    /**
     * @brief Default TCP port for remote profiling connections ("BAKE" on a phone keypad).
     */
    constexpr std::uint16_t remoteDefaultPort = 22553;

    /**
     * @brief Upper bound on a single message payload; anything larger means a corrupt stream.
     */
    constexpr std::uint32_t remoteMaxPayloadSize = 64 * 1024;

    enum class remote_message_type : std::uint16_t
    {
        invalid      = 0,
        hello        = 1,  //< Sent by the host when a tool connects. No payload.
        telemetry    = 2,  //< Periodic profiling snapshot. Payload is @ref telemetry_snapshot.
        set_property = 3,  //< Live edit from the tool. Payload is @ref set_property_command.
    };

    /**
     * @brief Fixed preamble for every message on the wire.
     */
    struct remote_message_header
    {
        std::uint32_t magic       = remoteProtocolMagic;
        std::uint16_t version     = remoteProtocolVersion;
        std::uint16_t messageType = static_cast<std::uint16_t>(remote_message_type::invalid);
        std::uint32_t payloadSize = 0;
    };

    static_assert(sizeof(remote_message_header) == 12, "Header layout is part of the wire format");
    static_assert(std::is_trivially_copyable_v<remote_message_header>);

    /**
     * @brief One frame of profiling data, mirroring the values plotted to Tracy.
     *
     * Extend by appending fields and bumping @ref remoteProtocolVersion.
     */
    struct telemetry_snapshot
    {
        std::uint32_t playingVoices      = 0;
        std::uint32_t nodeInstances      = 0;
        std::uint32_t gameObjects        = 0;
        std::uint32_t reserved           = 0;  //< Keeps the 64-bit fields naturally aligned.
        std::uint64_t memoryCurrentBytes = 0;
        std::uint64_t memoryTotalBytes   = 0;
    };

    static_assert(sizeof(telemetry_snapshot) == 32, "Snapshot layout is part of the wire format");
    static_assert(std::is_trivially_copyable_v<telemetry_snapshot>);

    /**
     * @brief A live edit: "set @c property on the object with @c objectID to @c value".
     *
     * Sent by the editor/tool when a value changes; applied by the runtime on
     * its next update. @c property is the @c sbk::core::synced_property_id
     * hash of the reflected property name - which properties exist is driven
     * entirely by reflection metadata (metadata_key::synced), not this header.
     */
    struct set_property_command
    {
        std::uint64_t objectID = 0;  //< sbk_id of the target object.
        std::uint32_t property = 0;  //< synced_property_id of the reflected property name.
        float value            = 0.0F;
    };

    static_assert(sizeof(set_property_command) == 16, "Command layout is part of the wire format");
    static_assert(std::is_trivially_copyable_v<set_property_command>);

    /**
     * @brief Serializes a header plus optional payload into one send-ready buffer.
     */
    inline auto make_remote_message(const remote_message_type type, const void* payload,
                                    const std::uint32_t payloadSize) -> std::vector<char>
    {
        remote_message_header header;
        header.messageType = static_cast<std::uint16_t>(type);
        header.payloadSize = payloadSize;

        std::vector<char> buffer(sizeof(remote_message_header) + payloadSize);
        std::memcpy(buffer.data(), &header, sizeof(header));

        if (payloadSize > 0)
        {
            std::memcpy(buffer.data() + sizeof(header), payload, payloadSize);
        }

        return buffer;
    }
}  // namespace sbk::engine::profiling
