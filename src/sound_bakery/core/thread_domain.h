#pragma once

#include "sound_bakery/sound_bakery_common.h"

#include <atomic>
#include <cstdint>
#include <source_location>

/**
 * @file thread_domain.h
 * @brief Marks which logical thread owns the currently executing code, and warns on violations.
 *
 * Sound Bakery's threading contract is single-owner: database objects belong
 * to the studio domain, and the game-facing API belongs to the game domain.
 * Domains are scope-based rather than thread-id-based on purpose: the
 * executors are concurrencpp manual executors, which run on whatever thread
 * drains them. Marking the drain scope means "am I serialized with all other
 * studio work?" stays true even if the drain ever moves to another thread.
 */

namespace sbk::core
{
    enum class thread_domain : std::uint8_t
    {
        unknown,  //< Not inside any marked scope, e.g. the app's main thread outside update().
        game,     //< Inside system::update() - the thread the game/app pumps Sound Bakery from.
        studio,   //< Inside the studio drain (system::update_async) - owns the database and objects.
    };

    [[nodiscard]] auto get_current_thread_domain() -> thread_domain;
    [[nodiscard]] auto to_string(thread_domain domain) -> const char*;

    /**
     * @brief True when the current thread may act as @p domain.
     *
     * Normally that means being inside a matching scoped_thread_domain. In
     * single-threaded (editor) mode the sole owner thread satisfies every
     * domain: with no other thread pumping engine work, its access is
     * serialized by definition. The SBK_EXPECT_* macros use this.
     */
    [[nodiscard]] auto current_thread_satisfies(thread_domain domain) -> bool;

    /**
     * @brief Selects single-threaded (editor) mode from configuration.
     *
     * Mirrors sbk_system_config::singleThreadedUpdate and is applied once when
     * the system initialises - the config is the single source of truth, so
     * there is no separate "unset" call: initialising a multi-threaded system
     * simply passes @c false. When enabled, current_thread_satisfies() is
     * satisfied for every domain because a single thread owns all engine work.
     */
    auto set_single_threaded_mode(bool enabled) -> void;

    /**
     * @brief RAII marker: code inside this scope belongs to @p domain.
     *
     * Placed where an executor drains (see system::update and
     * system::update_async). Saves and restores the previous domain, so
     * nested scopes behave.
     */
    class SB_CLASS scoped_thread_domain final
    {
    public:
        explicit scoped_thread_domain(thread_domain domain);
        ~scoped_thread_domain();

        scoped_thread_domain(const scoped_thread_domain&)                    = delete;
        auto operator=(const scoped_thread_domain&) -> scoped_thread_domain& = delete;

    private:
        thread_domain m_previous;
    };

    namespace detail
    {
        auto warn_wrong_thread_domain(thread_domain expected, const std::source_location& location) -> void;
    }  // namespace detail
}  // namespace sbk::core

/**
 * @def SBK_EXPECT_STUDIO_THREAD / SBK_EXPECT_GAME_THREAD
 *
 * Declares that the enclosing function must run on the given domain. On
 * violation it logs a warning once per call site (not once per frame), so
 * running an app produces a complete, readable list of offending call sites.
 *
 * Deliberately a warning, not an assert: the current codebase still calls
 * into the database from the wrong threads in places, and a hard assert
 * would make it unusable before the threading cleanup lands. Once a call
 * site is clean, these graduate to asserts by strengthening the macro.
 */
#define SBK_DETAIL_EXPECT_THREAD_DOMAIN(expectedDomain)                                          \
    do                                                                                           \
    {                                                                                            \
        if (!::sbk::core::current_thread_satisfies(expectedDomain)) [[unlikely]]                 \
        {                                                                                        \
            static std::atomic<bool> sbkWarnedThreadDomain{false};                               \
            if (!sbkWarnedThreadDomain.exchange(true))                                           \
            {                                                                                    \
                ::sbk::core::detail::warn_wrong_thread_domain((expectedDomain),                  \
                                                              std::source_location::current()); \
            }                                                                                    \
        }                                                                                        \
    } while (0)

#define SBK_EXPECT_STUDIO_THREAD() SBK_DETAIL_EXPECT_THREAD_DOMAIN(::sbk::core::thread_domain::studio)
#define SBK_EXPECT_GAME_THREAD()   SBK_DETAIL_EXPECT_THREAD_DOMAIN(::sbk::core::thread_domain::game)
