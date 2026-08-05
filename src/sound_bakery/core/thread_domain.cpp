#include "sound_bakery/core/thread_domain.h"

#include "sound_bakery/sound_bakery.h"  // sbk_log

#include <spdlog/fmt/fmt.h>

#include <atomic>

namespace
{
    thread_local sbk::core::thread_domain t_currentDomain = sbk::core::thread_domain::unknown;

    // Single-threaded (editor) mode, mirrored from sbk_system_config::singleThreadedUpdate
    // when the system initialises. When set, the sole owner thread satisfies every
    // domain: with nothing else pumping engine work, its access is serialized by
    // definition, so the domain checks pass without a scope.
    std::atomic<bool> s_singleThreadedMode{false};
}  // namespace

auto sbk::core::get_current_thread_domain() -> thread_domain { return t_currentDomain; }

auto sbk::core::current_thread_satisfies(const thread_domain domain) -> bool
{
    return t_currentDomain == domain || s_singleThreadedMode.load(std::memory_order_relaxed);
}

auto sbk::core::set_single_threaded_mode(const bool enabled) -> void
{
    s_singleThreadedMode.store(enabled, std::memory_order_relaxed);
}

auto sbk::core::to_string(const thread_domain domain) -> const char*
{
    switch (domain)
    {
        case thread_domain::game:
            return "game";
        case thread_domain::studio:
            return "studio";
        case thread_domain::unknown:
        default:
            return "unknown";
    }
}

sbk::core::scoped_thread_domain::scoped_thread_domain(const thread_domain domain) : m_previous(t_currentDomain)
{
    t_currentDomain = domain;
}

sbk::core::scoped_thread_domain::~scoped_thread_domain() { t_currentDomain = m_previous; }

auto sbk::core::detail::warn_wrong_thread_domain(const thread_domain expected,
                                                 const std::source_location& location) -> void
{
    const std::string message = fmt::format(
        "Thread contract violation: {} ({}:{}) expects the {} thread but ran under '{}'. "
        "Warning once per call site; this becomes an assert after the threading cleanup.",
        location.function_name(), location.file_name(), location.line(), to_string(expected),
        to_string(get_current_thread_domain()));

    sbk_log(MA_LOG_LEVEL_WARNING, message.c_str());

    BOOST_ASSERT_MSG(false, "Thread contract violation");
}
