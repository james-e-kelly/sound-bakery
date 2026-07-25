#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/error/error.h"

/**
 * @file result.h
 * @brief @ref sbk::result and the control-flow macros that make error handling automatic.
 *
 * The goal: a function calls another and, on failure, writes no logging or plumbing code.
 * It just forwards. Logging happens once, at the origin, courtesy of @ref sbk::make_error.
 *
 * ## Which macro do I use?
 *
 * Look at what YOUR function returns. That picks the prefix; the verb picks the action.
 *
 *   Your function returns ...   | Prefix       | Recommended?
 *   --------------------------- | ------------ | -----------------------------------------
 *   sbk::result<T>  (rich)      | (none)       | Yes - preferred for new C++ code
 *   sbk_status      (C code)    | SBK_STATUS_    | Only at the C ABI boundary / legacy code
 *   sbk_id          (lookups)   | SBK_ID_      | Only for legacy id-returning lookups
 *
 * Verbs (same meaning in every family):
 *
 *   CHECK  - guard a bool condition; on failure, log + return the given code as a failure.
 *   TRY    - run a fallible call; on failure, log (at origin) + return/forward the failure.
 *   FAIL   - unconditionally log + return a failure (e.g. an unreachable branch).
 *
 * Suffixes:
 *
 *   _MSG   - takes a fmt-style message + args for context the code alone can't convey.
 *   _C     - the call being TRY'd is a C function that returns @c sbk_status.
 *
 * So, for example:
 *   - In a `sbk::result<T>` function:  SBK_CHECK, SBK_CHECK_MSG, SBK_TRY, SBK_TRY_C, SBK_FAIL
 *   - In a `sbk_status`    function:  SBK_STATUS_CHECK, SBK_STATUS_CHECK_MSG, SBK_STATUS_TRY_C, SBK_STATUS_FAIL
 *   - In a `sbk_id`        function:  SBK_ID_CHECK, SBK_ID_CHECK_MSG
 *
 * See error/README.md for a fuller guide and worked examples.
 */

// `sbk::result`, `sbk::ok`, and `sbk::to_status` live in error.h (the lightweight, fmt-free
// header) so other headers can declare result-returning functions cheaply. This header adds
// the control-flow macros, which need fmt.

// --- internals -------------------------------------------------------------

#define SBK_DETAIL_CONCAT_(a, b) a##b
#define SBK_DETAIL_CONCAT(a, b)  SBK_DETAIL_CONCAT_(a, b)
#define SBK_DETAIL_UNIQUE(name)  SBK_DETAIL_CONCAT(name, __LINE__)

// ===========================================================================
// Family 1 - enclosing function returns sbk::result<T>   (preferred)
// ===========================================================================

/**
 * @brief Evaluates @p expr (a @c sbk::result<U>), binds its value via @p decl, else forwards the error.
 *
 * @code
 *   SBK_TRY(auto sound, load_sound(path));   // sound is the unwrapped value; errors early-return
 * @endcode
 */
#define SBK_TRY(decl, expr)                                                             \
    auto&& SBK_DETAIL_UNIQUE(sbkResult_) = (expr);                                      \
    if (!SBK_DETAIL_UNIQUE(sbkResult_).has_value()) [[unlikely]]                        \
        return ::tl::make_unexpected(std::move(SBK_DETAIL_UNIQUE(sbkResult_)).error()); \
    decl = std::move(SBK_DETAIL_UNIQUE(sbkResult_)).value()

/**
 * @brief Runs @p expr (a @c sbk::result<void>, or any result whose value is ignored); forwards its error.
 */
#define SBK_TRYV(expr)                                                                      \
    do                                                                                      \
    {                                                                                       \
        auto&& SBK_DETAIL_UNIQUE(sbkResult_) = (expr);                                      \
        if (!SBK_DETAIL_UNIQUE(sbkResult_).has_value()) [[unlikely]]                        \
            return ::tl::make_unexpected(std::move(SBK_DETAIL_UNIQUE(sbkResult_)).error()); \
    } while (0)

/**
 * @brief Calls a C function returning @c sbk_status; on failure logs at this site and forwards the error.
 */
