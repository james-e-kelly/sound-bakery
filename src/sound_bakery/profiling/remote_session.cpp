#include "sound_bakery/profiling/remote_session.h"

#include "sound_bakery/error/result.h"
#include "sound_bakery/sound_bakery.h"  // sbk_log

#include <boost/asio.hpp>

#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace sbk::engine::profiling
{
    struct remote_session::impl
    {
        asio::io_context ioContext;
        tcp::socket socket{ioContext};
        remote_message_header header{};
        std::vector<char> payload;
        std::optional<telemetry_snapshot> latestTelemetry;
        std::uint64_t telemetryCount = 0;
        std::vector<char> writeBuffer;
        std::deque<std::vector<char>> writeQueue;
        bool writing                 = false;
        bool connecting              = false;
        bool handshaken              = false;

        /**
         * @brief Queues a message; commands are FIFO and never dropped.
         */
        auto send(std::vector<char> message) -> void
        {
            if (writing || connecting)
            {
                writeQueue.push_back(std::move(message));
                return;
            }

            writeBuffer = std::move(message);
            start_write();
        }

        auto start_write() -> void
        {
            writing = true;

            asio::async_write(socket, asio::buffer(writeBuffer),
                              [this](const boost::system::error_code& error, std::size_t)
                              {
                                  writing = false;

                                  if (error)
                                  {
                                      fail("Remote session connection lost: " + error.message());
                                      return;
                                  }

                                  if (!writeQueue.empty())
                                  {
                                      writeBuffer = std::move(writeQueue.front());
                                      writeQueue.pop_front();
                                      start_write();
                                  }
                              });
        }

        /**
         * @brief Flushes messages queued while the connect was still in flight.
         */
        auto flush_queued_writes() -> void
        {
            if (!writing && !writeQueue.empty())
            {
                writeBuffer = std::move(writeQueue.front());
                writeQueue.pop_front();
                start_write();
            }
        }

        auto fail(const std::string& reason) -> void
        {
            connecting = false;
            handshaken = false;

            boost::system::error_code ignored;
            socket.close(ignored);

            sbk::log_error(SBK_ERR_SYSTEM, reason);
        }

        auto read_header() -> void
        {
            asio::async_read(socket, asio::buffer(&header, sizeof(header)),
                             [this](const boost::system::error_code& error, std::size_t)
                             {
                                 if (error)
                                 {
                                     fail("Remote session connection lost: " + error.message());
                                     return;
                                 }

                                 if (header.magic != remoteProtocolMagic ||
                                     header.version != remoteProtocolVersion ||
                                     header.payloadSize > remoteMaxPayloadSize)
                                 {
                                     fail("Remote session stream is corrupt or from an incompatible version");
                                     return;
                                 }

                                 if (header.payloadSize == 0)
                                 {
                                     handle_message();
                                     return;
                                 }

                                 payload.resize(header.payloadSize);
                                 asio::async_read(socket, asio::buffer(payload),
                                                  [this](const boost::system::error_code& readError, std::size_t)
                                                  {
                                                      if (readError)
                                                      {
                                                          fail("Remote session connection lost: " +
                                                               readError.message());
                                                          return;
                                                      }

                                                      handle_message();
                                                  });
                             });
        }

        auto handle_message() -> void
        {
            switch (static_cast<remote_message_type>(header.messageType))
            {
                case remote_message_type::hello:
                    handshaken = true;
                    SBK_INFO("Connected to remote Sound Bakery instance");
                    break;

                case remote_message_type::telemetry:
                    if (header.payloadSize == sizeof(telemetry_snapshot))
                    {
                        telemetry_snapshot snapshot;
                        std::memcpy(&snapshot, payload.data(), sizeof(snapshot));
                        latestTelemetry = snapshot;
                        ++telemetryCount;
                    }
                    break;

                default:
                    break;  //< Unknown messages are skipped by length so newer hosts stay compatible.
            }

            read_header();
        }
    };

    remote_session::remote_session()  = default;
    remote_session::~remote_session() = default;

    auto remote_session::connect(const std::string_view host, const std::uint16_t port) -> sbk::result<void>
    {
        disconnect();

        auto newImpl = std::make_unique<impl>();

        boost::system::error_code error;
        tcp::resolver resolver(newImpl->ioContext);
        const tcp::resolver::results_type endpoints = resolver.resolve(std::string(host), std::to_string(port), error);
        SBK_CHECK_MSG(!error, SBK_ERR_SYSTEM, "Could not resolve remote session host '{}': {}", host,
                      error.message());

        newImpl->connecting = true;

        asio::async_connect(newImpl->socket, endpoints,
                            [implPointer = newImpl.get()](const boost::system::error_code& connectError,
                                                          const tcp::endpoint&)
                            {
                                implPointer->connecting = false;

                                if (connectError)
                                {
                                    implPointer->fail("Could not connect to the remote session host: " +
                                                      connectError.message());
                                    return;
                                }

                                implPointer->read_header();
                                implPointer->flush_queued_writes();
                            });

        m_impl = std::move(newImpl);
        return sbk::ok();
    }

    auto remote_session::disconnect() -> void
    {
        // Destroying the io_context abandons pending handlers without invoking them.
        m_impl.reset();
    }

    auto remote_session::update() -> void
    {
        if (!m_impl)
        {
            return;
        }

        m_impl->ioContext.poll();

        if (m_impl->ioContext.stopped())
        {
            m_impl->ioContext.restart();
        }
    }

    auto remote_session::is_connecting() const -> bool { return m_impl != nullptr && m_impl->connecting; }

    auto remote_session::is_connected() const -> bool
    {
        return m_impl != nullptr && m_impl->handshaken && m_impl->socket.is_open();
    }

    auto remote_session::send_set_property(const sbk_id objectID, const std::uint32_t propertyID, const float value)
        -> sbk::result<void>
    {
        SBK_CHECK_MSG(m_impl != nullptr && m_impl->socket.is_open(), SBK_ERR_SYSTEM,
                      "Cannot send a live edit: no remote session");

        set_property_command command;
        command.objectID = objectID;
        command.property = propertyID;
        command.value    = value;

        m_impl->send(make_remote_message(remote_message_type::set_property, &command, sizeof(command)));
        return sbk::ok();
    }

    auto remote_session::get_latest_telemetry() const -> std::optional<telemetry_snapshot>
    {
        return m_impl ? m_impl->latestTelemetry : std::nullopt;
    }

    auto remote_session::get_telemetry_count() const -> std::uint64_t { return m_impl ? m_impl->telemetryCount : 0; }
}  // namespace sbk::engine::profiling
