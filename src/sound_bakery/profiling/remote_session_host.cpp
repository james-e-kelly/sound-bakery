#include "sound_bakery/profiling/remote_session_host.h"

#include "sound_bakery/error/result.h"
#include "sound_bakery/sound_bakery.h"  // sbk_log

#include <boost/asio.hpp>

#include <cstring>
#include <optional>
#include <utility>
#include <vector>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace sbk::engine::profiling
{
    namespace
    {
        /**
         * @brief One connected tool.
         *
         * Keeps at most one write in flight. If new telemetry arrives mid-send,
         * the pending message is replaced rather than queued - a slow tool
         * gets the latest snapshot, never a growing backlog.
         *
         * Received live-edit commands are appended to @c commandSink, which the
         * owning impl outlives (abandoned handlers are destroyed, never run).
         */
        struct tool_connection : std::enable_shared_from_this<tool_connection>
        {
            tool_connection(tcp::socket&& sessionSocket, std::vector<set_property_command>& sink)
                : socket(std::move(sessionSocket)), commandSink(&sink)
            {
            }

            auto send(std::vector<char> message) -> void
            {
                if (!alive)
                {
                    return;
                }

                if (writing)
                {
                    pendingMessage = std::move(message);
                    return;
                }

                writeBuffer = std::move(message);
                start_write();
            }

            auto start_write() -> void
            {
                writing = true;

                asio::async_write(socket, asio::buffer(writeBuffer),
                                  [self = shared_from_this()](const boost::system::error_code& error, std::size_t)
                                  {
                                      self->writing = false;

                                      if (error)
                                      {
                                          self->alive = false;
                                          return;
                                      }

                                      if (!self->pendingMessage.empty())
                                      {
                                          self->writeBuffer = std::move(self->pendingMessage);
                                          self->pendingMessage.clear();
                                          self->start_write();
                                      }
                                  });
            }

            /**
             * @brief Reads the tool's messages; a read error doubles as disconnect detection.
             */
            auto read_next_message() -> void
            {
                asio::async_read(socket, asio::buffer(&readHeader, sizeof(readHeader)),
                                 [self = shared_from_this()](const boost::system::error_code& error, std::size_t)
                                 {
                                     if (error || self->readHeader.magic != remoteProtocolMagic ||
                                         self->readHeader.version != remoteProtocolVersion ||
                                         self->readHeader.payloadSize > remoteMaxPayloadSize)
                                     {
                                         self->alive = false;
                                         return;
                                     }

                                     if (self->readHeader.payloadSize == 0)
                                     {
                                         self->handle_message();
                                         return;
                                     }

                                     self->readPayload.resize(self->readHeader.payloadSize);
                                     asio::async_read(self->socket, asio::buffer(self->readPayload),
                                                      [self](const boost::system::error_code& readError, std::size_t)
                                                      {
                                                          if (readError)
                                                          {
                                                              self->alive = false;
                                                              return;
                                                          }

                                                          self->handle_message();
                                                      });
                                 });
            }

            auto handle_message() -> void
            {
                if (static_cast<remote_message_type>(readHeader.messageType) == remote_message_type::set_property &&
                    readHeader.payloadSize == sizeof(set_property_command))
                {
                    set_property_command command;
                    std::memcpy(&command, readPayload.data(), sizeof(command));
                    commandSink->push_back(command);
                }
                //< Unknown messages are skipped by length so newer tools stay compatible.

                read_next_message();
            }

            [[nodiscard]] auto is_alive() const -> bool { return alive; }

        private:
            tcp::socket socket;
            std::vector<set_property_command>* commandSink = nullptr;
            std::vector<char> writeBuffer;
            std::vector<char> pendingMessage;
            remote_message_header readHeader{};
            std::vector<char> readPayload;
            bool writing = false;
            bool alive   = true;
        };
    }  // namespace

    struct remote_session_host::impl
    {
        asio::io_context ioContext;
        std::optional<tcp::acceptor> acceptor;
        std::vector<std::shared_ptr<tool_connection>> sessions;
        std::vector<set_property_command> receivedCommands;
        std::uint16_t port = 0;

        auto start_accept() -> void
        {
            acceptor->async_accept(
                [this](const boost::system::error_code& error, tcp::socket socket)
                {
                    if (!error)
                    {
                        auto session = std::make_shared<tool_connection>(std::move(socket), receivedCommands);
                        session->send(make_remote_message(remote_message_type::hello, nullptr, 0));
                        session->read_next_message();
                        sessions.push_back(std::move(session));

                        SBK_INFO("Tool connected to the remote session");
                    }

                    if (acceptor.has_value() && acceptor->is_open())
                    {
                        start_accept();
                    }
                });
        }
    };

    remote_session_host::remote_session_host()  = default;
    remote_session_host::~remote_session_host() = default;

    auto remote_session_host::open(const std::uint16_t port) -> sbk::result<void>
    {
        close();

        auto newImpl = std::make_unique<impl>();
        newImpl->acceptor.emplace(newImpl->ioContext);

        boost::system::error_code error;
        const tcp::endpoint endpoint(tcp::v4(), port);

        newImpl->acceptor->open(endpoint.protocol(), error);
        SBK_CHECK_MSG(!error, SBK_ERR_SYSTEM, "Remote session host could not open a socket: {}", error.message());

        newImpl->acceptor->bind(endpoint, error);
        SBK_CHECK_MSG(!error, SBK_ERR_SYSTEM, "Remote session host could not bind port {}: {}", port,
                      error.message());

        newImpl->acceptor->listen(asio::socket_base::max_listen_connections, error);
        SBK_CHECK_MSG(!error, SBK_ERR_SYSTEM, "Remote session host could not listen: {}", error.message());

        newImpl->port = newImpl->acceptor->local_endpoint().port();
        newImpl->start_accept();

        m_impl = std::move(newImpl);

        SBK_INFO(fmt::format("Remote session host listening on port {}", m_impl->port).c_str());
        return sbk::ok();
    }

    auto remote_session_host::close() -> void
    {
        // Destroying the io_context abandons pending handlers without invoking
        // them and closes the acceptor and every session socket.
        m_impl.reset();
    }

    auto remote_session_host::update() -> void
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

        std::erase_if(m_impl->sessions, [](const auto& session) { return !session->is_alive(); });
    }

    auto remote_session_host::publish_telemetry(const telemetry_snapshot& snapshot) -> void
    {
        if (!m_impl || m_impl->sessions.empty())
        {
            return;
        }

        const std::vector<char> message =
            make_remote_message(remote_message_type::telemetry, &snapshot, sizeof(snapshot));

        for (const std::shared_ptr<tool_connection>& session : m_impl->sessions)
        {
            session->send(message);
        }
    }

    auto remote_session_host::consume_property_commands() -> std::vector<set_property_command>
    {
        if (!m_impl)
        {
            return {};
        }

        return std::exchange(m_impl->receivedCommands, {});
    }

    auto remote_session_host::is_open() const -> bool
    {
        return m_impl != nullptr && m_impl->acceptor.has_value() && m_impl->acceptor->is_open();
    }

    auto remote_session_host::get_port() const -> std::uint16_t { return m_impl ? m_impl->port : 0; }

    auto remote_session_host::get_connection_count() const -> std::size_t { return m_impl ? m_impl->sessions.size() : 0; }
}  // namespace sbk::engine::profiling