#define SBK_TRY_C(expr)                                                   \
    do                                                                    \
    {                                                                     \
        const sbk_status SBK_DETAIL_UNIQUE(sbkCode_) = (expr);            \
        if (SBK_DETAIL_UNIQUE(sbkCode_) != SBK_SUCCESS) [[unlikely]]      \
            return ::sbk::make_error(SBK_DETAIL_UNIQUE(sbkCode_), #expr); \
    } while (0)

/**
 * @brief Like @c SBK_TRY_C, but attaches a fmt-formatted context message the code alone can't convey.
 *
 * @code
 *   SBK_TRY_C_MSG(sc_system_play_sound(sys, s, &inst), "voice playback for '{}'", voiceName);
 * @endcode
 */
#define SBK_TRY_C_MSG(expr, ...)                                                                                \
    do                                                                                                          \
    {                                                                                                           \
        const sbk_status SBK_DETAIL_UNIQUE(sbkCode_) = (expr);                                                  \
        if (SBK_DETAIL_UNIQUE(sbkCode_) != SBK_SUCCESS)                                                         \
            return ::sbk::make_error(SBK_DETAIL_UNIQUE(sbkCode_), ::fmt::format(__VA_ARGS__) + " [" #expr "]"); \
    } while (0)

/**
 * @brief Guards @p cond; if false, logs and returns @p code as an error (message defaults to the condition).
 */
#define SBK_CHECK(cond, code)                                         \
    do                                                                \
    {                                                                 \
        if (!(cond)) [[unlikely]]                                     \
            return ::sbk::make_error((code), "check failed: " #cond); \
    } while (0)

/**
 * @brief Guards @p cond; if false, logs a fmt-formatted @p message and returns @p code as an error.
 *
 * @code
 *   SBK_CHECK_MSG(voice != nullptr, SBK_ERR_NULL, "voice playback is broken: no voice for '{}'", name);
 * @endcode
 */
#define SBK_CHECK_MSG(cond, code, ...)                                    \
    do                                                                    \
    {                                                                     \
        if (!(cond)) [[unlikely]]                                         \
            return ::sbk::make_error((code), ::fmt::format(__VA_ARGS__)); \
    } while (0)

/**
 * @brief Unconditionally logs a fmt-formatted message and returns @p code as an error.
 *
 * Handy for unreachable branches, e.g. an unexpected @c default: case.
 */
#define SBK_FAIL(code, ...) return ::sbk::make_error((code), ::fmt::format(__VA_ARGS__))

// ===========================================================================
// Family 2 - enclosing function returns a raw sbk_status   (C ABI / legacy)
// ===========================================================================

/**
 * @brief Guards @p cond; if false, logs and returns @p code (message defaults to the condition).
 */
#define SBK_STATUS_CHECK(cond, code)                          \
    do                                                        \
    {                                                         \
        if (!(cond)) [[unlikely]]                             \
        {                                                     \
            ::sbk::log_error((code), "check failed: " #cond); \
            return (code);                                    \
        }                                                     \
    } while (0)

/**
 * @brief Guards @p cond; if false, logs a fmt-formatted @p message and returns @p code.
 */
#define SBK_STATUS_CHECK_MSG(cond, code, ...)                     \
    do                                                            \
    {                                                             \
        if (!(cond)) [[unlikely]]                                 \
        {                                                         \
            ::sbk::log_error((code), ::fmt::format(__VA_ARGS__)); \
            return (code);                                        \
        }                                                         \
    } while (0)

/**
 * @brief Calls a C function returning @c sbk_status; on failure logs and returns the same code.
 */
#define SBK_STATUS_TRY_C(expr)                                       \
    do                                                               \
    {                                                                \
        const sbk_status SBK_DETAIL_UNIQUE(sbkCode_) = (expr);       \
        if (SBK_DETAIL_UNIQUE(sbkCode_) != SBK_SUCCESS) [[unlikely]] \
        {                                                            \
            ::sbk::log_error(SBK_DETAIL_UNIQUE(sbkCode_), #expr);    \
            return SBK_DETAIL_UNIQUE(sbkCode_);                      \
        }                                                            \
    } while (0)

/**
 * @brief Like @c SBK_STATUS_TRY_C, but attaches a fmt-formatted context message.
 */
#define SBK_STATUS_TRY_C_MSG(expr, ...)                                    \
    do                                                                     \
    {                                                                      \
        const sbk_status SBK_DETAIL_UNIQUE(sbkCode_) = (expr);             \
        if (SBK_DETAIL_UNIQUE(sbkCode_) != SBK_SUCCESS) [[unlikely]]       \
        {                                                                  \
            ::sbk::log_error(SBK_DETAIL_UNIQUE(sbkCode_),                  \
                             ::fmt::format(__VA_ARGS__) + " [" #expr "]"); \
            return SBK_DETAIL_UNIQUE(sbkCode_);                            \
        }                                                                  \
    } while (0)

/**
 * @brief Unconditionally logs a fmt-formatted message and returns @p code.
 */
#define SBK_STATUS_FAIL(code, ...)                            \
    do                                                        \
    {                                                         \
        ::sbk::log_error((code), ::fmt::format(__VA_ARGS__)); \
        return (code);                                        \
    } while (0)

// ===========================================================================
// Family 3 - enclosing function returns sbk_id   (legacy lookups)
// ===========================================================================

/**
 * @brief Guards @p cond; if false, logs @p code and returns @c SBK_INVALID_ID (for @c sbk_id functions).
 */
#define SBK_ID_CHECK(cond, code)                              \
    do                                                        \
    {                                                         \
        if (!(cond)) [[unlikely]]                             \
        {                                                     \
            ::sbk::log_error((code), "check failed: " #cond); \
            return SBK_INVALID_ID;                            \
        }                                                     \
    } while (0)

/**
 * @brief Guards @p cond; if false, logs a fmt-formatted @p message and returns @c SBK_INVALID_ID.
 */
#define SBK_ID_CHECK_MSG(cond, code, ...)                         \
    do                                                            \
    {                                                             \
        if (!(cond)) [[unlikely]]                                 \
        {                                                         \
            ::sbk::log_error((code), ::fmt::format(__VA_ARGS__)); \
            return SBK_INVALID_ID;                                \
        }                                                         \
    } while (0)
