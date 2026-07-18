#pragma once

#include "spdlog/logger.h"
#include "spdlog/sinks/callback_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#if defined(_WIN32)
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/wincolor_sink.h"
#endif

#include <cstdint>
#include <filesystem>

namespace sbk::core
{
    typedef enum
    {
        SBK_LOG_LEVEL_DEBUG   = 4,
        SBK_LOG_LEVEL_INFO    = 3,
        SBK_LOG_LEVEL_WARNING = 2,
        SBK_LOG_LEVEL_ERROR   = 1
    } sbk_log_level;

    typedef void (* sbk_log_callback_proc)(unsigned int level, const char* pMessage);

    class external_log_callback : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        external_log_callback() = default;

        auto set_callback(sbk_log_callback_proc callback) -> void
        {
            m_callback = callback;
        }

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override 
        { 
            if (m_callback)
            {
                switch (msg.level)
                {
                    case spdlog::level::trace:
                    case spdlog::level::debug:
                        m_callback(SBK_LOG_LEVEL_DEBUG, msg.payload.data());
                        break;
                    default:
                    case spdlog::level::info:
                        m_callback(SBK_LOG_LEVEL_INFO, msg.payload.data());
                        break;
                    case spdlog::level::warn:
                        m_callback(SBK_LOG_LEVEL_WARNING, msg.payload.data());
                        break;
                    case spdlog::level::err:
                    case spdlog::level::critical:
                        m_callback(SBK_LOG_LEVEL_ERROR, msg.payload.data());
                        break;
                }
            }
        }

        void flush_() override {};

    private:
        sbk_log_callback_proc m_callback = nullptr;
    };

	class logger
	{
    public:
        logger(const std::string& name)
        {
            m_sinks = std::make_shared<spdlog::sinks::dist_sink_mt>();

            m_logger = std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list{m_sinks});
            m_logger->set_level(spdlog::level::debug);
            m_logger->set_pattern("[%Y-%m-%d %H:%M:%S %z][Thread %t][%l] %n: %v");
        }

        auto add_console_sink() -> void
        {
        #if defined(_WIN32)
            const std::shared_ptr<spdlog::sinks::msvc_sink_mt> windowsSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
            m_sinks->add_sink(windowsSink);
        #endif

            const std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            m_sinks->add_sink(stdoutSink);
        }

        auto add_file_sink(const std::filesystem::path& file) -> void
        {
            auto now          = spdlog::log_clock::now();
            const time_t tnow = spdlog::log_clock::to_time_t(now);
            const tm nowtm   = spdlog::details::os::localtime(tnow);

            const auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                file.string(), nowtm.tm_hour, nowtm.tm_min, true, uint16_t{0}, spdlog::file_event_handlers{});
            dailySink->set_level(spdlog::level::trace);

            const auto basicFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file.string(), true);
            basicFileSink->set_level(spdlog::level::trace);

            m_sinks->add_sink(dailySink);
            m_sinks->add_sink(basicFileSink);
        }

        auto add_external_log(sbk_log_callback_proc callback) -> void
        {
            const auto externalCallbackSink = std::make_shared<external_log_callback>();
            externalCallbackSink->set_callback(callback);
            externalCallbackSink->set_level(spdlog::level::info);

            m_sinks->add_sink(externalCallbackSink);
        }

        auto get_logger() const -> const std::shared_ptr<spdlog::logger>& { return m_logger; }

    private:
        std::shared_ptr<spdlog::logger> m_logger;
        std::shared_ptr<spdlog::sinks::dist_sink_mt> m_sinks;
	};
}